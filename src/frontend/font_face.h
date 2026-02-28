#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstdint>
#include <filesystem>
#include <vector>

namespace sprawn {

struct GlyphBitmap {
    uint32_t codepoint;
    int      bearing_x;  // left bearing in pixels
    int      bearing_y;  // top bearing in pixels (from baseline, positive = up)
    int      advance_x;  // horizontal advance in pixels
    int      width;
    int      height;
    std::vector<uint8_t> pixels; // 1 byte per pixel, 0-255 alpha
};

class FontFace {
public:
    // Load a .ttf/.otf file at a given pixel size.
    FontFace(const std::filesystem::path& path, int size_px);
    ~FontFace();

    FontFace(const FontFace&) = delete;
    FontFace& operator=(const FontFace&) = delete;

    // Rasterize a Unicode codepoint.  Returns an empty bitmap if the glyph
    // is not present in this font.
    GlyphBitmap rasterize(uint32_t codepoint);

    int line_height()    const { return line_height_; }
    int ascent()         const { return ascent_; }
    int advance_width()  const { return advance_width_; } // nominal cell width

private:
    FT_Library library_{};
    FT_Face    face_{};
    int        line_height_{};
    int        ascent_{};
    int        advance_width_{};
};

// Locate a usable monospace font on the current system.
// Returns an empty path if none found.
std::filesystem::path find_system_mono_font();

} // namespace sprawn
