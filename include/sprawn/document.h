#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>

namespace sprawn {

class Document {
public:
    Document();
    ~Document();

    Document(Document&&) noexcept;
    Document& operator=(Document&&) noexcept;

    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;

    void open_file(const std::filesystem::path& path);

    std::string line(size_t line_number) const;
    size_t line_count() const;

    /// Insert text at the given line and byte offset within that line.
    void insert(size_t line, size_t col, std::string_view text);
    /// Erase `count` bytes starting at the given line and byte offset.
    void erase(size_t line, size_t col, size_t count);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace sprawn
