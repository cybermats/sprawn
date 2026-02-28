#pragma once

#include <sprawn/document.h>

#include "mapped_file.h"

#include <memory>
#include <vector>

namespace sprawn::detail {

class MmapDocument : public Document {
public:
    explicit MmapDocument(std::unique_ptr<MappedFile> file);

    std::size_t line_count() const override;
    std::string_view line(std::size_t index) const override;
    std::size_t size_bytes() const override;
    bool is_fully_loaded() const override;

private:
    void build_line_index();

    std::unique_ptr<MappedFile> file_;
    std::vector<std::size_t> line_offsets_;
};

} // namespace sprawn::detail
