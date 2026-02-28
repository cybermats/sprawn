#pragma once

#include "renderer.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace sprawn {

class GlyphAtlas;

struct GlyphEntry {
    uint32_t codepoint;
    int x;       // x position relative to run origin (pixels)
    int cluster; // byte index in source UTF-8 string (for cursor mapping)
};

struct GlyphRun {
    std::vector<GlyphEntry> glyphs;
    int total_width; // total advance width in pixels
};

class TextLayout {
public:
    TextLayout(GlyphAtlas& atlas, int line_height, int ascent);

    // Decode UTF-8 and build a GlyphRun (Phase 1: no HarfBuzz).
    GlyphRun shape_line(std::string_view utf8);

    // Blit all glyphs in the run at baseline position (x, y+ascent).
    void draw_run(Renderer& r, const GlyphRun& run, int x, int y, Color tint);

    // Pixel x-offset of the left edge of column col within the run.
    int x_for_column(const GlyphRun& run, size_t col) const;

    // Nearest column index for a given pixel x relative to run origin.
    size_t column_for_x(const GlyphRun& run, int x) const;

    int line_height() const { return line_height_; }
    int ascent()      const { return ascent_; }

private:
    GlyphAtlas& atlas_;
    int line_height_;
    int ascent_;
};

} // namespace sprawn
