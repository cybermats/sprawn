#pragma once

#include <cstddef>

namespace sprawn {

class Viewport {
public:
    Viewport(int width_px, int height_px, int line_height);

    void resize(int width_px, int height_px);
    void set_line_height(int lh);

    size_t first_line() const { return first_line_; }
    size_t last_line(size_t total_lines) const;
    size_t visible_lines() const;

    int scroll_x_px() const { return scroll_x_px_; }

    // Pixel y-coordinate of the top of a given line relative to viewport.
    int line_to_y(size_t line) const;

    // Line index for a pixel y coordinate (clamped).
    size_t y_to_line(int y) const;

    // Scroll by dy lines (positive = down). Clamps to [0, total_lines-1].
    void scroll_by(float dx_px, float dy_lines, size_t total_lines);

    // Ensure a line is visible; scrolls minimally if needed.
    void ensure_line_visible(size_t line, size_t total_lines);

private:
    int    width_px_{}, height_px_{};
    int    line_height_{};
    size_t first_line_{0};
    int    scroll_x_px_{0};
};

} // namespace sprawn
