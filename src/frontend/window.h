#pragma once

#include <SDL2/SDL.h>
#include <string>

namespace sprawn::detail {

class Window {
public:
    Window(const std::string& title, int width, int height);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    SDL_Renderer* renderer() const { return renderer_; }
    SDL_Window* window() const { return window_; }

    int width() const { return width_; }
    int height() const { return height_; }

    // Call after SDL_WINDOWEVENT_RESIZED.
    void update_size();

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};

} // namespace sprawn::detail
