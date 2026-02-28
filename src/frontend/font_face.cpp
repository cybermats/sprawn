#include "font_face.h"

#include <stdexcept>
#include <array>

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

FontFace::FontFace(const std::filesystem::path& path, int size_px) {
    if (FT_Init_FreeType(&library_))
        throw std::runtime_error("FreeType init failed");

    if (FT_New_Face(library_, path.c_str(), 0, &face_))
        throw std::runtime_error("Failed to load font: " + path.string());

    FT_Set_Pixel_Sizes(face_, 0, static_cast<FT_UInt>(size_px));

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
}

FontFace::~FontFace() {
    if (face_)    FT_Done_Face(face_);
    if (library_) FT_Done_FreeType(library_);
}

GlyphBitmap FontFace::rasterize(uint32_t codepoint) {
    FT_UInt glyph_index = FT_Get_Char_Index(face_, codepoint);
    if (glyph_index == 0)
        return {}; // not in font

    if (FT_Load_Glyph(face_, glyph_index, FT_LOAD_DEFAULT))
        return {};

    if (FT_Render_Glyph(face_->glyph, FT_RENDER_MODE_NORMAL))
        return {};

    FT_Bitmap& bm = face_->glyph->bitmap;
    GlyphBitmap result;
    result.codepoint = codepoint;
    result.bearing_x = face_->glyph->bitmap_left;
    result.bearing_y = face_->glyph->bitmap_top;
    result.advance_x = static_cast<int>(face_->glyph->advance.x >> 6);
    result.width     = static_cast<int>(bm.width);
    result.height    = static_cast<int>(bm.rows);

    if (bm.width > 0 && bm.rows > 0) {
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
    return result;
}

} // namespace sprawn
