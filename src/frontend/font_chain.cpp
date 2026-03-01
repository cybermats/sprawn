#include "font_chain.h"

#include <cstdio>

namespace sprawn {

FontChain::FontChain(const std::filesystem::path& primary, int size_px)
    : size_px_(size_px)
{
    fonts_.push_back(std::make_unique<FontFace>(primary, size_px));
}

void FontChain::add_fallback(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) return;
    try {
        fonts_.push_back(std::make_unique<FontFace>(path, size_px_));
    } catch (const std::exception& e) {
        std::fprintf(stderr, "sprawn: skipping fallback font '%s': %s\n",
                     path.c_str(), e.what());
    }
}

std::pair<uint8_t, uint32_t> FontChain::resolve(uint32_t codepoint) const {
    for (uint8_t i = 0; i < fonts_.size(); ++i) {
        uint32_t gid = fonts_[i]->glyph_index(codepoint);
        if (gid != 0)
            return {i, gid};
    }
    return {0, 0};
}

} // namespace sprawn
