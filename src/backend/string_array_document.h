#pragma once

#include <sprawn/document.h>

#include <string>
#include <vector>

namespace sprawn::detail {

class StringArrayDocument : public Document {
public:
    StringArrayDocument() = default;

    void add_line(std::string line);
    void mark_fully_loaded();

    std::size_t line_count() const override;
    std::string_view line(std::size_t index) const override;
    std::size_t size_bytes() const override;
    bool is_fully_loaded() const override;

private:
    std::vector<std::string> lines_;
    std::size_t total_bytes_ = 0;
    bool fully_loaded_ = false;
};

} // namespace sprawn::detail
