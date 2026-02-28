#include "text_layout.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace sprawn::detail {

// A simple Document that owns a string and provides line-oriented access.
// Used by set_text() for backward compatibility.
class OwnedTextDocument : public Document {
public:
    explicit OwnedTextDocument(std::string text)
        : text_(std::move(text))
    {
        build_line_index();
    }

    std::size_t line_count() const override {
        return line_offsets_.empty() ? 0 : line_offsets_.size() - 1;
    }

    std::string_view line(std::size_t index) const override {
        assert(index < line_count());
        std::size_t start = line_offsets_[index];
        std::size_t end = line_offsets_[index + 1];
        // Strip trailing \n.
        if (end > start && text_[end - 1] == '\n') --end;
        return {text_.data() + start, end - start};
    }

    std::size_t size_bytes() const override { return text_.size(); }
    bool is_fully_loaded() const override { return true; }

private:
    void build_line_index() {
        if (text_.empty()) return;
        line_offsets_.push_back(0);
        for (std::size_t i = 0; i < text_.size(); ++i) {
            if (text_[i] == '\n') {
                line_offsets_.push_back(i + 1);
            }
        }
        if (text_.back() != '\n') {
            line_offsets_.push_back(text_.size());
        }
    }

    std::string text_;
    std::vector<std::size_t> line_offsets_;
};

TextLayout::TextLayout(const Font& font)
    : font_(font)
{
}

void TextLayout::set_document(const Document* doc) {
    doc_ = doc;
    owned_doc_.reset();
    shaped_cache_.clear();
    scroll_y_ = 0;
}

void TextLayout::set_text(std::string_view utf8_text) {
    owned_doc_ = std::make_unique<OwnedTextDocument>(std::string(utf8_text));
    doc_ = owned_doc_.get();
    shaped_cache_.clear();
    scroll_y_ = 0;
}

void TextLayout::set_viewport(int viewport_height) {
    viewport_height_ = viewport_height;
    set_scroll_y(scroll_y_);
}

std::size_t TextLayout::line_count() const {
    return doc_ ? doc_->line_count() : 0;
}

int TextLayout::line_height() const {
    return font_.line_height();
}

int TextLayout::total_height() const {
    return static_cast<int>(line_count()) * line_height();
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
    return std::min(last, line_count());
}

const ShapedRun& TextLayout::get_shaped_line(std::size_t line_index) {
    ensure_shaped(line_index);
    return shaped_cache_[line_index];
}

void TextLayout::ensure_shaped(std::size_t line_index) {
    if (!doc_ || line_index >= line_count()) return;

    // Already cached?
    if (shaped_cache_.count(line_index)) return;

    std::string_view raw = doc_->line(line_index);

    // Strip trailing \r (CRLF files: backend leaves \r, we strip here).
    if (!raw.empty() && raw.back() == '\r') {
        raw.remove_suffix(1);
    }

    shaped_cache_[line_index] = font_.shape(raw);
    evict_distant_lines();
}

void TextLayout::evict_distant_lines() {
    std::size_t first = first_visible_line();
    std::size_t last = last_visible_line();

    // Only evict when viewport has shifted significantly.
    if (first == last_evict_first_ && last == last_evict_last_) return;
    last_evict_first_ = first;
    last_evict_last_ = last;

    // Keep a buffer around the viewport (2x viewport lines on each side).
    std::size_t visible_count = last > first ? last - first : 0;
    std::size_t buffer = visible_count * 2;
    std::size_t keep_first = first > buffer ? first - buffer : 0;
    std::size_t keep_last = last + buffer;

    auto it = shaped_cache_.begin();
    while (it != shaped_cache_.end()) {
        if (it->first < keep_first || it->first >= keep_last) {
            it = shaped_cache_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace sprawn::detail
