#pragma once

#include <sprawn/decoration.h>

#include <cstddef>
#include <string_view>

namespace sprawn {

class DecorationSource {
public:
    virtual ~DecorationSource() = default;
    virtual LineDecoration decorate(size_t line_number) const = 0;
    virtual std::string_view name() const = 0;
    virtual int base_priority() const { return 0; }
    virtual void on_edit(size_t line, size_t col,
                         std::string_view text, bool is_insert) {
        (void)line; (void)col; (void)text; (void)is_insert;
    }
};

} // namespace sprawn
