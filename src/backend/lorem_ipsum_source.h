#pragma once

#include <sprawn/source.h>

#include <cstddef>
#include <string>

namespace sprawn::detail {

class LoremIpsumSource : public Source {
public:
    explicit LoremIpsumSource(std::size_t num_lines);

    std::size_t read(char* buf, std::size_t max) override;
    bool at_end() const override;
    SourceInfo info() const override;

private:
    std::size_t num_lines_;
    std::size_t current_line_ = 0;
    std::string pending_;     // partially consumed line data
    std::size_t pending_pos_ = 0;
};

} // namespace sprawn::detail
