#include <sprawn/frontend/renderer.h>

#include <stdexcept>
#include <string>

namespace sprawn {

Renderer::Renderer(SDL_Renderer* r) : r_(r) {}

void Renderer::begin_frame(Color bg) {
    SDL_SetRenderDrawColor(r_, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderClear(r_);
}

void Renderer::fill_rect(Rect rect, Color c) {
    SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, c.a);
    SDL_Rect sdlr{rect.x, rect.y, rect.w, rect.h};
    SDL_RenderFillRect(r_, &sdlr);
}

TextureHandle Renderer::create_texture(int w, int h) {
    // Use SDL_PIXELFORMAT_ABGR8888 which maps to {r,g,b,a} bytes in memory
    SDL_Texture* tex = SDL_CreateTexture(
        r_,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        w, h
    );
    if (tex)
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    return tex;
}

void Renderer::update_texture(TextureHandle tex, const void* pixels, int pitch) {
    SDL_UpdateTexture(tex, nullptr, pixels, pitch);
}

void Renderer::blit(TextureHandle tex, SDL_Rect src, SDL_Rect dst, Color tint) {
    SDL_SetTextureColorMod(tex, tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaMod(tex, tint.a);
    SDL_RenderCopy(r_, tex, &src, &dst);
}

void Renderer::set_clip(SDL_Rect r) {
    SDL_RenderSetClipRect(r_, &r);
}

void Renderer::clear_clip() {
    SDL_RenderSetClipRect(r_, nullptr);
}

void Renderer::end_frame() {
    // Nothing needed here; Window::present() calls SDL_RenderPresent.
}

} // namespace sprawn
