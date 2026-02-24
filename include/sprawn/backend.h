#pragma once

#include <sprawn/document.h>
#include <sprawn/source.h>

#include <memory>
#include <string>

namespace sprawn {

struct BackendConfig {
    // Reserved for future options (e.g. encoding, chunk size).
};

class Backend {
public:
    explicit Backend(const BackendConfig& config = {});
    ~Backend();

    Backend(const Backend&) = delete;
    Backend& operator=(const Backend&) = delete;
    Backend(Backend&&) noexcept;
    Backend& operator=(Backend&&) noexcept;

    // Convenience: open a file by path.
    void open_file(const std::string& path);

    // Generic: load from any Source.
    void load_from(std::unique_ptr<Source> source);

    // Access the loaded document (nullptr if nothing loaded).
    const Document* document() const;

    // Info about the current source (empty if nothing loaded).
    SourceInfo source_info() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace sprawn
