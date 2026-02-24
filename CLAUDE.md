# Sprawn

A modular text viewer and basic editor. It should be able to open very large files and scroll around in them effortlessly, including font highlighting. It is platform independent and supports many different OS' and systems.

**Language**: C++ (C++17)
**Build system**: CMake with pkg-config

## Architecture

Three interchangeable modules connected via abstract interfaces:

```
Backend → (Document interface) → Frontend
              ↑
          Middleware (future)
```

### Frontend (`libsprawn_frontend`)

Renders text in a window with full Unicode support. Handles text layout internally (no widget toolkit).

- **Public API**: `include/sprawn/frontend.h` (pimpl pattern — callers never see internal deps)
- **Internal sources**: `src/frontend/`
- **Dependencies**: SDL2 (windowing/rendering), FreeType2 (glyph rasterization), HarfBuzz (text shaping)
- **Key components**:
  - `window` — SDL2 window + renderer wrapper
  - `font` — FreeType + HarfBuzz integration for shaping and rasterization
  - `glyph_cache` — caches rasterized glyphs as SDL textures (keyed by glyph ID, not codepoint)
  - `text_layout` — line splitting, viewport math, lazy shaping (only visible lines are shaped)
  - `renderer` — draws shaped text via SDL_Renderer

### Backend (`libsprawn_backend`)

Reads data from sources and provides it as a line-based document. No external dependencies.

- **Public API**: `include/sprawn/backend.h`, `include/sprawn/source.h`, `include/sprawn/document.h`
- **Internal sources**: `src/backend/`
- **Key abstractions**:
  - `Source` (abstract) — byte-oriented reader (`read()`, `at_end()`, `info()`)
    - `FileSource` — reads from files via `fopen`/`fread`
  - `Document` (abstract) — line-oriented read access (`line_count()`, `line()`, `is_fully_loaded()`)
    - `StringArrayDocument` — stores lines in `vector<string>`
  - `Backend` (pimpl) — coordinates Source → line splitting → Document

### Middleware (future)

Modifies or acts on the file content between backend and frontend.

## Directory Structure

```
sprawn/
├── CMakeLists.txt                        # Top-level: find deps, add subdirs
├── CLAUDE.md
├── include/sprawn/
│   ├── frontend.h                        # Public frontend API
│   ├── backend.h                         # Public backend API
│   ├── source.h                          # Abstract Source interface
│   └── document.h                        # Abstract Document interface
├── src/
│   ├── frontend/                         # Frontend internals
│   │   ├── CMakeLists.txt
│   │   ├── frontend.cpp, window, font, glyph_cache, text_layout, renderer
│   │   └── util.h
│   └── backend/                          # Backend internals
│       ├── CMakeLists.txt
│       ├── backend.cpp
│       ├── file_source.h/.cpp
│       └── string_array_document.h/.cpp
├── examples/
│   ├── CMakeLists.txt
│   └── demo.cpp                          # Demo app (optional file arg via backend)
└── plans/                                # Design documents
```

## Key Design Decisions

- **Pimpl pattern** on public classes keeps SDL/FreeType/HarfBuzz out of public headers
- **Lazy shaping** — only ~50 visible lines shaped at a time for instant open on large files
- **Source is byte-oriented, Document is line-oriented** — clean separation of concerns
- **Document::line() returns string_view** — zero-copy for consumers
- **No word-wrap in v1** — lines end at `\n` only

## Building

```bash
cmake -B build && cmake --build build
./build/examples/sprawn_demo [optional_file.txt]
```

### Dependencies

```bash
# Ubuntu/Debian
sudo apt install libsdl2-dev libfreetype-dev libharfbuzz-dev

# macOS
brew install sdl2 freetype harfbuzz
```
