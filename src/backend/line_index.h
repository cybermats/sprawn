#pragma once

#include "piece_table.h"

#include <cstddef>
#include <string_view>
#include <vector>

namespace sprawn {

class LineIndex {
public:
    LineIndex() = default;

    void rebuild(const PieceTable& table);

    size_t line_count() const;

    // Returns {offset, length} of the line (excluding line endings \n and \r\n)
    struct LineSpan {
        size_t offset;
        size_t length;
    };

    LineSpan line_span(size_t line_number) const;

    // Convert (line, col) to absolute byte offset
    size_t to_offset(size_t line, size_t col) const;

private:
    // line_starts_[i] is the byte offset where line i begins
    std::vector<size_t> line_starts_;
    // cr_before_lf_[i] is true if line i ends with \r\n (not just \n)
    std::vector<bool> cr_before_lf_;
    size_t total_length_ = 0;
};

} // namespace sprawn
