#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sprawn::detail {

// Decode one UTF-8 codepoint from data, advancing pos.
// Returns 0xFFFD (replacement char) on invalid sequences.
inline uint32_t utf8_decode(const char* data, std::size_t len, std::size_t& pos) {
    if (pos >= len) return 0;

    auto byte = static_cast<uint8_t>(data[pos]);

    uint32_t cp;
    int extra;

    if (byte < 0x80) {
        cp = byte;
        extra = 0;
    } else if ((byte & 0xE0) == 0xC0) {
        cp = byte & 0x1F;
        extra = 1;
    } else if ((byte & 0xF0) == 0xE0) {
        cp = byte & 0x0F;
        extra = 2;
    } else if ((byte & 0xF8) == 0xF0) {
        cp = byte & 0x07;
        extra = 3;
    } else {
        ++pos;
        return 0xFFFD;
    }

    if (pos + extra >= len + 1) {
        pos = len;
        return 0xFFFD;
    }

    ++pos;
    for (int i = 0; i < extra; ++i) {
        auto cont = static_cast<uint8_t>(data[pos]);
        if ((cont & 0xC0) != 0x80) return 0xFFFD;
        cp = (cp << 6) | (cont & 0x3F);
        ++pos;
    }

    return cp;
}

} // namespace sprawn::detail
