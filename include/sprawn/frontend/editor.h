#pragma once

#include "events.h"
#include "input_handler.h"
#include "line_cache.h"
#include "renderer.h"
#include "text_layout.h"
#include "viewport.h"
#include <sprawn/middleware/controller.h>

#include <SDL2/SDL.h>
#include <cstddef>

namespace sprawn {

class GlyphAtlas;
class FontChain;

struct CursorPos {
    size_t line{0};
    size_t col{0};
};

struct SelectAnchor {
    size_t line{0};
    size_t col{0};
    bool   active{false};
};

class Editor {
public:
    Editor(Controller& ctrl, Renderer& renderer,
           FontChain& fonts, GlyphAtlas& atlas,
           int width_px, int height_px, float dpi_scale = 1.0f);

    void handle_event(const SDL_Event& ev);
    void render();
    void on_resize(int w, int h);
    void on_dpi_change(float new_scale);

private:
    void apply_command(const EditorCommand& cmd);
    void render_cursor(int y, const GlyphRun& run, std::string_view utf8);
    void rebuild_fonts(int logical_size, float scale);
    void recompute_gutter();

    // Selection helpers
    bool has_selection() const;
    std::pair<CursorPos, CursorPos> selection_range() const;
    std::string selected_text() const;
    void delete_selection();

    Controller&   ctrl_;
    Renderer&     renderer_;
    FontChain&    fonts_;
    GlyphAtlas&   atlas_;
    TextLayout    layout_;
    Viewport      viewport_;
    LineCache     line_cache_;
    InputHandler  input_;
    CursorPos     cursor_;
    SelectAnchor  anchor_;
    int           gutter_width_{0};
    float         dpi_scale_{1.0f};
    int           font_size_logical_{16};

    static constexpr int kGutterPad  = 8;
};

} // namespace sprawn
