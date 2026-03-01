#pragma once

#include <sprawn/color.h>

#include <vector>

namespace sprawn {

struct TextStyle {
    Color fg{220, 220, 220, 255};
    Color bg{0, 0, 0, 0};
    bool  bold{false};
    bool  italic{false};
    bool  underline{false};
    Color underline_color{0, 0, 0, 0};
};

struct StyledSpan {
    int       byte_start;  // inclusive byte offset
    int       byte_end;    // exclusive byte offset
    TextStyle style;
    int       priority{0};
};

struct LineDecoration {
    std::vector<StyledSpan> spans;
};

} // namespace sprawn
