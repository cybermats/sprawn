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
class FontFace;

struct CursorPos {
    size_t line{0};
    size_t col{0};
};

class Editor {
public:
    Editor(Document& doc, Renderer& renderer,
           FontFace& font, GlyphAtlas& atlas,
           int width_px, int height_px);

    void handle_event(const SDL_Event& ev);
    void render();
    void on_resize(int w, int h);

private:
    void apply_command(const EditorCommand& cmd);
    void render_gutter(size_t line, int y);
    void render_cursor(int y, const GlyphRun& run);

    Document&     doc_;
    Renderer&     renderer_;
    GlyphAtlas&   atlas_;
    TextLayout    layout_;
    Viewport      viewport_;
    LineCache     line_cache_;
    InputHandler  input_;
    CursorPos     cursor_;
    int           gutter_width_{0};

    static constexpr int kGutterPad  = 8;
    static constexpr int kFontSize   = 16;
};

} // namespace sprawn
