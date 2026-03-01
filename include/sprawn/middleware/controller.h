#pragma once

#include <sprawn/decoration.h>
#include <sprawn/middleware/decoration_source.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace sprawn {

class Document;

class Controller {
public:
    explicit Controller(Document& doc);
    virtual ~Controller() = default;

    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;
    Controller(Controller&&) = delete;
    Controller& operator=(Controller&&) = delete;

    virtual void open_file(const std::filesystem::path& path);
    virtual std::string line(size_t line_number) const;
    virtual size_t line_count() const;
    virtual void insert(size_t line, size_t col, std::string_view text);
    virtual void erase(size_t line, size_t col, size_t count);

    void add_decoration_source(std::shared_ptr<DecorationSource> source);
    void remove_decoration_source(std::string_view name);
    virtual LineDecoration decorations(size_t line_number) const;

protected:
    Document& doc_;

private:
    std::vector<std::shared_ptr<DecorationSource>> sources_;
};

} // namespace sprawn
