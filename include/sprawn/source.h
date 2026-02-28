#pragma once

#include <cstddef>
#include <span>

namespace sprawn {

class Source {
public:
    virtual ~Source() = default;
    virtual std::span<const std::byte> data() const = 0;
};

} // namespace sprawn
