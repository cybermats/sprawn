#pragma once

#include "font.h"
#include "glyph_cache.h"
#include "text_layout.h"

#include <SDL2/SDL.h>

namespace sprawn::detail {

class Renderer {
public:
    Renderer(SDL_Renderer* sdl_renderer, GlyphCache& cache, const Font& font);

    void draw_frame(TextLayout& layout, int viewport_width, int viewport_height);

    // Text color (default: light gray).
    void set_text_color(uint8_t r, uint8_t g, uint8_t b);

    // Background color (default: dark).
    void set_bg_color(uint8_t r, uint8_t g, uint8_t b);

private:
    void draw_run(const ShapedRun& run, int x, int y);
    void draw_scrollbar(const TextLayout& layout, int viewport_height);

    SDL_Renderer* renderer_;
    GlyphCache& cache_;
    const Font& font_;

    uint8_t text_r_ = 0xE0, text_g_ = 0xE0, text_b_ = 0xE0;
    uint8_t bg_r_ = 0x1E, bg_g_ = 0x1E, bg_b_ = 0x2E;
    int left_margin_ = 8;
};

} // namespace sprawn::detail
