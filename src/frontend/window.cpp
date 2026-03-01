#include <sprawn/frontend/window.h>

#include <stdexcept>
#include <string>

namespace sprawn {

Window::Window(const char* title, int width, int height)
    : width_(width), height_(height)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());

    window_ = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!window_)
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());

    renderer_ = SDL_CreateRenderer(
        window_, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer_)
        throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());

    // Compute initial DPI scale
    update_dpi_scale();

    // Enable text input
    SDL_StartTextInput();
}

Window::~Window() {
    SDL_StopTextInput();
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_)   SDL_DestroyWindow(window_);
    SDL_Quit();
}

bool Window::poll_events(const std::function<void(const SDL_Event&)>& handler) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT)
            return false;
        if (ev.type == SDL_WINDOWEVENT &&
            ev.window.event == SDL_WINDOWEVENT_RESIZED)
        {
            width_  = ev.window.data1;
            height_ = ev.window.data2;
            update_dpi_scale();
        }
        handler(ev);
    }
    return true;
}

bool Window::update_dpi_scale() {
    int draw_w = 0;
    SDL_GetRendererOutputSize(renderer_, &draw_w, nullptr);
    float old = dpi_scale_;
    if (draw_w > 0 && width_ > 0)
        dpi_scale_ = static_cast<float>(draw_w) / static_cast<float>(width_);
    return dpi_scale_ != old;
}

void Window::present() {
    SDL_RenderPresent(renderer_);
}

} // namespace sprawn
