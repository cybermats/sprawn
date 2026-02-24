#include <sprawn/frontend.h>

#include "window.h"
#include "font.h"
#include "glyph_cache.h"
#include "text_layout.h"
#include "renderer.h"

#include <SDL2/SDL.h>
#include <stdexcept>
#include <string>

namespace sprawn {

// Find a usable default monospace font on the system.
static std::string find_default_font() {
    const char* candidates[] = {
        // Linux
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSansMono-Regular.ttf",
        "/usr/share/fonts/noto/NotoSansMono-Regular.ttf",
        // macOS
        "/System/Library/Fonts/Menlo.ttc",
        "/System/Library/Fonts/SFMono-Regular.otf",
        "/Library/Fonts/Courier New.ttf",
        // Windows
        "C:\\Windows\\Fonts\\consola.ttf",
        "C:\\Windows\\Fonts\\cour.ttf",
    };

    for (const char* path : candidates) {
        FILE* f = fopen(path, "rb");
        if (f) {
            fclose(f);
            return path;
        }
    }

    throw std::runtime_error(
        "No suitable font found. Please set font_path in FrontendConfig.");
}

// Try to add fallback fonts for broader Unicode coverage (CJK, etc.).
static void add_fallback_fonts(detail::Font& font) {
    const char* fallbacks[] = {
        // Linux — Noto CJK
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/google-noto-cjk/NotoSansCJK-Regular.ttc",
        // Linux — Noto Sans as general fallback
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        // macOS
        "/System/Library/Fonts/PingFang.ttc",
        "/System/Library/Fonts/Hiragino Sans GB.ttc",
        "/Library/Fonts/Arial Unicode MS.ttf",
        // Windows
        "C:\\Windows\\Fonts\\msyh.ttc",
        "C:\\Windows\\Fonts\\yugothm.ttc",
        "C:\\Windows\\Fonts\\malgun.ttf",
    };

    for (const char* path : fallbacks) {
        font.add_fallback(path);
    }
}

struct Frontend::Impl {
    FrontendConfig config;
    std::unique_ptr<detail::Window> window;
    std::unique_ptr<detail::Font> font;
    std::unique_ptr<detail::GlyphCache> glyph_cache;
    std::unique_ptr<detail::TextLayout> layout;
    std::unique_ptr<detail::Renderer> renderer;
    bool running = false;

    void init() {
        window = std::make_unique<detail::Window>(
            config.title, config.window_width, config.window_height);

        std::string font_path = config.font_path.empty()
            ? find_default_font()
            : config.font_path;

        font = std::make_unique<detail::Font>(font_path, config.font_size);
        add_fallback_fonts(*font);
        glyph_cache = std::make_unique<detail::GlyphCache>(
            window->renderer(), *font);
        layout = std::make_unique<detail::TextLayout>(*font);
        layout->set_viewport(window->height());
        renderer = std::make_unique<detail::Renderer>(
            window->renderer(), *glyph_cache, *font);
        running = true;
    }

    bool handle_events() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = false;
                return false;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    window->update_size();
                    layout->set_viewport(window->height());
                }
                break;

            case SDL_MOUSEWHEEL: {
                int delta = event.wheel.y;
                if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                    delta = -delta;
                }
                layout->scroll_by(-delta * font->line_height() * 3);
                break;
            }

            case SDL_KEYDOWN:
                handle_key(event.key.keysym);
                break;
            }
        }
        return running;
    }

    void handle_key(const SDL_Keysym& key) {
        int lh = font->line_height();
        int page = window->height();

        switch (key.sym) {
        case SDLK_UP:
            layout->scroll_by(-lh);
            break;
        case SDLK_DOWN:
            layout->scroll_by(lh);
            break;
        case SDLK_PAGEUP:
            layout->scroll_by(-page);
            break;
        case SDLK_PAGEDOWN:
            layout->scroll_by(page);
            break;
        case SDLK_HOME:
            layout->set_scroll_y(0);
            break;
        case SDLK_END:
            layout->set_scroll_y(layout->total_height());
            break;
        case SDLK_q:
            if (key.mod & KMOD_CTRL) {
                running = false;
            }
            break;
        default:
            break;
        }
    }

    void draw() {
        renderer->draw_frame(*layout, window->width(), window->height());
    }
};

Frontend::Frontend(const FrontendConfig& config)
    : impl_(std::make_unique<Impl>())
{
    impl_->config = config;
    impl_->init();
}

Frontend::~Frontend() = default;
Frontend::Frontend(Frontend&&) noexcept = default;
Frontend& Frontend::operator=(Frontend&&) noexcept = default;

void Frontend::set_text(std::string_view utf8_text) {
    impl_->layout->set_text(utf8_text);
}

void Frontend::scroll_to_line(std::size_t line) {
    impl_->layout->scroll_to_line(line);
}

void Frontend::run() {
    while (impl_->running) {
        if (!process_events()) break;
        render();
    }
}

bool Frontend::process_events() {
    return impl_->handle_events();
}

void Frontend::render() {
    impl_->draw();
}

} // namespace sprawn
