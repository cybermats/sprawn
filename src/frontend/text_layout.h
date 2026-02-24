#pragma once

#include "font.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace sprawn::detail {

struct LineEntry {
    std::size_t offset; // byte offset into the text
    std::size_t length; // byte length
    ShapedRun shaped;   // lazily filled
    bool is_shaped = false;
};

class TextLayout {
public:
    explicit TextLayout(const Font& font);

    void set_text(std::string_view utf8_text);
    void set_viewport(int viewport_height);

    std::size_t line_count() const { return lines_.size(); }
    int line_height() const;
    int total_height() const;

    // Scroll position in pixels.
    int scroll_y() const { return scroll_y_; }
    void set_scroll_y(int y);
    void scroll_by(int delta);
    void scroll_to_line(std::size_t line);

    // Returns range of visible lines [first, last) — shapes them if needed.
    std::size_t first_visible_line() const;
    std::size_t last_visible_line() const;

    // Access a shaped line. Must be in visible range (will shape on demand).
    const ShapedRun& get_shaped_line(std::size_t line_index);

    std::string_view text() const { return text_; }

private:
    void split_lines();
    void ensure_shaped(std::size_t line_index);

    const Font& font_;
    std::string text_;
    std::vector<LineEntry> lines_;
    int viewport_height_ = 0;
    int scroll_y_ = 0;
};

} // namespace sprawn::detail
