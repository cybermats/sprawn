#pragma once

#include "font_face.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <utility>
#include <vector>

namespace sprawn {

class FontChain {
public:
    FontChain(const std::filesystem::path& primary, int size_px);
    ~FontChain();

    void add_fallback(const std::filesystem::path& path);

    FontFace& primary()               { return *fonts_[0]; }
    const FontFace& primary() const   { return *fonts_[0]; }
    FontFace& font(uint8_t index)     { assert(fonts_[index]); return *fonts_[index]; }
    uint8_t   count() const           { return static_cast<uint8_t>(fonts_.size()); }

    // Find a font that has the codepoint. Returns {font_index, glyph_id}.
    // Returns {0, 0} if no font has it.
    std::pair<uint8_t, uint32_t> resolve(uint32_t codepoint) const;

    // Rebuild all fonts at a new pixel size. Clears and recreates all FontFaces.
    void rebuild(int new_size_px);

    int size_px()       const { return size_px_; }
    int line_height()   const { return fonts_[0]->line_height(); }
    int ascent()        const { return fonts_[0]->ascent(); }
    int advance_width() const { return fonts_[0]->advance_width(); }

private:
    FT_Library library_{};
    std::vector<std::unique_ptr<FontFace>> fonts_;
    std::vector<std::filesystem::path> paths_;
    int size_px_;
};

} // namespace sprawn
