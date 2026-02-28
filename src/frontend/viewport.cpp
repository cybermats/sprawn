#include <sprawn/frontend/viewport.h>

#include <algorithm>
#include <cmath>

namespace sprawn {

Viewport::Viewport(int width_px, int height_px, int line_height)
    : width_px_(width_px), height_px_(height_px), line_height_(line_height)
{}

void Viewport::resize(int width_px, int height_px) {
    width_px_  = width_px;
    height_px_ = height_px;
}

void Viewport::set_line_height(int lh) {
    line_height_ = lh;
}

size_t Viewport::visible_lines() const {
    if (line_height_ <= 0) return 1;
    return static_cast<size_t>((height_px_ + line_height_ - 1) / line_height_);
}

size_t Viewport::last_line(size_t total_lines) const {
    if (total_lines == 0) return 0;
    size_t last = first_line_ + visible_lines();
    return last < total_lines ? last : total_lines;
}

int Viewport::line_to_y(size_t line) const {
    if (line < first_line_) return -(static_cast<int>(first_line_ - line) * line_height_);
    return static_cast<int>(line - first_line_) * line_height_;
}

size_t Viewport::y_to_line(int y) const {
    if (y < 0) return first_line_ > 0 ? first_line_ - 1 : 0;
    return first_line_ + static_cast<size_t>(y / line_height_);
}

void Viewport::scroll_by(float dx_px, float dy_lines, size_t total_lines) {
    if (total_lines == 0) return;

    // Vertical scroll
    long long new_first = static_cast<long long>(first_line_)
                        + static_cast<long long>(std::lround(dy_lines));
    if (new_first < 0) new_first = 0;
    size_t max_first = total_lines > visible_lines()
                     ? total_lines - visible_lines()
                     : 0;
    if (static_cast<size_t>(new_first) > max_first)
        new_first = static_cast<long long>(max_first);
    first_line_ = static_cast<size_t>(new_first);

    // Horizontal scroll
    scroll_x_px_ -= static_cast<int>(dx_px);
    if (scroll_x_px_ < 0) scroll_x_px_ = 0;
}

void Viewport::ensure_line_visible(size_t line, size_t total_lines) {
    if (line < first_line_) {
        first_line_ = line;
    } else if (line >= first_line_ + visible_lines()) {
        size_t vl = visible_lines();
        first_line_ = line >= vl ? line - vl + 1 : 0;
    }
    // Clamp
    if (total_lines > 0) {
        size_t max_first = total_lines > visible_lines()
                         ? total_lines - visible_lines()
                         : 0;
        if (first_line_ > max_first) first_line_ = max_first;
    }
}

} // namespace sprawn
