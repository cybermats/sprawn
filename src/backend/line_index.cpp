#include "line_index.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace sprawn {

void LineIndex::rebuild(const PieceTable& table) {
    line_starts_.clear();
    cr_before_lf_.clear();
    line_starts_.push_back(0);
    total_length_ = table.length();

    // We handle three line ending styles:
    //   \n     (Unix)
    //   \r\n   (Windows) — \r is stripped from line content
    //   \r     (classic Mac) — treated as line terminator when not followed by \n
    char prev_char = '\0';
    size_t global_offset = 0;
    for (const auto& piece : table.pieces()) {
        const char* base = table.buffer_data(piece.buffer);
        const char* data = base + piece.offset;
        size_t len = piece.length;

        for (size_t i = 0; i < len; ++i) {
            if (data[i] == '\n') {
                bool cr = (i > 0) ? (data[i - 1] == '\r') : (prev_char == '\r');
                if (cr) {
                    // This \n is part of a \r\n pair. We already pushed a line
                    // break for the \r — replace it with the correct \r\n break.
                    // The line start is after the \n, not after the \r.
                    line_starts_.back() = global_offset + i + 1;
                    cr_before_lf_.back() = true;
                } else {
                    cr_before_lf_.push_back(false);
                    line_starts_.push_back(global_offset + i + 1);
                }
            } else if (data[i] == '\r') {
                // Tentatively treat as a line break (classic Mac).
                // If followed by \n, the \n handler above will correct it.
                cr_before_lf_.push_back(false);
                line_starts_.push_back(global_offset + i + 1);
            }
            prev_char = data[i];
        }
        global_offset += len;
    }
    // Last line has no terminator
    cr_before_lf_.push_back(false);
}

size_t LineIndex::line_count() const {
    return line_starts_.empty() ? 0 : line_starts_.size();
}

LineIndex::LineSpan LineIndex::line_span(size_t line_number) const {
    if (line_number >= line_starts_.size()) {
        throw std::out_of_range("line number out of range");
    }

    size_t start = line_starts_[line_number];
    size_t end;
    if (line_number + 1 < line_starts_.size()) {
        // End is just before the '\n' (and '\r' if \r\n) of this line
        end = line_starts_[line_number + 1] - 1;
        if (cr_before_lf_[line_number] && end > start) {
            end -= 1;  // strip the \r
        }
    } else {
        end = total_length_;
    }

    return {start, end - start};
}

size_t LineIndex::to_offset(size_t line, size_t col) const {
    if (line >= line_starts_.size()) {
        throw std::out_of_range("line number out of range");
    }
    auto span = line_span(line);
    if (col > span.length) {
        throw std::out_of_range("column out of range");
    }
    return line_starts_[line] + col;
}

} // namespace sprawn
