#pragma once

#include <sprawn/decoration.h>

#include <vector>

namespace sprawn {

class DecorationCompositor {
public:
    static std::vector<StyledSpan> flatten(
        const LineDecoration& deco, int line_byte_len,
        const TextStyle& default_style = TextStyle{});
};

} // namespace sprawn
