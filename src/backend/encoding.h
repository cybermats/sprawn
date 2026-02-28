#pragma once

#include <sprawn/encoding.h>

#include <cstddef>
#include <span>
#include <string>
#include <utility>

namespace sprawn {

// Detect encoding from raw data (BOM check + heuristics)
Encoding detect_encoding(std::span<const std::byte> data);

// Convert raw bytes to UTF-8. For UTF-8 and ASCII this is a no-op view.
// Returns the UTF-8 string and the number of bytes consumed from the BOM (if any).
struct EncodingResult {
    std::span<const std::byte> data;  // data after skipping BOM
    Encoding encoding;
};

EncodingResult skip_bom(std::span<const std::byte> data);

} // namespace sprawn
