#include "glyph_cache.h"

namespace sprawn::detail {

GlyphCache::GlyphCache(SDL_Renderer* renderer, const Font& font)
    : renderer_(renderer), font_(font)
{
}

GlyphCache::~GlyphCache() {
    for (auto& [id, glyph] : cache_) {
        if (glyph.texture) {
            SDL_DestroyTexture(glyph.texture);
        }
    }
}

const CachedGlyph& GlyphCache::get(int font_index, uint32_t glyph_id) {
    uint64_t key = make_key(font_index, glyph_id);
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        return it->second;
    }

    GlyphBitmap bmp = font_.rasterize(font_index, glyph_id);

    CachedGlyph cached;
    cached.width = bmp.width;
    cached.height = bmp.height;
    cached.bearing_x = bmp.bearing_x;
    cached.bearing_y = bmp.bearing_y;

    if (bmp.width > 0 && bmp.height > 0) {
        SDL_Texture* tex = SDL_CreateTexture(
            renderer_,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_STATIC,
            bmp.width, bmp.height
        );

        if (tex) {
            std::vector<uint32_t> rgba(static_cast<std::size_t>(bmp.width * bmp.height));
            for (std::size_t i = 0; i < rgba.size(); ++i) {
                uint8_t a = bmp.buffer[i];
                rgba[i] = (0xFFu << 24) | (0xFFu << 16) | (0xFFu << 8) | a;
            }

            SDL_UpdateTexture(tex, nullptr, rgba.data(), bmp.width * 4);
            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
            cached.texture = tex;
        }
    }

    auto [ins, _] = cache_.emplace(key, cached);
    return ins->second;
}

} // namespace sprawn::detail
