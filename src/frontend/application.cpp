#include <sprawn/frontend/application.h>
#include <sprawn/frontend/editor.h>
#include <sprawn/frontend/renderer.h>
#include <sprawn/frontend/window.h>
#include <sprawn/document.h>

#include "font_chain.h"
#include "glyph_atlas.h"

#include <cstdio>
#include <stdexcept>
#include <string>

namespace sprawn {

bool run_application(std::string_view filepath) {
    Document doc;
    if (!filepath.empty()) {
        try {
            doc.open_file(std::string(filepath));
        } catch (const std::exception& e) {
            std::fprintf(stderr, "sprawn: cannot open '%.*s': %s\n",
                         static_cast<int>(filepath.size()), filepath.data(),
                         e.what());
            return false;
        }
    } else {
        doc.insert(0, 0, "");
    }

    try {
        constexpr int kInitW    = 1200;
        constexpr int kInitH    = 800;
        constexpr int kFontSize = 16;

        Window window("Sprawn", kInitW, kInitH);
        Renderer renderer(window.sdl_renderer());

        auto font_path = find_system_mono_font();
        if (font_path.empty()) {
            std::fprintf(stderr, "sprawn: no monospace font found\n");
            return false;
        }

        FontChain fonts(font_path, kFontSize);

        // Add fallback fonts for broad Unicode coverage
        fonts.add_fallback("/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf");
        fonts.add_fallback("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc");
        fonts.add_fallback("/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf");

        GlyphAtlas atlas(renderer, fonts);
        Editor editor(doc, renderer, fonts, atlas, kInitW, kInitH);

        bool running = true;
        while (running) {
            running = window.poll_events([&](const SDL_Event& ev) {
                editor.handle_event(ev);
            });
            editor.render();
            window.present();
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "sprawn: fatal error: %s\n", e.what());
        return false;
    }

    return true;
}

} // namespace sprawn
