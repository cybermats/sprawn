#pragma once

#include <cstddef>
#include <string_view>

namespace sprawn {

class Document {
public:
    virtual ~Document() = default;

    virtual std::size_t line_count() const = 0;
    virtual std::string_view line(std::size_t index) const = 0;
    virtual std::size_t size_bytes() const = 0;
    virtual bool is_fully_loaded() const = 0;
};

} // namespace sprawn
