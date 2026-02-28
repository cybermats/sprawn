#pragma once

#include <functional>
#include <SDL2/SDL.h>

namespace sprawn {

class Window {
public:
    Window(const char* title, int width, int height);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // Poll pending events. Returns false if a quit event was received.
    bool poll_events(const std::function<void(const SDL_Event&)>& handler);

    void present();

    SDL_Renderer* sdl_renderer() const { return renderer_; }
    int width_px()  const { return width_;  }
    int height_px() const { return height_; }
    float dpi_scale() const { return dpi_scale_; }

private:
    SDL_Window*   window_{};
    SDL_Renderer* renderer_{};
    int   width_{};
    int   height_{};
    float dpi_scale_{1.0f};
};

} // namespace sprawn
