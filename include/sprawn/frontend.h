#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

namespace sprawn {

class Document;


struct FrontendConfig {
    int window_width = 800;
    int window_height = 600;
    std::string title = "Sprawn";
    std::string font_path;
    int font_size = 16;
    float line_spacing = 1.2f;
};

class Frontend {
public:
    explicit Frontend(const FrontendConfig& config);
    ~Frontend();

    Frontend(const Frontend&) = delete;
    Frontend& operator=(const Frontend&) = delete;
    Frontend(Frontend&&) noexcept;
    Frontend& operator=(Frontend&&) noexcept;

    void set_text(std::string_view utf8_text);
    void set_document(const Document& doc);
    void scroll_to_line(std::size_t line);

    // Blocking event loop — returns when window is closed.
    void run();

    // Non-blocking alternatives for embedding.
    // Returns false when the window should close.
    bool process_events();
    void render();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace sprawn
