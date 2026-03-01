#include "piece_table.h"

#include <algorithm>
#include <stdexcept>

namespace sprawn {

PieceTable::PieceTable(std::span<const std::byte> original)
    : original_(original)
    , total_length_(original.size())
{
    if (!original.empty()) {
        pieces_.push_back({Buffer::original, 0, original.size()});
    }
}

const char* PieceTable::buffer_data(Buffer buf) const {
    if (buf == Buffer::original) {
        return reinterpret_cast<const char*>(original_.data());
    }
    return add_buffer_.data();
}

PieceTable::PieceLocation PieceTable::find_piece(size_t pos) const {
    size_t offset = 0;
    for (size_t i = 0; i < pieces_.size(); ++i) {
        if (pos <= offset + pieces_[i].length) {
            return {i, pos - offset};
        }
        offset += pieces_[i].length;
    }
    return {pieces_.size(), 0};
}

void PieceTable::split_piece(size_t piece_index, size_t offset_in_piece) {
    if (offset_in_piece == 0 || offset_in_piece == pieces_[piece_index].length) {
        return;
    }

    auto& piece = pieces_[piece_index];
    Piece right{piece.buffer, piece.offset + offset_in_piece,
                piece.length - offset_in_piece};
    piece.length = offset_in_piece;
    pieces_.insert(pieces_.begin() + static_cast<ptrdiff_t>(piece_index) + 1, right);
}

void PieceTable::insert(size_t pos, std::string_view text) {
    if (text.empty()) return;
    if (pos > total_length_) {
        throw std::out_of_range("insert position out of range");
    }

    size_t add_offset = add_buffer_.size();
    add_buffer_.append(text);

    Piece new_piece{Buffer::add, add_offset, text.size()};

    if (pieces_.empty()) {
        pieces_.push_back(new_piece);
    } else {
        auto [pi, off] = find_piece(pos);

        if (pi == pieces_.size()) {
            // Insert at end
            pieces_.push_back(new_piece);
        } else if (off == 0) {
            // Insert before this piece
            pieces_.insert(pieces_.begin() + static_cast<ptrdiff_t>(pi), new_piece);
        } else if (off == pieces_[pi].length) {
            // Insert after this piece
            pieces_.insert(pieces_.begin() + static_cast<ptrdiff_t>(pi) + 1, new_piece);
        } else {
            // Split the piece and insert in the middle
            split_piece(pi, off);
            pieces_.insert(pieces_.begin() + static_cast<ptrdiff_t>(pi) + 1, new_piece);
        }
    }

    total_length_ += text.size();
}

void PieceTable::erase(size_t pos, size_t count) {
    if (count == 0) return;
    if (pos > total_length_ || count > total_length_ - pos) {
        throw std::out_of_range("erase range out of bounds");
    }

    auto [start_pi, start_off] = find_piece(pos);

    // Split at start if needed
    if (start_off > 0) {
        split_piece(start_pi, start_off);
        start_pi++;
        start_off = 0;
    }

    // Remove whole pieces and trim the last one
    size_t remaining = count;
    while (remaining > 0 && start_pi < pieces_.size()) {
        auto& piece = pieces_[start_pi];
        if (piece.length <= remaining) {
            remaining -= piece.length;
            pieces_.erase(pieces_.begin() + static_cast<ptrdiff_t>(start_pi));
        } else {
            piece.offset += remaining;
            piece.length -= remaining;
            remaining = 0;
        }
    }

    total_length_ -= count;
}

std::string PieceTable::text() const {
    std::string result;
    result.reserve(total_length_);
    for (const auto& piece : pieces_) {
        const char* base = buffer_data(piece.buffer);
        result.append(base + piece.offset, piece.length);
    }
    return result;
}

std::string PieceTable::text(size_t pos, size_t count) const {
    if (count > total_length_ || pos > total_length_ - count) {
        count = total_length_ > pos ? total_length_ - pos : 0;
    }
    if (count == 0) return {};

    std::string result;
    result.reserve(count);

    auto [pi, off] = find_piece(pos);
    size_t remaining = count;

    while (remaining > 0 && pi < pieces_.size()) {
        const auto& piece = pieces_[pi];
        const char* base = buffer_data(piece.buffer);
        size_t avail = piece.length - off;
        size_t take = std::min(avail, remaining);
        result.append(base + piece.offset + off, take);
        remaining -= take;
        off = 0;
        ++pi;
    }

    return result;
}

size_t PieceTable::length() const {
    return total_length_;
}

} // namespace sprawn
