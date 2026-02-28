#pragma once

#include <sprawn/document.h>
#include "piece_table.h"

#include <memory>
#include <vector>

namespace sprawn::detail {

class PieceTableDocument : public Document {
public:
    explicit PieceTableDocument(std::shared_ptr<PieceTable> table);

    void mark_fully_loaded();

    std::size_t line_count() const override;
    std::string_view line(std::size_t index) const override;
    std::size_t size_bytes() const override;
    bool is_fully_loaded() const override;

private:
    void rebuild_line_index();

    std::shared_ptr<PieceTable> table_;
    std::vector<std::size_t> line_offsets_;
    bool fully_loaded_ = false;
};

} // namespace sprawn::detail
