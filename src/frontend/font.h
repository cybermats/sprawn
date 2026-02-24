#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>

namespace sprawn::detail {

struct ShapedGlyph {
    uint32_t glyph_id;
    int font_index;  // 0 = primary, 1+ = fallback
    int x_offset;    // from HarfBuzz, in pixels
    int y_offset;
    int x_advance;
    int y_advance;
};

struct ShapedRun {
    std::vector<ShapedGlyph> glyphs;
    int total_advance = 0;
};

struct GlyphBitmap {
    std::vector<uint8_t> buffer; // alpha values
    int width = 0;
    int height = 0;
    int bearing_x = 0;
    int bearing_y = 0;
};

class Font {
public:
    Font(const std::string& path, int pixel_size);
    ~Font();

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    // Add a fallback font (e.g. for CJK coverage).
    void add_fallback(const std::string& path);

    ShapedRun shape(std::string_view text) const;
    GlyphBitmap rasterize(int font_index, uint32_t glyph_id) const;

    int line_height() const { return line_height_; }
    int ascender() const { return ascender_; }

private:
    struct FontFace {
        FT_Face ft_face = nullptr;
        hb_font_t* hb_font = nullptr;
    };

    ShapedRun shape_with(int font_index, std::string_view text) const;

    FT_Library ft_library_ = nullptr;
    FontFace primary_;
    std::vector<FontFace> fallbacks_;
    int line_height_ = 0;
    int ascender_ = 0;
    int pixel_size_ = 0;
};

} // namespace sprawn::detail
