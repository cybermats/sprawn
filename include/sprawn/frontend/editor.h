#pragma once

#include "events.h"
#include "input_handler.h"
#include "line_cache.h"
#include "renderer.h"
#include "text_layout.h"
#include "viewport.h"
#include <sprawn/document.h>

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
    Editor(Document& doc, Renderer& renderer,
           FontChain& fonts, GlyphAtlas& atlas,
           int width_px, int height_px);

    void handle_event(const SDL_Event& ev);
    void render();
    void on_resize(int w, int h);

private:
    void apply_command(const EditorCommand& cmd);
    void render_cursor(int y, const GlyphRun& run, std::string_view utf8);

    // Selection helpers
    bool has_selection() const;
    std::pair<CursorPos, CursorPos> selection_range() const;
    std::string selected_text() const;
    void delete_selection();

    Document&     doc_;
    Renderer&     renderer_;
    GlyphAtlas&   atlas_;
    TextLayout    layout_;
    Viewport      viewport_;
    LineCache     line_cache_;
    InputHandler  input_;
    CursorPos     cursor_;
    SelectAnchor  anchor_;
    int           gutter_width_{0};

    static constexpr int kGutterPad  = 8;
    static constexpr int kFontSize   = 16;
};

} // namespace sprawn
