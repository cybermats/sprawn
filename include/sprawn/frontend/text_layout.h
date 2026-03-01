#pragma once

#include "renderer.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace sprawn {

class GlyphAtlas;
class FontChain;

struct GlyphEntry {
    uint32_t glyph_id;
    uint8_t  font_index{0};
    int x;         // x position relative to run origin (pixels)
    int y_offset;  // vertical offset from HarfBuzz (pixels, positive = up)
    int cluster;   // byte index in source UTF-8 string (for cursor mapping)
};

struct GlyphRun {
    std::vector<GlyphEntry> glyphs;
    int total_width;       // total advance width in pixels
    bool truncated{false}; // true if shaping stopped early (lazy shaping)
};

class TextLayout {
public:
    TextLayout(GlyphAtlas& atlas, FontChain& fonts, float dpi_scale = 1.0f);

    // Shape a UTF-8 line using HarfBuzz + ICU BiDi.
    // If max_width_px > 0, stop shaping after exceeding that logical width (lazy shaping).
    GlyphRun shape_line(std::string_view utf8, int max_width_px = 0);

    // Blit all glyphs in the run at baseline position (x, y+ascent).
    void draw_run(Renderer& r, const GlyphRun& run, int x, int y, Color tint);

    // Pixel x-offset of the left edge of column col within the run.
    // utf8 is the original line text (needed for byte↔codepoint mapping).
    int x_for_column(const GlyphRun& run, std::string_view utf8, size_t col) const;

    // Nearest column index for a given pixel x relative to run origin.
    size_t column_for_x(const GlyphRun& run, std::string_view utf8, int x) const;

    // Deleted old signatures so missed call sites fail at compile time.
    int x_for_column(const GlyphRun&, size_t) const = delete;
    size_t column_for_x(const GlyphRun&, int) const = delete;

    // Reinitialize with a new DPI scale (after font rebuild).
    void reset(float dpi_scale);

    int line_height() const { return line_height_; }
    int ascent()      const { return ascent_; }

private:
    GlyphAtlas& atlas_;
    FontChain&  fonts_;
    float dpi_scale_{1.0f};
    int line_height_;
    int ascent_;
};

} // namespace sprawn
