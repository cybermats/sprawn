#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include <cstdint>
#include <filesystem>
#include <vector>

namespace sprawn {

struct GlyphBitmap {
    uint32_t id;         // glyph ID (or codepoint, depending on caller)
    int      bearing_x;  // left bearing in pixels
    int      bearing_y;  // top bearing in pixels (from baseline, positive = up)
    int      advance_x;  // horizontal advance in pixels
    int      width;
    int      height;
    std::vector<uint8_t> pixels; // alpha-only (1 bpp) or RGBA (4 bpp)
    bool color{false};           // true when pixels are 4-byte RGBA (color emoji)
};

class FontFace {
public:
    // Load a .ttf/.otf file at a given pixel size.
    FontFace(const std::filesystem::path& path, int size_px);
    ~FontFace();

    FontFace(const FontFace&) = delete;
    FontFace& operator=(const FontFace&) = delete;

    // Rasterize a Unicode codepoint (looks up glyph index internally).
    GlyphBitmap rasterize(uint32_t codepoint);

    // Rasterize by glyph ID directly (for HarfBuzz output).
    GlyphBitmap rasterize_glyph(uint32_t glyph_id);

    // Check if this font has a glyph for the given codepoint.
    bool has_codepoint(uint32_t cp) const;

    // Get glyph index for a codepoint (0 = not found).
    uint32_t glyph_index(uint32_t codepoint) const;

    hb_font_t* hb_font() const { return hb_font_; }

    int line_height()    const { return line_height_; }
    int ascent()         const { return ascent_; }
    int advance_width()  const { return advance_width_; } // nominal cell width

    // Scale factor for bitmap-only fonts (e.g. color emoji).
    // Returns 1.0 for scalable fonts.
    double bitmap_scale() const { return bitmap_scale_; }

private:
    FT_Library  library_{};
    FT_Face     face_{};
    hb_font_t*  hb_font_{};
    int         size_px_{};
    int         line_height_{};
    int         ascent_{};
    int         advance_width_{};
    double      bitmap_scale_{1.0};
    bool        bitmap_only_{}; // true for fixed-size bitmap fonts (e.g. color emoji)
};

// Locate a usable monospace font on the current system.
// Returns an empty path if none found.
std::filesystem::path find_system_mono_font();

} // namespace sprawn
