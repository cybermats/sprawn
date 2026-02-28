#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace sprawn::detail {

class PieceTable {
public:
    enum class BufferId { Original, Add };

    struct Piece {
        BufferId buffer;
        std::size_t offset;
        std::size_t length;
    };

    explicit PieceTable(std::string original);

    std::size_t total_length() const;
    const std::vector<Piece>& pieces() const;
    const std::string& original_buffer() const;

    /// Returns the data for a single piece.
    std::string_view piece_data(const Piece& p) const;

    /// Extracts a range from the logical content.
    /// For the single-piece case this is O(1); multi-piece is O(pieces).
    std::string_view substr(std::size_t offset, std::size_t length) const;

private:
    std::string original_;
    std::string add_;
    std::vector<Piece> pieces_;
};

} // namespace sprawn::detail
