#include <sprawn/frontend/editor.h>
#include "glyph_atlas.h"
#include "font_face.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

namespace sprawn {

namespace {

// FNV-1a 64-bit hash
uint64_t fnv1a(std::string_view s) {
    uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}

// Format a line number into a fixed-width right-aligned string.
std::string format_line_number(size_t n, int width) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%*zu", width, n + 1);
    return buf;
}

} // namespace

Editor::Editor(Document& doc, Renderer& renderer,
               FontFace& font, GlyphAtlas& atlas,
               int width_px, int height_px)
    : doc_(doc),
      renderer_(renderer),
      atlas_(atlas),
      layout_(atlas, font.line_height(), font.ascent()),
      viewport_(width_px, height_px, font.line_height()),
      line_cache_(512)
{
    // Pre-compute gutter width based on digit count of longest line number
    size_t lc = doc_.line_count();
    int digits = 1;
    for (size_t n = lc > 0 ? lc - 1 : 0; n >= 10; n /= 10) ++digits;

    // Each digit is advance_width wide; add padding on both sides
    gutter_width_ = digits * font.advance_width() + kGutterPad * 2;
}

void Editor::handle_event(const SDL_Event& ev) {
    // Window resize
    if (ev.type == SDL_WINDOWEVENT &&
        ev.window.event == SDL_WINDOWEVENT_RESIZED)
    {
        on_resize(ev.window.data1, ev.window.data2);
        return;
    }

    auto cmd = input_.translate(ev);
    if (cmd) apply_command(*cmd);
}

void Editor::on_resize(int w, int h) {
    viewport_.resize(w, h);
}

void Editor::apply_command(const EditorCommand& cmd) {
    std::visit([this](const auto& c) {
        using T = std::decay_t<decltype(c)>;

        if constexpr (std::is_same_v<T, MoveCursor>) {
            size_t total = doc_.line_count();
            if (total == 0) return;

            // Vertical
            if (c.dy != 0) {
                long long new_line = static_cast<long long>(cursor_.line) + c.dy;
                if (new_line < 0) new_line = 0;
                if (new_line >= static_cast<long long>(total))
                    new_line = static_cast<long long>(total) - 1;
                cursor_.line = static_cast<size_t>(new_line);
            }

            // Horizontal
            if (c.dx != 0) {
                std::string line_str = doc_.line(cursor_.line);
                // Count UTF-8 characters (simplified)
                size_t char_count = 0;
                for (size_t i = 0; i < line_str.size(); ) {
                    unsigned char byte = static_cast<unsigned char>(line_str[i]);
                    if (byte < 0x80) i += 1;
                    else if (byte < 0xE0) i += 2;
                    else if (byte < 0xF0) i += 3;
                    else i += 4;
                    ++char_count;
                }
                long long new_col = static_cast<long long>(cursor_.col) + c.dx;
                if (new_col < 0) new_col = 0;
                if (static_cast<size_t>(new_col) > char_count)
                    new_col = static_cast<long long>(char_count);
                cursor_.col = static_cast<size_t>(new_col);
            }

            viewport_.ensure_line_visible(cursor_.line, doc_.line_count());

        } else if constexpr (std::is_same_v<T, MoveHome>) {
            cursor_.col = 0;

        } else if constexpr (std::is_same_v<T, MoveEnd>) {
            std::string s = doc_.line(cursor_.line);
            // Count characters
            size_t n = 0;
            for (size_t i = 0; i < s.size(); ) {
                unsigned char b = static_cast<unsigned char>(s[i]);
                if (b < 0x80) i += 1;
                else if (b < 0xE0) i += 2;
                else if (b < 0xF0) i += 3;
                else i += 4;
                ++n;
            }
            cursor_.col = n;

        } else if constexpr (std::is_same_v<T, MovePgUp>) {
            size_t vl = viewport_.visible_lines();
            cursor_.line = cursor_.line > vl ? cursor_.line - vl : 0;
            viewport_.ensure_line_visible(cursor_.line, doc_.line_count());

        } else if constexpr (std::is_same_v<T, MovePgDn>) {
            size_t vl    = viewport_.visible_lines();
            size_t total = doc_.line_count();
            cursor_.line = std::min(cursor_.line + vl, total > 0 ? total - 1 : 0);
            viewport_.ensure_line_visible(cursor_.line, doc_.line_count());

        } else if constexpr (std::is_same_v<T, ScrollLines>) {
            viewport_.scroll_by(0.0f, c.dy, doc_.line_count());

        } else if constexpr (std::is_same_v<T, ClickPosition>) {
            // Determine line from y
            size_t clicked_line = viewport_.y_to_line(c.y_px);
            size_t total = doc_.line_count();
            if (total > 0 && clicked_line >= total)
                clicked_line = total - 1;
            cursor_.line = clicked_line;

            // Determine column from x (subtract gutter)
            int text_x = c.x_px - gutter_width_ + viewport_.scroll_x_px();
            if (text_x < 0) text_x = 0;

            std::string utf8 = doc_.line(cursor_.line);
            uint64_t    h    = fnv1a(utf8);
            const GlyphRun* cached = line_cache_.get(cursor_.line, h);
            if (cached) {
                cursor_.col = layout_.column_for_x(*cached, text_x);
            } else {
                GlyphRun run = layout_.shape_line(utf8);
                cursor_.col  = layout_.column_for_x(run, text_x);
                line_cache_.put(cursor_.line, h, std::move(run));
            }

        } else if constexpr (std::is_same_v<T, InsertText>) {
            doc_.insert(cursor_.line, cursor_.col, c.text);
            // Advance cursor by the number of characters inserted
            size_t n = 0;
            for (size_t i = 0; i < c.text.size(); ) {
                unsigned char b = static_cast<unsigned char>(c.text[i]);
                if (b < 0x80) i += 1;
                else if (b < 0xE0) i += 2;
                else if (b < 0xF0) i += 3;
                else i += 4;
                ++n;
            }
            cursor_.col += n;
            line_cache_.invalidate(cursor_.line);

        } else if constexpr (std::is_same_v<T, DeleteBackward>) {
            if (cursor_.col > 0) {
                doc_.erase(cursor_.line, cursor_.col - 1, 1);
                --cursor_.col;
                line_cache_.invalidate(cursor_.line);
            } else if (cursor_.line > 0) {
                // Join with previous line
                size_t prev_len = 0;
                {
                    std::string prev = doc_.line(cursor_.line - 1);
                    for (size_t i = 0; i < prev.size(); ) {
                        unsigned char b = static_cast<unsigned char>(prev[i]);
                        if (b < 0x80) i += 1;
                        else if (b < 0xE0) i += 2;
                        else if (b < 0xF0) i += 3;
                        else i += 4;
                        ++prev_len;
                    }
                }
                // Erase the newline at end of previous line
                doc_.erase(cursor_.line - 1, prev_len, 1);
                line_cache_.invalidate_range(cursor_.line - 1, 1, -1);
                --cursor_.line;
                cursor_.col = prev_len;
                viewport_.ensure_line_visible(cursor_.line, doc_.line_count());
            }

        } else if constexpr (std::is_same_v<T, DeleteForward>) {
            std::string s = doc_.line(cursor_.line);
            size_t char_count = 0;
            for (size_t i = 0; i < s.size(); ) {
                unsigned char b = static_cast<unsigned char>(s[i]);
                if (b < 0x80) i += 1;
                else if (b < 0xE0) i += 2;
                else if (b < 0xF0) i += 3;
                else i += 4;
                ++char_count;
            }
            if (cursor_.col < char_count) {
                doc_.erase(cursor_.line, cursor_.col, 1);
                line_cache_.invalidate(cursor_.line);
            } else {
                // Merge with next line (delete newline)
                doc_.erase(cursor_.line, cursor_.col, 1);
                line_cache_.invalidate_range(cursor_.line, 1, -1);
            }

        } else if constexpr (std::is_same_v<T, NewLine>) {
            doc_.insert(cursor_.line, cursor_.col, "\n");
            line_cache_.invalidate_range(cursor_.line, 0, 1);
            line_cache_.invalidate(cursor_.line + 1);
            ++cursor_.line;
            cursor_.col = 0;
            viewport_.ensure_line_visible(cursor_.line, doc_.line_count());

        } else if constexpr (std::is_same_v<T, Copy>) {
            std::string s = doc_.line(cursor_.line);
            SDL_SetClipboardText(s.c_str());

        } else if constexpr (std::is_same_v<T, Cut>) {
            std::string s = doc_.line(cursor_.line);
            SDL_SetClipboardText(s.c_str());
            // Erase entire line content
            size_t char_count = 0;
            for (size_t i = 0; i < s.size(); ) {
                unsigned char b = static_cast<unsigned char>(s[i]);
                if (b < 0x80) i += 1;
                else if (b < 0xE0) i += 2;
                else if (b < 0xF0) i += 3;
                else i += 4;
                ++char_count;
            }
            if (char_count > 0) {
                doc_.erase(cursor_.line, 0, char_count);
                cursor_.col = 0;
                line_cache_.invalidate(cursor_.line);
            }

        } else if constexpr (std::is_same_v<T, Paste>) {
            char* text = SDL_GetClipboardText();
            if (text) {
                doc_.insert(cursor_.line, cursor_.col, text);
                SDL_free(text);
                line_cache_.invalidate(cursor_.line);
            }

        } else if constexpr (std::is_same_v<T, Quit>) {
            SDL_Event quit{};
            quit.type = SDL_QUIT;
            SDL_PushEvent(&quit);
        }

    }, cmd);
}

void Editor::render() {
    // Background
    renderer_.begin_frame(Color{30, 30, 30, 255});

    size_t total = doc_.line_count();
    if (total == 0) {
        renderer_.end_frame();
        return;
    }

    int lh = layout_.line_height();
    size_t first = viewport_.first_line();
    size_t last  = viewport_.last_line(total);

    // Text clip region (exclude gutter)
    SDL_Rect text_clip{gutter_width_, 0,
                       // We don't know window width here; use a large number
                       32767, 32767};
    renderer_.set_clip(text_clip);

    for (size_t L = first; L < last; ++L) {
        int y = viewport_.line_to_y(L);

        // Fetch and shape the line
        std::string utf8 = doc_.line(L);
        uint64_t    h    = fnv1a(utf8);
        const GlyphRun* run_ptr = line_cache_.get(L, h);
        GlyphRun tmp_run;
        if (!run_ptr) {
            tmp_run = layout_.shape_line(utf8);
            line_cache_.put(L, h, tmp_run);
            run_ptr = line_cache_.get(L, h);
            if (!run_ptr) {
                // Cache refused (full with nullptr) — use tmp_run directly
                run_ptr = &tmp_run;
            }
        }

        // Draw text
        int text_x = gutter_width_ - viewport_.scroll_x_px();
        layout_.draw_run(renderer_, *run_ptr, text_x, y, Color{220, 220, 220, 255});

        // Draw cursor if on this line
        if (L == cursor_.line)
            render_cursor(y, *run_ptr);
    }

    renderer_.clear_clip();

    // Draw gutter (on top, not clipped)
    int digits = 1;
    for (size_t n = total > 0 ? total - 1 : 0; n >= 10; n /= 10) ++digits;

    for (size_t L = first; L < last; ++L) {
        int y = viewport_.line_to_y(L);
        // Gutter background
        renderer_.fill_rect(Rect{0, y, gutter_width_, lh}, Color{40, 40, 40, 255});

        std::string num_str = format_line_number(L, digits);
        GlyphRun num_run = layout_.shape_line(num_str);
        // Right-align: x = gutter_width_ - kGutterPad - total_width
        int gx = gutter_width_ - kGutterPad - num_run.total_width;
        layout_.draw_run(renderer_, num_run, gx, y, Color{100, 110, 120, 255});
    }

    renderer_.end_frame();
}

void Editor::render_cursor(int y, const GlyphRun& run) {
    int cursor_x = gutter_width_ - viewport_.scroll_x_px()
                 + layout_.x_for_column(run, cursor_.col);

    int lh = layout_.line_height();
    // 2-pixel-wide cursor bar
    renderer_.fill_rect(Rect{cursor_x, y, 2, lh}, Color{220, 220, 220, 220});
}

} // namespace sprawn
