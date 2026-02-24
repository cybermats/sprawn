#include "renderer.h"

#include <algorithm>

namespace sprawn::detail {

Renderer::Renderer(SDL_Renderer* sdl_renderer, GlyphCache& cache, const Font& font)
    : renderer_(sdl_renderer), cache_(cache), font_(font)
{
}

void Renderer::set_text_color(uint8_t r, uint8_t g, uint8_t b) {
    text_r_ = r; text_g_ = g; text_b_ = b;
}

void Renderer::set_bg_color(uint8_t r, uint8_t g, uint8_t b) {
    bg_r_ = r; bg_g_ = g; bg_b_ = b;
}

void Renderer::draw_frame(TextLayout& layout, int /*viewport_width*/, int viewport_height) {
    // Clear background.
    SDL_SetRenderDrawColor(renderer_, bg_r_, bg_g_, bg_b_, 0xFF);
    SDL_RenderClear(renderer_);

    int lh = layout.line_height();
    if (lh <= 0) {
        SDL_RenderPresent(renderer_);
        return;
    }

    int ascender = font_.ascender();
    std::size_t first = layout.first_visible_line();
    std::size_t last = layout.last_visible_line();

    for (std::size_t i = first; i < last; ++i) {
        const ShapedRun& run = layout.get_shaped_line(i);
        int y = static_cast<int>(i) * lh - layout.scroll_y() + ascender;
        draw_run(run, left_margin_, y);
    }

    draw_scrollbar(layout, viewport_height);

    SDL_RenderPresent(renderer_);
}

void Renderer::draw_run(const ShapedRun& run, int x, int y) {
    int pen_x = x;

    for (const auto& sg : run.glyphs) {
        const CachedGlyph& cg = cache_.get(sg.font_index, sg.glyph_id);
        if (cg.texture) {
            SDL_SetTextureColorMod(cg.texture, text_r_, text_g_, text_b_);

            SDL_Rect dst;
            dst.x = pen_x + sg.x_offset + cg.bearing_x;
            dst.y = y - cg.bearing_y + sg.y_offset;
            dst.w = cg.width;
            dst.h = cg.height;

            SDL_RenderCopy(renderer_, cg.texture, nullptr, &dst);
        }
        pen_x += sg.x_advance;
    }
}

void Renderer::draw_scrollbar(const TextLayout& layout, int viewport_height) {
    int total = layout.total_height();
    if (total <= viewport_height || viewport_height <= 0) return;

    int bar_width = 6;
    float ratio = static_cast<float>(viewport_height) / static_cast<float>(total);
    int bar_height = std::max(20, static_cast<int>(ratio * viewport_height));
    float scroll_ratio = static_cast<float>(layout.scroll_y()) /
                         static_cast<float>(total - viewport_height);
    int bar_y = static_cast<int>(scroll_ratio * (viewport_height - bar_height));

    int window_w = 0;
    SDL_GetRendererOutputSize(renderer_, &window_w, nullptr);

    SDL_SetRenderDrawColor(renderer_, 0x80, 0x80, 0x80, 0xA0);
    SDL_Rect bar_rect = {window_w - bar_width - 2, bar_y, bar_width, bar_height};
    SDL_RenderFillRect(renderer_, &bar_rect);
}

} // namespace sprawn::detail
