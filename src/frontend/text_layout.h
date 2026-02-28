#pragma once

#include <sprawn/document.h>

#include "font.h"

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace sprawn::detail {

class TextLayout {
public:
    explicit TextLayout(const Font& font);

    // Set a Document to read lines from on demand (zero-copy path).
    void set_document(const Document* doc);

    // Legacy path: owns a copy of the text, splits into lines internally.
    void set_text(std::string_view utf8_text);

    void set_viewport(int viewport_height);

    std::size_t line_count() const;
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

private:
    void ensure_shaped(std::size_t line_index);
    void evict_distant_lines();

    const Font& font_;

    // Document-backed mode (preferred).
    const Document* doc_ = nullptr;

    // Owned-text mode (legacy/set_text path).
    std::unique_ptr<Document> owned_doc_;

    // Cache of shaped runs, keyed by line index.
    // Only viewport-nearby lines are kept; distant ones are evicted.
    std::map<std::size_t, ShapedRun> shaped_cache_;
    std::size_t last_evict_first_ = 0;
    std::size_t last_evict_last_ = 0;

    int viewport_height_ = 0;
    int scroll_y_ = 0;
};

} // namespace sprawn::detail
