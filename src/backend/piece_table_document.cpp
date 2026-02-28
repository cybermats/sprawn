#include "piece_table_document.h"

#include <cassert>

namespace sprawn::detail {

PieceTableDocument::PieceTableDocument(std::shared_ptr<PieceTable> table)
    : table_(std::move(table))
{
    rebuild_line_index();
}

void PieceTableDocument::mark_fully_loaded() {
    fully_loaded_ = true;
}

std::size_t PieceTableDocument::line_count() const {
    return line_offsets_.empty() ? 0 : line_offsets_.size() - 1;
}

std::string_view PieceTableDocument::line(std::size_t index) const {
    assert(index < line_count() && "line index out of range");
    const auto& orig = table_->original_buffer();
    std::size_t start = line_offsets_[index];
    std::size_t end = line_offsets_[index + 1];

    // Strip trailing \n if present.
    std::size_t len = (end > start && orig[end - 1] == '\n')
                          ? end - start - 1
                          : end - start;
    return {orig.data() + start, len};
}

std::size_t PieceTableDocument::size_bytes() const {
    return table_->total_length();
}

bool PieceTableDocument::is_fully_loaded() const {
    return fully_loaded_;
}

void PieceTableDocument::rebuild_line_index() {
    const auto& orig = table_->original_buffer();
    line_offsets_.clear();

    if (orig.empty()) return;

    line_offsets_.push_back(0);
    for (std::size_t i = 0; i < orig.size(); ++i) {
        if (orig[i] == '\n') {
            line_offsets_.push_back(i + 1);
        }
    }

    // If the file does NOT end with '\n', the final line has no sentinel yet.
    if (orig.back() != '\n') {
        line_offsets_.push_back(orig.size());
    }
    // If the file DOES end with '\n', the loop already pushed orig.size()
    // as the sentinel (e.g. "hello\n" -> {0, 6}), and line_count()=1.
}

} // namespace sprawn::detail
