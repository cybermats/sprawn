#include "glyph_atlas.h"

#include <cstring>
#include <stdexcept>

namespace sprawn {

GlyphAtlas::GlyphAtlas(Renderer& renderer, FontChain& fonts,
                       int atlas_w, int atlas_h)
    : renderer_(renderer), fonts_(fonts),
      atlas_w_(atlas_w), atlas_h_(atlas_h)
{
    texture_ = renderer_.create_texture(atlas_w_, atlas_h_);
    if (!texture_)
        throw std::runtime_error("Failed to create glyph atlas texture");

    // Clear texture to transparent
    std::vector<uint8_t> clear(atlas_w_ * atlas_h_ * 4, 0);
    renderer_.update_texture(texture_, clear.data(), atlas_w_ * 4);

    // Pre-cache common ASCII printable range using primary font glyph IDs
    FontFace& primary = fonts_.primary();
    for (uint32_t cp = 32; cp < 127; ++cp) {
        uint32_t gid = primary.glyph_index(cp);
        if (gid != 0)
            get_or_add(gid, 0);
    }
}

void GlyphAtlas::clear() {
    cache_.clear();
    cur_x_ = 1;
    cur_y_ = 1;
    shelf_h_ = 0;

    // Blank the texture
    std::vector<uint8_t> blank(atlas_w_ * atlas_h_ * 4, 0);
    renderer_.update_texture(texture_, blank.data(), atlas_w_ * 4);

    // Re-pre-cache common ASCII printable range using primary font glyph IDs
    FontFace& primary = fonts_.primary();
    for (uint32_t cp = 32; cp < 127; ++cp) {
        uint32_t gid = primary.glyph_index(cp);
        if (gid != 0)
            get_or_add(gid, 0);
    }
}

GlyphAtlas::~GlyphAtlas() {
    if (texture_) SDL_DestroyTexture(texture_);
}

const AtlasGlyph* GlyphAtlas::get_or_add(uint32_t glyph_id, uint8_t font_index) {
    uint64_t key = make_key(glyph_id, font_index);
    auto it = cache_.find(key);
    if (it != cache_.end())
        return &it->second;

    GlyphBitmap bm = fonts_.font(font_index).rasterize_glyph(glyph_id);
    if (bm.pixels.empty() && (bm.width == 0 || bm.height == 0)) {
        // Invisible or missing glyph — store a dummy entry so we don't retry
        AtlasGlyph ag{};
        ag.rect      = {0, 0, 0, 0};
        ag.bearing_x = bm.bearing_x;
        ag.bearing_y = bm.bearing_y;
        ag.advance_x = bm.advance_x > 0 ? bm.advance_x : fonts_.advance_width();
        cache_[key] = ag;
        return &cache_[key];
    }

    const int pad = 1;
    int w = bm.width  + pad;
    int h = bm.height + pad;

    // Start a new shelf if needed
    if (cur_x_ + w > atlas_w_) {
        cur_x_   = 1;
        cur_y_  += shelf_h_ + pad;
        shelf_h_ = 0;
    }

    if (cur_y_ + h > atlas_h_) {
        // Atlas full — return nullptr (caller should handle gracefully)
        return nullptr;
    }

    if (h > shelf_h_) shelf_h_ = h;

    upload_glyph(cur_x_, cur_y_, bm);

    AtlasGlyph ag;
    ag.rect      = {cur_x_, cur_y_, bm.width, bm.height};
    ag.bearing_x = bm.bearing_x;
    ag.bearing_y = bm.bearing_y;
    ag.advance_x = bm.advance_x;

    cur_x_ += w;

    cache_[key] = ag;
    return &cache_[key];
}

void GlyphAtlas::upload_glyph(int tex_x, int tex_y, const GlyphBitmap& bm) {
    int w = bm.width;
    int h = bm.height;
    SDL_Rect dst{tex_x, tex_y, w, h};

    if (bm.color) {
        // Color emoji: pixels are already RGBA
        SDL_UpdateTexture(texture_, &dst, bm.pixels.data(), w * 4);
    } else {
        // Alpha-only bitmap → RGBA (white text, varying alpha)
        std::vector<uint8_t> rgba(w * h * 4);
        for (int i = 0; i < w * h; ++i) {
            rgba[i * 4 + 0] = 255;
            rgba[i * 4 + 1] = 255;
            rgba[i * 4 + 2] = 255;
            rgba[i * 4 + 3] = bm.pixels[i];
        }
        SDL_UpdateTexture(texture_, &dst, rgba.data(), w * 4);
    }
}

} // namespace sprawn
