#include "mmap_document.h"

#include <cassert>

namespace sprawn::detail {

MmapDocument::MmapDocument(std::unique_ptr<MappedFile> file)
    : file_(std::move(file))
{
    build_line_index();
}

std::size_t MmapDocument::line_count() const {
    return line_offsets_.empty() ? 0 : line_offsets_.size() - 1;
}

std::string_view MmapDocument::line(std::size_t index) const {
    assert(index < line_count() && "line index out of range");
    std::size_t start = line_offsets_[index];
    std::size_t end = line_offsets_[index + 1];

    // Strip trailing \n if present.
    if (end > start && file_->data()[end - 1] == '\n') {
        --end;
    }
    return {file_->data() + start, end - start};
}

std::size_t MmapDocument::size_bytes() const {
    return file_->size();
}

bool MmapDocument::is_fully_loaded() const {
    return true;
}

void MmapDocument::build_line_index() {
    const char* data = file_->data();
    std::size_t size = file_->size();

    if (size == 0) return;

    line_offsets_.push_back(0);
    for (std::size_t i = 0; i < size; ++i) {
        if (data[i] == '\n') {
            line_offsets_.push_back(i + 1);
        }
    }

    // If the file does NOT end with '\n', add a sentinel for the final line.
    if (data[size - 1] != '\n') {
        line_offsets_.push_back(size);
    }
}

} // namespace sprawn::detail
