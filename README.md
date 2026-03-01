# Sprawn

A text editor designed to handle very large files efficiently across all modern platforms.

## Features

- **Large file support** — files are memory-mapped and edited via a Piece Table, so even multi-gigabyte files open instantly with minimal memory usage.
- **Full Unicode** — Latin, CJK, Arabic (with BiDi), color emoji, and more.
- **Complex text shaping** — HarfBuzz for glyph shaping, ICU for bidirectional text.
- **Font fallback chain** — automatically falls back through multiple fonts to find the right glyph.
- **HiDPI aware** — sharp rendering on high-density displays.
- **Runtime font zoom** — Ctrl+scroll to resize (8–72 px).
- **Text selection and clipboard** — standard select, copy, cut, paste.
- **Multiple encodings** — UTF-8, UTF-16, UTF-32, ASCII, ISO 8859-1, and more. Files are kept in their original encoding internally.

## Building

Requires a C++20 compiler and the following libraries:

- [SDL2](https://www.libsdl.org/)
- [FreeType](https://freetype.org/)
- [HarfBuzz](https://harfbuzz.github.io/)
- [ICU](https://icu.unicode.org/)

```bash
cmake -B build
cmake --build build -j$(nproc)
```

## Usage

```bash
./build/src/frontend/sprawn [file]
```

Open a file by passing its path as an argument, or launch without arguments for an empty buffer.

## Tests

```bash
cd build && ctest --output-on-failure
```

## Architecture

Sprawn is organized into three layers:

| Layer | Directory | Purpose |
|---|---|---|
| **Backend** | `src/backend/` | File I/O (mmap), Piece Table editing, encoding conversion, line indexing |
| **Middleware** | `src/middleware/` | Controller interface between frontend and backend; plugin seam for future extensions |
| **Frontend** | `src/frontend/` | SDL2 window, FreeType/HarfBuzz text rendering, glyph atlas, input handling |

The frontend never talks to the backend directly — all document operations go through the middleware `Controller`.

## License

MIT — see [LICENSE](LICENSE).
