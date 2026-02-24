#pragma once

#include <cstddef>
#include <string>

namespace sprawn {

struct SourceInfo {
    std::string name;
    std::size_t size_bytes = 0;
    bool seekable = false;
};

class Source {
public:
    virtual ~Source() = default;

    // Read up to max bytes into buf. Returns number of bytes actually read.
    virtual std::size_t read(char* buf, std::size_t max) = 0;

    // Returns true when there is no more data to read.
    virtual bool at_end() const = 0;

    // Metadata about this source.
    virtual SourceInfo info() const = 0;
};

} // namespace sprawn
