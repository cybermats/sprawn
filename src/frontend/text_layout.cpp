#include "text_layout.h"

#include <algorithm>
#include <cmath>

namespace sprawn::detail {

TextLayout::TextLayout(const Font& font)
    : font_(font)
{
}

void TextLayout::set_text(std::string_view utf8_text) {
    text_ = std::string(utf8_text);
    lines_.clear();
    scroll_y_ = 0;
    split_lines();
}

void TextLayout::set_viewport(int viewport_height) {
    viewport_height_ = viewport_height;
    // Re-clamp scroll.
    set_scroll_y(scroll_y_);
}

int TextLayout::line_height() const {
    return font_.line_height();
}

int TextLayout::total_height() const {
    return static_cast<int>(lines_.size()) * line_height();
}

void TextLayout::set_scroll_y(int y) {
    int max_scroll = std::max(0, total_height() - viewport_height_);
    scroll_y_ = std::clamp(y, 0, max_scroll);
}

void TextLayout::scroll_by(int delta) {
    set_scroll_y(scroll_y_ + delta);
}

void TextLayout::scroll_to_line(std::size_t line) {
    set_scroll_y(static_cast<int>(line) * line_height());
}

std::size_t TextLayout::first_visible_line() const {
    if (line_height() == 0) return 0;
    return static_cast<std::size_t>(scroll_y_ / line_height());
}

std::size_t TextLayout::last_visible_line() const {
    if (line_height() == 0) return 0;
    std::size_t last = static_cast<std::size_t>((scroll_y_ + viewport_height_) / line_height()) + 1;
    return std::min(last, lines_.size());
}

const ShapedRun& TextLayout::get_shaped_line(std::size_t line_index) {
    ensure_shaped(line_index);
    return lines_[line_index].shaped;
}

void TextLayout::split_lines() {
    std::size_t pos = 0;
    std::size_t len = text_.size();

    while (pos <= len) {
        std::size_t nl = text_.find('\n', pos);
        if (nl == std::string::npos) {
            // Last line (may be empty if text ends with \n).
            lines_.push_back({pos, len - pos, {}, false});
            break;
        }
        lines_.push_back({pos, nl - pos, {}, false});
        pos = nl + 1;
    }
}

void TextLayout::ensure_shaped(std::size_t line_index) {
    if (line_index >= lines_.size()) return;
    auto& entry = lines_[line_index];
    if (entry.is_shaped) return;

    std::string_view line_text(text_.data() + entry.offset, entry.length);
    entry.shaped = font_.shape(line_text);
    entry.is_shaped = true;
}

} // namespace sprawn::detail
