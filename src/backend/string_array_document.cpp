#include "string_array_document.h"

namespace sprawn::detail {

void StringArrayDocument::add_line(std::string line) {
    total_bytes_ += line.size() + 1; // +1 for the newline
    lines_.push_back(std::move(line));
}

void StringArrayDocument::mark_fully_loaded() {
    fully_loaded_ = true;
}

std::size_t StringArrayDocument::line_count() const {
    return lines_.size();
}

std::string_view StringArrayDocument::line(std::size_t index) const {
    return lines_.at(index);
}

std::size_t StringArrayDocument::size_bytes() const {
    return total_bytes_;
}

bool StringArrayDocument::is_fully_loaded() const {
    return fully_loaded_;
}

} // namespace sprawn::detail
