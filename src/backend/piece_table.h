#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sprawn {

class PieceTable {
public:
    enum class Buffer : uint8_t { original, add };

    struct Piece {
        Buffer buffer;
        size_t offset;
        size_t length;
    };

    PieceTable() = default;
    explicit PieceTable(std::span<const std::byte> original);

    void insert(size_t pos, std::string_view text);
    void erase(size_t pos, size_t count);

    std::string text() const;
    std::string text(size_t pos, size_t count) const;
    size_t length() const;

    const std::vector<Piece>& pieces() const { return pieces_; }
    const char* buffer_data(Buffer buf) const;

private:
    struct PieceLocation {
        size_t piece_index;
        size_t offset_in_piece;
    };

    PieceLocation find_piece(size_t pos) const;
    void split_piece(size_t piece_index, size_t offset_in_piece);

    // Non-owning view into the memory-mapped file data.
    // Must remain valid for the lifetime of this PieceTable.
    std::span<const std::byte> original_;
    std::string add_buffer_;
    std::vector<Piece> pieces_;
    size_t total_length_ = 0;
};

} // namespace sprawn
