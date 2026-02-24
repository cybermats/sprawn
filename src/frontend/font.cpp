#include "font.h"
#include "util.h"
#include <stdexcept>
#include <cstring>

namespace sprawn::detail {

Font::Font(const std::string& path, int pixel_size)
    : pixel_size_(pixel_size)
{
    if (FT_Init_FreeType(&ft_library_)) {
        throw std::runtime_error("FT_Init_FreeType failed");
    }

    if (FT_New_Face(ft_library_, path.c_str(), 0, &primary_.ft_face)) {
        FT_Done_FreeType(ft_library_);
        throw std::runtime_error("Failed to load font: " + path);
    }

    if (FT_Set_Pixel_Sizes(primary_.ft_face, 0, static_cast<FT_UInt>(pixel_size))) {
        FT_Done_Face(primary_.ft_face);
        FT_Done_FreeType(ft_library_);
        throw std::runtime_error("FT_Set_Pixel_Sizes failed");
    }

    primary_.hb_font = hb_ft_font_create(primary_.ft_face, nullptr);
    if (!primary_.hb_font) {
        FT_Done_Face(primary_.ft_face);
        FT_Done_FreeType(ft_library_);
        throw std::runtime_error("hb_ft_font_create failed");
    }

    ascender_ = static_cast<int>(primary_.ft_face->size->metrics.ascender >> 6);
    int descender = static_cast<int>(primary_.ft_face->size->metrics.descender >> 6);
    line_height_ = ascender_ - descender;
}

Font::~Font() {
    for (auto& fb : fallbacks_) {
        if (fb.hb_font) hb_font_destroy(fb.hb_font);
        if (fb.ft_face) FT_Done_Face(fb.ft_face);
    }
    if (primary_.hb_font) hb_font_destroy(primary_.hb_font);
    if (primary_.ft_face) FT_Done_Face(primary_.ft_face);
    if (ft_library_) FT_Done_FreeType(ft_library_);
}

void Font::add_fallback(const std::string& path) {
    FontFace fb;
    if (FT_New_Face(ft_library_, path.c_str(), 0, &fb.ft_face)) {
        return; // Silently skip unavailable fallback fonts.
    }
    if (FT_Set_Pixel_Sizes(fb.ft_face, 0, static_cast<FT_UInt>(pixel_size_))) {
        FT_Done_Face(fb.ft_face);
        return;
    }
    fb.hb_font = hb_ft_font_create(fb.ft_face, nullptr);
    if (!fb.hb_font) {
        FT_Done_Face(fb.ft_face);
        return;
    }
    fallbacks_.push_back(fb);
}

ShapedRun Font::shape_with(int font_index, std::string_view text) const {
    const FontFace& face = (font_index == 0)
        ? primary_
        : fallbacks_[static_cast<std::size_t>(font_index - 1)];

    ShapedRun run;
    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8(buf, text.data(), static_cast<int>(text.size()), 0,
                       static_cast<int>(text.size()));
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(buf, HB_SCRIPT_COMMON);
    hb_buffer_guess_segment_properties(buf);

    hb_shape(face.hb_font, buf, nullptr, 0);

    unsigned int glyph_count = 0;
    hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

    run.glyphs.reserve(glyph_count);
    int total_advance = 0;

    for (unsigned int i = 0; i < glyph_count; ++i) {
        ShapedGlyph sg;
        sg.glyph_id = glyph_info[i].codepoint;
        sg.font_index = font_index;
        sg.x_offset = glyph_pos[i].x_offset >> 6;
        sg.y_offset = glyph_pos[i].y_offset >> 6;
        sg.x_advance = glyph_pos[i].x_advance >> 6;
        sg.y_advance = glyph_pos[i].y_advance >> 6;
        total_advance += sg.x_advance;
        run.glyphs.push_back(sg);
    }

    run.total_advance = total_advance;
    hb_buffer_destroy(buf);
    return run;
}

ShapedRun Font::shape(std::string_view text) const {
    // Shape with primary font first.
    ShapedRun run = shape_with(0, text);

    if (fallbacks_.empty()) return run;

    // Check for .notdef glyphs (glyph_id == 0) and try fallbacks.
    // We need original cluster info to map back to source text.
    // Re-shape to get cluster values.
    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8(buf, text.data(), static_cast<int>(text.size()), 0,
                       static_cast<int>(text.size()));
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(buf, HB_SCRIPT_COMMON);
    hb_buffer_guess_segment_properties(buf);
    hb_shape(primary_.hb_font, buf, nullptr, 0);

    unsigned int glyph_count = 0;
    hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);

    // For each .notdef glyph, try fallback fonts.
    for (unsigned int i = 0; i < glyph_count; ++i) {
        if (glyph_info[i].codepoint != 0) continue; // Has glyph in primary.

        // Find the byte range for this cluster.
        uint32_t cluster_start = glyph_info[i].cluster;
        uint32_t cluster_end;
        if (i + 1 < glyph_count) {
            cluster_end = glyph_info[i + 1].cluster;
        } else {
            cluster_end = static_cast<uint32_t>(text.size());
        }

        if (cluster_end <= cluster_start) {
            // RTL or complex case — just skip for now.
            continue;
        }

        std::string_view cluster_text(text.data() + cluster_start,
                                      cluster_end - cluster_start);

        // Try each fallback.
        for (int fb = 0; fb < static_cast<int>(fallbacks_.size()); ++fb) {
            ShapedRun fb_run = shape_with(fb + 1, cluster_text);
            if (!fb_run.glyphs.empty() && fb_run.glyphs[0].glyph_id != 0) {
                // Fallback produced a valid glyph. Replace in the main run.
                // Adjust advance: subtract old, add new.
                run.total_advance -= run.glyphs[i].x_advance;
                run.glyphs[i] = fb_run.glyphs[0];
                run.total_advance += run.glyphs[i].x_advance;
                break;
            }
        }
    }

    hb_buffer_destroy(buf);
    return run;
}

GlyphBitmap Font::rasterize(int font_index, uint32_t glyph_id) const {
    const FT_Face face = (font_index == 0)
        ? primary_.ft_face
        : fallbacks_[static_cast<std::size_t>(font_index - 1)].ft_face;

    GlyphBitmap bmp;

    if (FT_Load_Glyph(face, glyph_id, FT_LOAD_DEFAULT)) {
        return bmp;
    }
    if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) {
        return bmp;
    }

    const FT_Bitmap& ftbmp = face->glyph->bitmap;
    bmp.width = static_cast<int>(ftbmp.width);
    bmp.height = static_cast<int>(ftbmp.rows);
    bmp.bearing_x = face->glyph->bitmap_left;
    bmp.bearing_y = face->glyph->bitmap_top;

    if (bmp.width > 0 && bmp.height > 0) {
        bmp.buffer.resize(static_cast<std::size_t>(bmp.width * bmp.height));
        for (int row = 0; row < bmp.height; ++row) {
            std::memcpy(
                bmp.buffer.data() + row * bmp.width,
                ftbmp.buffer + row * ftbmp.pitch,
                static_cast<std::size_t>(bmp.width)
            );
        }
    }

    return bmp;
}

} // namespace sprawn::detail
