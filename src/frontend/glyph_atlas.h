#pragma once

#include "font_chain.h"
#include <sprawn/frontend/renderer.h>

#include <SDL2/SDL.h>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace sprawn {

struct AtlasGlyph {
    SDL_Rect rect;     // location in atlas texture (pixels)
    int bearing_x;     // left bearing
    int bearing_y;     // top bearing from baseline (positive = up)
    int advance_x;     // horizontal advance in pixels
};

class GlyphAtlas {
public:
    GlyphAtlas(Renderer& renderer, FontChain& fonts,
               int atlas_w = 1024, int atlas_h = 1024);
    ~GlyphAtlas();

    GlyphAtlas(const GlyphAtlas&) = delete;
    GlyphAtlas& operator=(const GlyphAtlas&) = delete;

    // Clear all cached glyphs and reset the atlas. Re-pre-caches ASCII.
    void clear();

    // Returns nullptr if the glyph cannot be rendered.
    const AtlasGlyph* get_or_add(uint32_t glyph_id, uint8_t font_index = 0);

    // Const lookup — returns nullptr if the glyph is not already cached.
    const AtlasGlyph* get(uint32_t glyph_id, uint8_t font_index = 0) const;

    SDL_Texture* texture() const { return texture_; }

private:
    Renderer&   renderer_;
    FontChain&  fonts_;
    SDL_Texture* texture_{};
    int atlas_w_, atlas_h_;

    // Shelf packing state
    int cur_x_{1};   // leave 1px border
    int cur_y_{1};
    int shelf_h_{0};

    // Key: (uint64_t(font_index) << 32) | glyph_id
    std::unordered_map<uint64_t, AtlasGlyph> cache_;

    static uint64_t make_key(uint32_t glyph_id, uint8_t font_index) {
        return (uint64_t(font_index) << 32) | glyph_id;
    }

    // Upload a single glyph bitmap region to the texture.
    void upload_glyph(int tex_x, int tex_y, const GlyphBitmap& bm);
};

} // namespace sprawn
