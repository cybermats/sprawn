#include <sprawn/frontend/editor.h>
#include <sprawn/decoration.h>
#include <sprawn/frontend/decoration_compositor.h>
#include "font_chain.h"
#include "glyph_atlas.h"

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

// Count UTF-8 characters (codepoints) in s.
size_t utf8_char_count(std::string_view s) {
    size_t n = 0;
    for (size_t i = 0; i < s.size(); ) {
        unsigned char b = static_cast<unsigned char>(s[i]);
        if      (b < 0x80) i += 1;
        else if (b < 0xE0) i += 2;
        else if (b < 0xF0) i += 3;
        else               i += 4;
        ++n;
    }
    return n;
}

// Byte offset of the char_idx-th UTF-8 codepoint in s.
size_t utf8_byte_offset(std::string_view s, size_t char_idx) {
    size_t i = 0;
    for (size_t n = 0; n < char_idx && i < s.size(); ++n) {
        unsigned char b = static_cast<unsigned char>(s[i]);
        if      (b < 0x80) i += 1;
        else if (b < 0xE0) i += 2;
        else if (b < 0xF0) i += 3;
        else               i += 4;
    }
    return i;
}

// Byte length of char_count codepoints starting at codepoint char_start.
size_t utf8_byte_count(std::string_view s, size_t char_start, size_t char_count) {
    size_t start_byte = utf8_byte_offset(s, char_start);
    size_t end_byte = utf8_byte_offset(s, char_start + char_count);
    return end_byte - start_byte;
}

// Format a line number into a fixed-width right-aligned string.
std::string format_line_number(size_t n, int width) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%*zu", width, n + 1);
    return buf;
}

} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

Editor::Editor(Controller& ctrl, Renderer& renderer,
               FontChain& fonts, GlyphAtlas& atlas,
               int width_px, int height_px, float dpi_scale)
    : ctrl_(ctrl),
      renderer_(renderer),
      fonts_(fonts),
      atlas_(atlas),
      layout_(atlas, fonts, dpi_scale),
      viewport_(width_px, height_px, layout_.line_height()),
      line_cache_(512),
      dpi_scale_(dpi_scale)
{
    recompute_gutter();
}

void Editor::rebuild_fonts(int logical_size, float scale) {
    font_size_logical_ = logical_size;
    dpi_scale_ = scale;
    int phys = static_cast<int>(logical_size * scale + 0.5f);
    fonts_.rebuild(phys);
    atlas_.clear();
    layout_.reset(scale);
    viewport_.set_line_height(layout_.line_height());
    line_cache_.clear();
    recompute_gutter();
}

void Editor::on_dpi_change(float new_scale) {
    rebuild_fonts(font_size_logical_, new_scale);
}

void Editor::recompute_gutter() {
    size_t lc = ctrl_.line_count();
    int digits = 1;
    for (size_t n = lc > 0 ? lc - 1 : 0; n >= 10; n /= 10) ++digits;
    int logical_adv = static_cast<int>(fonts_.advance_width() / dpi_scale_ + 0.5f);
    gutter_width_ = digits * logical_adv + kGutterPad * 2;
}

// ---------------------------------------------------------------------------
// Event handling
// ---------------------------------------------------------------------------

void Editor::handle_event(const SDL_Event& ev) {
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

// ---------------------------------------------------------------------------
// Selection helpers
// ---------------------------------------------------------------------------

bool Editor::has_selection() const {
    return anchor_.active &&
           (anchor_.line != cursor_.line || anchor_.col != cursor_.col);
}

std::pair<CursorPos, CursorPos> Editor::selection_range() const {
    CursorPos a{anchor_.line, anchor_.col};
    CursorPos c = cursor_;
    // Normalize so that start comes before end
    if (a.line < c.line || (a.line == c.line && a.col <= c.col))
        return {a, c};
    return {c, a};
}

std::string Editor::selected_text() const {
    if (!has_selection()) return {};
    auto [start, end] = selection_range();

    if (start.line == end.line) {
        std::string s = ctrl_.line(start.line);
        size_t b0 = utf8_byte_offset(s, start.col);
        size_t b1 = utf8_byte_offset(s, end.col);
        return s.substr(b0, b1 - b0);
    }

    std::string result;
    // First partial line
    std::string first_s = ctrl_.line(start.line);
    result += first_s.substr(utf8_byte_offset(first_s, start.col));
    result += '\n';
    // Middle lines
    for (size_t L = start.line + 1; L < end.line; ++L) {
        result += ctrl_.line(L);
        result += '\n';
    }
    // Last partial line
    std::string last_s = ctrl_.line(end.line);
    result += last_s.substr(0, utf8_byte_offset(last_s, end.col));
    return result;
}

void Editor::delete_selection() {
    if (!has_selection()) return;
    auto [start, end] = selection_range();

    size_t removed_lines = end.line - start.line;

    if (removed_lines == 0) {
        std::string line_text = ctrl_.line(start.line);
        size_t byte_col = utf8_byte_offset(line_text, start.col);
        size_t byte_count = utf8_byte_count(line_text, start.col, end.col - start.col);
        ctrl_.erase(start.line, byte_col, byte_count);
        line_cache_.invalidate(start.line);
    } else {
        // Count total bytes to erase (including newlines)
        std::string first_line = ctrl_.line(start.line);
        size_t first_line_chars = utf8_char_count(first_line);
        size_t clamped_col = std::min(start.col, first_line_chars);
        size_t byte_col = utf8_byte_offset(first_line, clamped_col);
        size_t count = first_line.size() - byte_col + 1; // rest of first line + newline
        for (size_t L = start.line + 1; L < end.line; ++L)
            count += ctrl_.line(L).size() + 1; // full line + newline
        count += utf8_byte_offset(ctrl_.line(end.line), end.col);

        ctrl_.erase(start.line, byte_col, count);
        line_cache_.invalidate(start.line);
        line_cache_.invalidate_range(start.line + 1, removed_lines,
                                     -static_cast<int>(removed_lines));
    }

    cursor_ = start;
    anchor_.active = false;
    viewport_.ensure_line_visible(cursor_.line, ctrl_.line_count());
}

// ---------------------------------------------------------------------------
// Command dispatch
// ---------------------------------------------------------------------------

void Editor::apply_command(const EditorCommand& cmd) {
    std::visit([this](const auto& c) {
        using T = std::decay_t<decltype(c)>;

        // Helper: activate or clear selection anchor based on shift key
        auto handle_shift = [this](bool shift) {
            if (shift) {
                if (!anchor_.active)
                    anchor_ = {cursor_.line, cursor_.col, true};
            } else {
                anchor_.active = false;
            }
        };

        if constexpr (std::is_same_v<T, MoveCursor>) {
            handle_shift(c.shift);
            size_t total = ctrl_.line_count();
            if (total == 0) return;

            if (c.dy != 0) {
                long long new_line = static_cast<long long>(cursor_.line) + c.dy;
                if (new_line < 0) new_line = 0;
                if (new_line >= static_cast<long long>(total))
                    new_line = static_cast<long long>(total) - 1;
                cursor_.line = static_cast<size_t>(new_line);
            }

            if (c.dx != 0) {
                size_t char_count = utf8_char_count(ctrl_.line(cursor_.line));
                long long new_col = static_cast<long long>(cursor_.col) + c.dx;
                if (new_col < 0) new_col = 0;
                if (static_cast<size_t>(new_col) > char_count)
                    new_col = static_cast<long long>(char_count);
                cursor_.col = static_cast<size_t>(new_col);
            }

            viewport_.ensure_line_visible(cursor_.line, ctrl_.line_count());

        } else if constexpr (std::is_same_v<T, MoveHome>) {
            handle_shift(c.shift);
            cursor_.col = 0;

        } else if constexpr (std::is_same_v<T, MoveEnd>) {
            handle_shift(c.shift);
            cursor_.col = utf8_char_count(ctrl_.line(cursor_.line));

        } else if constexpr (std::is_same_v<T, MovePgUp>) {
            handle_shift(c.shift);
            size_t vl = viewport_.visible_lines();
            cursor_.line = cursor_.line > vl ? cursor_.line - vl : 0;
            viewport_.ensure_line_visible(cursor_.line, ctrl_.line_count());

        } else if constexpr (std::is_same_v<T, MovePgDn>) {
            handle_shift(c.shift);
            size_t vl    = viewport_.visible_lines();
            size_t total = ctrl_.line_count();
            cursor_.line = std::min(cursor_.line + vl, total > 0 ? total - 1 : 0);
            viewport_.ensure_line_visible(cursor_.line, ctrl_.line_count());

        } else if constexpr (std::is_same_v<T, ScrollLines>) {
            viewport_.scroll_by(0.0f, c.dy, ctrl_.line_count());

        } else if constexpr (std::is_same_v<T, ClickPosition>) {
            // Determine clicked line
            size_t clicked_line = viewport_.y_to_line(c.y_px);
            size_t total = ctrl_.line_count();
            if (total > 0 && clicked_line >= total)
                clicked_line = total - 1;

            // Determine column from x
            int text_x = c.x_px - gutter_width_ + viewport_.scroll_x_px();
            if (text_x < 0) text_x = 0;

            std::string utf8 = ctrl_.line(clicked_line);
            uint64_t h = fnv1a(utf8);
            const GlyphRun* cached = line_cache_.get(clicked_line, h);
            size_t col;
            if (cached) {
                col = layout_.column_for_x(*cached, utf8, text_x);
            } else {
                GlyphRun run = layout_.shape_line(utf8);
                col = layout_.column_for_x(run, utf8, text_x);
                line_cache_.put(clicked_line, h, std::move(run));
            }

            if (c.shift) {
                if (!anchor_.active)
                    anchor_ = {cursor_.line, cursor_.col, true};
            } else {
                anchor_.active = false;
            }
            cursor_.line = clicked_line;
            cursor_.col  = col;

        } else if constexpr (std::is_same_v<T, InsertText>) {
            if (has_selection()) delete_selection();
            std::string line_text = ctrl_.line(cursor_.line);
            size_t byte_col = utf8_byte_offset(line_text, cursor_.col);
            ctrl_.insert(cursor_.line, byte_col, c.text);
            cursor_.col += utf8_char_count(c.text);
            line_cache_.invalidate(cursor_.line);

        } else if constexpr (std::is_same_v<T, DeleteBackward>) {
            if (has_selection()) {
                delete_selection();
                return;
            }
            if (cursor_.col > 0) {
                std::string line_text = ctrl_.line(cursor_.line);
                size_t byte_col = utf8_byte_offset(line_text, cursor_.col - 1);
                size_t byte_count = utf8_byte_count(line_text, cursor_.col - 1, 1);
                ctrl_.erase(cursor_.line, byte_col, byte_count);
                --cursor_.col;
                line_cache_.invalidate(cursor_.line);
            } else if (cursor_.line > 0) {
                std::string prev_line = ctrl_.line(cursor_.line - 1);
                size_t prev_len = utf8_char_count(prev_line);
                size_t prev_byte_len = prev_line.size();
                ctrl_.erase(cursor_.line - 1, prev_byte_len, 1);
                line_cache_.invalidate_range(cursor_.line - 1, 1, -1);
                --cursor_.line;
                cursor_.col = prev_len;
                recompute_gutter();
                viewport_.ensure_line_visible(cursor_.line, ctrl_.line_count());
            }

        } else if constexpr (std::is_same_v<T, DeleteForward>) {
            if (has_selection()) {
                delete_selection();
                return;
            }
            std::string line_text_del = ctrl_.line(cursor_.line);
            size_t char_count = utf8_char_count(line_text_del);
            if (cursor_.col < char_count) {
                size_t byte_col = utf8_byte_offset(line_text_del, cursor_.col);
                size_t byte_count = utf8_byte_count(line_text_del, cursor_.col, 1);
                ctrl_.erase(cursor_.line, byte_col, byte_count);
                line_cache_.invalidate(cursor_.line);
            } else {
                // Merge with next line (delete the newline)
                ctrl_.erase(cursor_.line, line_text_del.size(), 1);
                line_cache_.invalidate_range(cursor_.line, 1, -1);
                recompute_gutter();
            }

        } else if constexpr (std::is_same_v<T, NewLine>) {
            if (has_selection()) delete_selection();
            {
                std::string nl_line = ctrl_.line(cursor_.line);
                size_t byte_col = utf8_byte_offset(nl_line, cursor_.col);
                ctrl_.insert(cursor_.line, byte_col, "\n");
            }
            line_cache_.invalidate_range(cursor_.line, 0, 1);
            line_cache_.invalidate(cursor_.line + 1);
            ++cursor_.line;
            cursor_.col = 0;
            recompute_gutter();
            viewport_.ensure_line_visible(cursor_.line, ctrl_.line_count());

        } else if constexpr (std::is_same_v<T, Copy>) {
            std::string text = has_selection() ? selected_text()
                                               : ctrl_.line(cursor_.line);
            SDL_SetClipboardText(text.c_str());

        } else if constexpr (std::is_same_v<T, Cut>) {
            if (has_selection()) {
                std::string text = selected_text();
                SDL_SetClipboardText(text.c_str());
                delete_selection();
            } else {
                // Cut entire line content
                std::string s = ctrl_.line(cursor_.line);
                SDL_SetClipboardText(s.c_str());
                if (!s.empty()) {
                    ctrl_.erase(cursor_.line, 0, s.size());
                    cursor_.col = 0;
                    line_cache_.invalidate(cursor_.line);
                }
            }

        } else if constexpr (std::is_same_v<T, Paste>) {
            if (has_selection()) delete_selection();
            char* clipboard = SDL_GetClipboardText();
            if (clipboard) {
                struct Guard { char* p; ~Guard() { SDL_free(p); } } guard{clipboard};
                std::string text(clipboard);
                {
                    std::string paste_line = ctrl_.line(cursor_.line);
                    size_t byte_col = utf8_byte_offset(paste_line, cursor_.col);
                    ctrl_.insert(cursor_.line, byte_col, text);
                }

                // Update cursor past the inserted text
                size_t newlines = 0;
                size_t last_nl  = 0;
                for (size_t i = 0; i < text.size(); ++i) {
                    if (text[i] == '\n') { ++newlines; last_nl = i; }
                }
                if (newlines == 0) {
                    cursor_.col += utf8_char_count(text);
                    line_cache_.invalidate(cursor_.line);
                } else {
                    size_t chars_after = utf8_char_count(
                        std::string_view(text).substr(last_nl + 1));
                    line_cache_.invalidate_range(cursor_.line, 0,
                                                  static_cast<int>(newlines));
                    line_cache_.invalidate(cursor_.line);
                    cursor_.line += newlines;
                    cursor_.col   = chars_after;
                    line_cache_.invalidate(cursor_.line);
                    recompute_gutter();
                }
                viewport_.ensure_line_visible(cursor_.line, ctrl_.line_count());
            }

        } else if constexpr (std::is_same_v<T, SelectAll>) {
            size_t total = ctrl_.line_count();
            if (total == 0) return;
            anchor_ = {0, 0, true};
            cursor_.line = total - 1;
            cursor_.col  = utf8_char_count(ctrl_.line(cursor_.line));

        } else if constexpr (std::is_same_v<T, ZoomFont>) {
            int new_size = font_size_logical_ + c.delta * 2;
            new_size = std::clamp(new_size, 8, 72);
            if (new_size != font_size_logical_)
                rebuild_fonts(new_size, dpi_scale_);

        } else if constexpr (std::is_same_v<T, Quit>) {
            SDL_Event quit{};
            quit.type = SDL_QUIT;
            SDL_PushEvent(&quit);
        }

    }, cmd);
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void Editor::render() {
    renderer_.begin_frame(Color{30, 30, 30, 255});

    size_t total = ctrl_.line_count();
    if (total == 0) {
        renderer_.end_frame();
        return;
    }

    int lh     = layout_.line_height();
    size_t first = viewport_.first_line();
    size_t last  = viewport_.last_line(total);

    // Pre-compute selection range (avoid repeated calls inside the loop)
    bool     sel = has_selection();
    CursorPos sel_start{}, sel_end{};
    if (sel) {
        auto [s, e] = selection_range();
        sel_start = s;
        sel_end   = e;
    }

    // Text clip region (exclude gutter)
    SDL_Rect text_clip{gutter_width_, 0, 32767, 32767};
    renderer_.set_clip(text_clip);

    for (size_t L = first; L < last; ++L) {
        int y      = viewport_.line_to_y(L);
        int text_x = gutter_width_ - viewport_.scroll_x_px();

        // Shape the line (from cache or fresh)
        std::string utf8 = ctrl_.line(L);
        uint64_t    h    = fnv1a(utf8);
        const GlyphRun* run_ptr = line_cache_.get(L, h);
        GlyphRun tmp_run;
        if (!run_ptr) {
            // Lazy shaping: only shape up to visible width + margin
            int shape_limit = viewport_.width_px() - gutter_width_
                            + viewport_.scroll_x_px() + 200;
            tmp_run = layout_.shape_line(utf8, shape_limit);

            // If cursor is on this truncated line, re-shape fully
            if (tmp_run.truncated && L == cursor_.line) {
                tmp_run = layout_.shape_line(utf8);
            }

            line_cache_.put(L, h, tmp_run);
            run_ptr = line_cache_.get(L, h);
            if (!run_ptr) run_ptr = &tmp_run;
        }

        // Draw selection highlight (before text so text renders on top)
        if (sel && L >= sel_start.line && L <= sel_end.line) {
            int x0, x1;
            int win_w = viewport_.width_px();

            if (sel_start.line == sel_end.line) {
                x0 = text_x + layout_.x_for_column(*run_ptr, utf8, sel_start.col);
                x1 = text_x + layout_.x_for_column(*run_ptr, utf8, sel_end.col);
            } else if (L == sel_start.line) {
                x0 = text_x + layout_.x_for_column(*run_ptr, utf8, sel_start.col);
                x1 = win_w;
            } else if (L == sel_end.line) {
                x0 = gutter_width_;
                x1 = text_x + layout_.x_for_column(*run_ptr, utf8, sel_end.col);
            } else {
                x0 = gutter_width_;
                x1 = win_w;
            }

            if (x1 > x0)
                renderer_.fill_rect(Rect{x0, y, x1 - x0, lh},
                                    Color{65, 120, 200, 160});
        }

        // Draw text with decoration styling
        LineDecoration deco = ctrl_.decorations(L);
        auto flat = DecorationCompositor::flatten(deco, static_cast<int>(utf8.size()));
        layout_.draw_run(renderer_, *run_ptr, text_x, y, flat, utf8);

        // Draw cursor if on this line
        if (L == cursor_.line)
            render_cursor(y, *run_ptr, utf8);
    }

    renderer_.clear_clip();

    // Draw gutter (unclipped, on top of text/selection)
    int digits = 1;
    for (size_t n = total > 0 ? total - 1 : 0; n >= 10; n /= 10) ++digits;

    for (size_t L = first; L < last; ++L) {
        int y = viewport_.line_to_y(L);
        renderer_.fill_rect(Rect{0, y, gutter_width_, lh}, Color{40, 40, 40, 255});

        std::string num_str = format_line_number(L, digits);
        GlyphRun num_run = layout_.shape_line(num_str);
        int gx = gutter_width_ - kGutterPad - num_run.total_width;
        layout_.draw_run(renderer_, num_run, gx, y, Color{100, 110, 120, 255});
    }

    renderer_.end_frame();
}

void Editor::render_cursor(int y, const GlyphRun& run, std::string_view utf8) {
    int cursor_x = gutter_width_ - viewport_.scroll_x_px()
                 + layout_.x_for_column(run, utf8, cursor_.col);

    int lh = layout_.line_height();
    renderer_.fill_rect(Rect{cursor_x, y, 2, lh}, Color{220, 220, 220, 220});
}

} // namespace sprawn
