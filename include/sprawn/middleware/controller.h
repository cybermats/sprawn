#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace sprawn {

class Document;

class Controller {
public:
    explicit Controller(Document& doc);
    virtual ~Controller() = default;

    virtual std::string line(size_t line_number) const;
    virtual size_t line_count() const;
    virtual void insert(size_t line, size_t col, std::string_view text);
    virtual void erase(size_t line, size_t col, size_t count);

protected:
    Document& doc_;
};

} // namespace sprawn
