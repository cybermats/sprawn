#include "lorem_ipsum_source.h"

#include <array>

namespace sprawn::detail {

static constexpr std::array<const char*, 10> fragments = {{
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit.",
    "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
    "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris.",
    "Duis aute irure dolor in reprehenderit in voluptate velit esse.",
    "Excepteur sint occaecat cupidatat non proident, sunt in culpa.",
    "Curabitur pretium tincidunt lacus, nec vehicula arcu fermentum ut.",
    "Maecenas sed diam eget risus varius blandit sit amet non magna.",
    "Vestibulum ante ipsum primis in faucibus orci luctus et ultrices.",
    "Pellentesque habitant morbi tristique senectus et netus et malesuada.",
    "Integer posuere erat a ante venenatis dapibus posuere velit aliquet.",
}};

LoremIpsumSource::LoremIpsumSource(std::size_t num_lines)
    : num_lines_(num_lines)
{
}

std::size_t LoremIpsumSource::read(char* buf, std::size_t max) {
    std::size_t written = 0;

    while (written < max) {
        // If we have pending data from a partially consumed line, drain it first.
        if (pending_pos_ < pending_.size()) {
            std::size_t avail = pending_.size() - pending_pos_;
            std::size_t to_copy = (avail < max - written) ? avail : (max - written);
            std::copy(pending_.data() + pending_pos_,
                      pending_.data() + pending_pos_ + to_copy,
                      buf + written);
            pending_pos_ += to_copy;
            written += to_copy;
            continue;
        }

        // Generate the next line.
        if (current_line_ >= num_lines_) {
            break;
        }

        pending_.clear();
        pending_pos_ = 0;
        pending_ += std::to_string(current_line_ + 1);
        pending_ += ": ";
        pending_ += fragments[current_line_ % fragments.size()];
        pending_ += '\n';
        ++current_line_;
    }

    return written;
}

bool LoremIpsumSource::at_end() const {
    return current_line_ >= num_lines_ && pending_pos_ >= pending_.size();
}

SourceInfo LoremIpsumSource::info() const {
    return {"lorem_ipsum(" + std::to_string(num_lines_) + " lines)", 0, false};
}

} // namespace sprawn::detail
