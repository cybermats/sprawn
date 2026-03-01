#include "font_face.h"

#include <stdexcept>
#include <array>
#include <cmath>

namespace sprawn {

namespace {
constexpr std::array<const char*, 6> kFallbackFonts = {
    "/home/mats/.local/share/fonts/JetBrainsMono-Regular.ttf",
    "/usr/share/fonts/truetype/noto/NotoSansMono-Regular.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
    "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
    nullptr
};
} // namespace

std::filesystem::path find_system_mono_font() {
    for (const char* p : kFallbackFonts) {
        if (!p) break;
        if (std::filesystem::exists(p)) return p;
    }
    return {};
}

FontFace::FontFace(const std::filesystem::path& path, int size_px)
    : size_px_(size_px)
{
    if (FT_Init_FreeType(&library_))
        throw std::runtime_error("FreeType init failed");

    if (FT_New_Face(library_, path.c_str(), 0, &face_))
        throw std::runtime_error("Failed to load font: " + path.string());

    FT_Error size_err = FT_Set_Pixel_Sizes(face_, 0, static_cast<FT_UInt>(size_px));
    if (size_err && FT_HAS_FIXED_SIZES(face_) && face_->num_fixed_sizes > 0) {
        // Pure bitmap font (e.g. NotoColorEmoji) — select nearest strike
        bitmap_only_ = true;
        FT_Select_Size(face_, 0);
        int strike_h = face_->available_sizes[0].height;
        if (strike_h > 0)
            bitmap_scale_ = static_cast<double>(size_px) / strike_h;
    }

    // Metrics in 26.6 fixed point → integer pixels
    auto* metrics = &face_->size->metrics;
    line_height_   = static_cast<int>(metrics->height >> 6);
    ascent_        = static_cast<int>(metrics->ascender >> 6);

    // Advance width of '0' as nominal cell width (monospace assumption)
    FT_UInt idx = FT_Get_Char_Index(face_, '0');
    if (idx && !FT_Load_Glyph(face_, idx, FT_LOAD_DEFAULT))
        advance_width_ = static_cast<int>(face_->glyph->advance.x >> 6);
    else
        advance_width_ = size_px / 2;

    // Create HarfBuzz font from FreeType face
    hb_font_ = hb_ft_font_create_referenced(face_);
}

FontFace::~FontFace() {
    if (hb_font_)  hb_font_destroy(hb_font_);
    if (face_)     FT_Done_Face(face_);
    if (library_)  FT_Done_FreeType(library_);
}

GlyphBitmap FontFace::rasterize(uint32_t codepoint) {
    FT_UInt glyph_index = FT_Get_Char_Index(face_, codepoint);
    if (glyph_index == 0)
        return {}; // not in font

    GlyphBitmap bm = rasterize_glyph(glyph_index);
    bm.id = codepoint; // caller expects codepoint in legacy path
    return bm;
}

GlyphBitmap FontFace::rasterize_glyph(uint32_t glyph_id) {
    if (FT_Load_Glyph(face_, glyph_id, FT_LOAD_DEFAULT | FT_LOAD_COLOR))
        return {};

    if (FT_Render_Glyph(face_->glyph, FT_RENDER_MODE_NORMAL))
        return {};

    FT_Bitmap& bm = face_->glyph->bitmap;
    GlyphBitmap result;
    result.id        = glyph_id;
    result.bearing_x = face_->glyph->bitmap_left;
    result.bearing_y = face_->glyph->bitmap_top;
    result.advance_x = static_cast<int>(face_->glyph->advance.x >> 6);
    result.width     = static_cast<int>(bm.width);
    result.height    = static_cast<int>(bm.rows);

    if (bm.width > 0 && bm.rows > 0) {
        if (bm.pixel_mode == FT_PIXEL_MODE_BGRA) {
            // Color emoji: convert BGRA → RGBA (4 bytes per pixel)
            result.color = true;
            int src_w = static_cast<int>(bm.width);
            int src_h = static_cast<int>(bm.rows);

            // Convert BGRA → RGBA into a temporary buffer
            std::vector<uint8_t> rgba(src_w * src_h * 4);
            for (int row = 0; row < src_h; ++row) {
                const unsigned char* s = bm.buffer + row * std::abs(bm.pitch);
                unsigned char* d = rgba.data() + row * src_w * 4;
                for (int col = 0; col < src_w; ++col) {
                    d[col * 4 + 0] = s[col * 4 + 2]; // R ← B
                    d[col * 4 + 1] = s[col * 4 + 1]; // G
                    d[col * 4 + 2] = s[col * 4 + 0]; // B ← R
                    d[col * 4 + 3] = s[col * 4 + 3]; // A
                }
            }

            // Scale down if the bitmap is from a larger strike (bitmap-only font)
            if (bitmap_only_ && src_h > size_px_) {
                double scale = static_cast<double>(size_px_) / src_h;
                int dst_w = std::max(1, static_cast<int>(std::round(src_w * scale)));
                int dst_h = std::max(1, static_cast<int>(std::round(src_h * scale)));

                std::vector<uint8_t> scaled(dst_w * dst_h * 4);
                for (int dy = 0; dy < dst_h; ++dy) {
                    for (int dx = 0; dx < dst_w; ++dx) {
                        int sx = static_cast<int>(dx / scale);
                        int sy = static_cast<int>(dy / scale);
                        if (sx >= src_w) sx = src_w - 1;
                        if (sy >= src_h) sy = src_h - 1;
                        int si = (sy * src_w + sx) * 4;
                        int di = (dy * dst_w + dx) * 4;
                        scaled[di + 0] = rgba[si + 0];
                        scaled[di + 1] = rgba[si + 1];
                        scaled[di + 2] = rgba[si + 2];
                        scaled[di + 3] = rgba[si + 3];
                    }
                }

                result.width  = dst_w;
                result.height = dst_h;
                result.bearing_x = static_cast<int>(std::round(result.bearing_x * scale));
                result.bearing_y = static_cast<int>(std::round(result.bearing_y * scale));
                result.advance_x = static_cast<int>(std::round(result.advance_x * scale));
                result.pixels = std::move(scaled);
            } else {
                result.pixels = std::move(rgba);
            }
        } else {
            // Grayscale: 1 byte per pixel (alpha only)
            result.pixels.resize(bm.width * bm.rows);
            if (bm.pixel_mode == FT_PIXEL_MODE_GRAY) {
                for (unsigned row = 0; row < bm.rows; ++row) {
                    const unsigned char* src = bm.buffer + row * std::abs(bm.pitch);
                    unsigned char* dst = result.pixels.data() + row * bm.width;
                    for (unsigned col = 0; col < bm.width; ++col)
                        dst[col] = src[col];
                }
            }
        }
    }
    return result;
}

bool FontFace::has_codepoint(uint32_t cp) const {
    return FT_Get_Char_Index(face_, cp) != 0;
}

uint32_t FontFace::glyph_index(uint32_t codepoint) const {
    return FT_Get_Char_Index(face_, codepoint);
}

} // namespace sprawn
