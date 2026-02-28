#include <sprawn/frontend.h>
#include <sprawn/backend.h>

#include <sstream>
#include <string>

static std::string generate_demo_text() {
    std::ostringstream oss;

    oss << "=== Sprawn Text Viewer Demo ===\n\n";

    // Latin
    oss << "English: The quick brown fox jumps over the lazy dog.\n";
    oss << "French:  Le renard brun rapide saute par-dessus le chien paresseux.\n";
    oss << "German:  Falsches \xC3\x9C" "ben von Xylophonmusik qu\xC3\xA4lt jeden gr\xC3\xB6\xC3\x9F" "eren Zwerg.\n";
    oss << "\n";

    // Cyrillic
    oss << "Russian: \xD0\x91\xD1\x8B\xD1\x81\xD1\x82\xD1\x80\xD0\xB0\xD1\x8F "
           "\xD0\xBA\xD0\xBE\xD1\x80\xD0\xB8\xD1\x87\xD0\xBD\xD0\xB5\xD0\xB2\xD0\xB0\xD1\x8F "
           "\xD0\xBB\xD0\xB8\xD1\x81\xD0\xB0 \xD0\xBF\xD1\x80\xD1\x8B\xD0\xB3\xD0\xB0\xD0\xB5\xD1\x82 "
           "\xD1\x87\xD0\xB5\xD1\x80\xD0\xB5\xD0\xB7 \xD0\xBB\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB2\xD1\x83\xD1\x8E "
           "\xD1\x81\xD0\xBE\xD0\xB1\xD0\xB0\xD0\xBA\xD1\x83.\n";
    oss << "\n";

    // Greek
    oss << "Greek:   \xCE\x93\xCE\xB5\xCE\xB9\xCE\xAC \xCF\x83\xCE\xBF\xCF\x85 "
           "\xCE\xBA\xCF\x8C\xCF\x83\xCE\xBC\xCE\xB5!\n";
    oss << "\n";

    // CJK (basic characters)
    oss << "Chinese: \xE4\xBD\xA0\xE5\xA5\xBD\xE4\xB8\x96\xE7\x95\x8C\n";
    oss << "Japanese: \xE3\x81\x93\xE3\x82\x93\xE3\x81\xAB\xE3\x81\xA1\xE3\x81\xAF\xE4\xB8\x96\xE7\x95\x8C\n";
    oss << "\n";

    oss << "--- Scroll test: 10,000 lines follow ---\n\n";

    for (int i = 1; i <= 10000; ++i) {
        oss << "Line " << i << ": ";
        switch (i % 5) {
        case 0: oss << "Lorem ipsum dolor sit amet, consectetur adipiscing elit."; break;
        case 1: oss << "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."; break;
        case 2: oss << "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris."; break;
        case 3: oss << "Duis aute irure dolor in reprehenderit in voluptate velit esse."; break;
        case 4: oss << "Excepteur sint occaecat cupidatat non proident, sunt in culpa."; break;
        }
        oss << '\n';
    }

    oss << "\n=== End of demo text ===\n";
    return oss.str();
}

int main(int argc, char* argv[]) {
    sprawn::FrontendConfig config;
    config.window_width = 1024;
    config.window_height = 768;
    config.title = "Sprawn Demo";
    config.font_size = 16;

    sprawn::Frontend frontend(config);
    sprawn::Backend backend;

    if (argc > 2 && std::string(argv[1]) == "--lorem") {
        std::size_t num_lines = std::stoul(argv[2]);
        backend.open_lorem_ipsum(num_lines);

        const auto* doc = backend.document();
        if (doc) {
            frontend.set_document(*doc);
        }
    } else if (argc > 1) {
        backend.open_file(argv[1]);

        const auto* doc = backend.document();
        if (doc) {
            frontend.set_document(*doc);
        }
    } else {
        frontend.set_text(generate_demo_text());
    }

    frontend.run();

    return 0;
}
