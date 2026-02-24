#pragma once

#include "font.h"

#include <SDL2/SDL.h>
#include <cstdint>
#include <unordered_map>

namespace sprawn::detail {

struct CachedGlyph {
    SDL_Texture* texture = nullptr;
    int width = 0;
    int height = 0;
    int bearing_x = 0;
    int bearing_y = 0;
};

class GlyphCache {
public:
    explicit GlyphCache(SDL_Renderer* renderer, const Font& font);
    ~GlyphCache();

    GlyphCache(const GlyphCache&) = delete;
    GlyphCache& operator=(const GlyphCache&) = delete;

    const CachedGlyph& get(int font_index, uint32_t glyph_id);

private:
    static uint64_t make_key(int font_index, uint32_t glyph_id) {
        return (static_cast<uint64_t>(font_index) << 32) | glyph_id;
    }

    SDL_Renderer* renderer_;
    const Font& font_;
    std::unordered_map<uint64_t, CachedGlyph> cache_;
    CachedGlyph empty_glyph_;
};

} // namespace sprawn::detail
