#pragma once

#include <sprawn/color.h>

#include <SDL2/SDL.h>

namespace sprawn {

using TextureHandle = SDL_Texture*;

class Renderer {
public:
    explicit Renderer(SDL_Renderer* r);

    void begin_frame(Color bg);

    void fill_rect(Rect rect, Color c);

    // Creates an RGBA streaming texture for the glyph atlas.
    TextureHandle create_texture(int w, int h);

    // Upload pixel data (row-major RGBA bytes, pitch in bytes).
    void update_texture(TextureHandle tex, const void* pixels, int pitch);

    // Blit a sub-rect of a texture to a destination rect with a color tint.
    void blit(TextureHandle tex, SDL_Rect src, SDL_Rect dst, Color tint);

    void set_clip(SDL_Rect r);
    void clear_clip();

    void end_frame();

    SDL_Renderer* raw() const { return r_; }

private:
    SDL_Renderer* r_;
};

} // namespace sprawn
