#include "piece_table.h"

#include <cassert>

namespace sprawn::detail {

PieceTable::PieceTable(std::string original)
    : original_(std::move(original))
{
    if (!original_.empty()) {
        pieces_.push_back({BufferId::Original, 0, original_.size()});
    }
}

std::size_t PieceTable::total_length() const {
    std::size_t len = 0;
    for (const auto& p : pieces_) {
        len += p.length;
    }
    return len;
}

const std::vector<PieceTable::Piece>& PieceTable::pieces() const {
    return pieces_;
}

const std::string& PieceTable::original_buffer() const {
    return original_;
}

std::string_view PieceTable::piece_data(const Piece& p) const {
    const std::string& buf = (p.buffer == BufferId::Original) ? original_ : add_;
    return {buf.data() + p.offset, p.length};
}

std::string_view PieceTable::substr(std::size_t offset, std::size_t length) const {
    // Only supports the single-piece case (before any edits).
    // Multi-piece would need materialization into a separate buffer.
    assert(pieces_.size() <= 1 && "substr does not support multi-piece tables yet");
    return {original_.data() + offset, length};
}

} // namespace sprawn::detail
