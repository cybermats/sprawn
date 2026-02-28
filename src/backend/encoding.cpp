#include "encoding.h"

#include <algorithm>

namespace sprawn {

Encoding detect_encoding(std::span<const std::byte> data) {
    // Check for UTF-8 BOM
    if (data.size() >= 3 &&
        data[0] == std::byte{0xEF} &&
        data[1] == std::byte{0xBB} &&
        data[2] == std::byte{0xBF}) {
        return Encoding::utf8;
    }

    // Heuristic: sample the first 8 KB for non-ASCII bytes
    constexpr size_t sample_size = 8192;
    auto sample = data.subspan(0, std::min(data.size(), sample_size));
    bool all_ascii = true;
    for (auto b : sample) {
        if (static_cast<unsigned char>(b) >= 0x80) {
            all_ascii = false;
            break;
        }
    }

    if (all_ascii) {
        return Encoding::ascii;
    }

    // Default to UTF-8
    return Encoding::utf8;
}

EncodingResult skip_bom(std::span<const std::byte> data) {
    // UTF-8 BOM
    if (data.size() >= 3 &&
        data[0] == std::byte{0xEF} &&
        data[1] == std::byte{0xBB} &&
        data[2] == std::byte{0xBF}) {
        return {data.subspan(3), Encoding::utf8};
    }

    return {data, detect_encoding(data)};
}

} // namespace sprawn
