#include "glyph_atlas.h"

#include <cstring>
#include <stdexcept>

namespace sprawn {

GlyphAtlas::GlyphAtlas(Renderer& renderer, FontFace& font,
                       int atlas_w, int atlas_h)
    : renderer_(renderer), font_(font),
      atlas_w_(atlas_w), atlas_h_(atlas_h)
{
    texture_ = renderer_.create_texture(atlas_w_, atlas_h_);
    if (!texture_)
        throw std::runtime_error("Failed to create glyph atlas texture");

    // Clear texture to transparent
    std::vector<uint8_t> clear(atlas_w_ * atlas_h_ * 4, 0);
    renderer_.update_texture(texture_, clear.data(), atlas_w_ * 4);

    // Pre-cache common ASCII printable range
    for (uint32_t cp = 32; cp < 127; ++cp)
        get_or_add(cp);
}

GlyphAtlas::~GlyphAtlas() {
    if (texture_) SDL_DestroyTexture(texture_);
}

const AtlasGlyph* GlyphAtlas::get_or_add(uint32_t codepoint) {
    auto it = cache_.find(codepoint);
    if (it != cache_.end())
        return &it->second;

    GlyphBitmap bm = font_.rasterize(codepoint);
    if (bm.pixels.empty() && (bm.width == 0 || bm.height == 0)) {
        // Invisible or missing glyph — store a dummy entry so we don't retry
        AtlasGlyph ag{};
        ag.rect      = {0, 0, 0, 0};
        ag.bearing_x = bm.bearing_x;
        ag.bearing_y = bm.bearing_y;
        ag.advance_x = bm.advance_x > 0 ? bm.advance_x : font_.advance_width();
        cache_[codepoint] = ag;
        return &cache_[codepoint];
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

    cache_[codepoint] = ag;
    return &cache_[codepoint];
}

void GlyphAtlas::upload_glyph(int tex_x, int tex_y, const GlyphBitmap& bm) {
    // Convert alpha-only bitmap to RGBA (white text, varying alpha)
    int w = bm.width;
    int h = bm.height;
    std::vector<uint8_t> rgba(w * h * 4);
    for (int i = 0; i < w * h; ++i) {
        rgba[i * 4 + 0] = 255;
        rgba[i * 4 + 1] = 255;
        rgba[i * 4 + 2] = 255;
        rgba[i * 4 + 3] = bm.pixels[i];
    }

    SDL_Rect dst{tex_x, tex_y, w, h};
    SDL_UpdateTexture(texture_, &dst, rgba.data(), w * 4);
}

} // namespace sprawn
