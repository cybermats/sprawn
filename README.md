# Sprawn

A fast, cross-platform text viewer and editor built for large files. Sprawn opens million-line files instantly and scrolls through them effortlessly, with full Unicode rendering including Latin, Cyrillic, Greek, CJK, and more.

## Features

- **Large file performance** — lazy text shaping means only visible lines are processed, so even huge files open without delay
- **Full Unicode support** — powered by HarfBuzz for text shaping and FreeType for glyph rasterization
- **Modular architecture** — swappable backend (how files are read), frontend (how they're displayed), and middleware (future: transformations and highlighting)
- **Cross-platform** — runs on Linux and macOS via SDL2

## Building

### Prerequisites

Ubuntu/Debian:

```bash
sudo apt install cmake build-essential libsdl2-dev libfreetype-dev libharfbuzz-dev
```

macOS:

```bash
brew install cmake sdl2 freetype harfbuzz
```

### Compile

```bash
cmake -B build
cmake --build build
```

### Run

```bash
# Launch with generated demo text
./build/examples/sprawn_demo

# Open a file
./build/examples/sprawn_demo path/to/file.txt
```

## Architecture

Sprawn is split into independent modules that communicate through abstract interfaces:

- **Backend** — reads data from sources (files, streams) and exposes it as a line-based `Document`
- **Frontend** — renders the document in a window with hardware-accelerated text drawing
- **Middleware** *(planned)* — sits between backend and frontend to provide syntax highlighting, search, and editing

## License

[MIT](LICENSE)
