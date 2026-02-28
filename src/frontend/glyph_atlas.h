#pragma once

#include "font_face.h"
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
    GlyphAtlas(Renderer& renderer, FontFace& font,
               int atlas_w = 1024, int atlas_h = 1024);
    ~GlyphAtlas();

    GlyphAtlas(const GlyphAtlas&) = delete;
    GlyphAtlas& operator=(const GlyphAtlas&) = delete;

    // Returns nullptr if the codepoint cannot be rendered.
    const AtlasGlyph* get_or_add(uint32_t codepoint);

    SDL_Texture* texture() const { return texture_; }

private:
    Renderer&   renderer_;
    FontFace&   font_;
    SDL_Texture* texture_{};
    int atlas_w_, atlas_h_;

    // Shelf packing state
    int cur_x_{1};   // leave 1px border
    int cur_y_{1};
    int shelf_h_{0};

    std::unordered_map<uint32_t, AtlasGlyph> cache_;

    // Upload a single glyph bitmap region to the texture.
    void upload_glyph(int tex_x, int tex_y, const GlyphBitmap& bm);
};

} // namespace sprawn
