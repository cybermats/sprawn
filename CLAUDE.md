# Sprawn

A text editor built for very large files, targeting all modern platforms.

## Architecture

Sprawn is split into three layers: **backend**, **middleware**, and **frontend**. Each layer lives in its own source directory with a matching public header directory under `include/sprawn/`.

### Backend (`src/backend/`)
Handles file I/O and editing.

- Opens files via **memory-mapped I/O** for a small memory footprint.
- Edits are tracked with a **Piece Table**, so even multi-gigabyte files can be modified without rewriting the buffer.
- Currently supports UTF-8 and ASCII encodings. Additional encodings (UTF-16, UTF-32, ISO 8859-1, etc.) are a future goal. The frontend always receives UTF-8.
- A **line index** provides fast random access to any line.

Key public headers: `document.h`, `encoding.h`, `source.h`.

### Middleware (`src/middleware/`)
Thin pass-through layer between the frontend and the backend.

- `Controller` wraps a `Document` and exposes `line()`, `line_count()`, `insert()`, `erase()`.
- All methods are `virtual` so future plugins (syntax highlighting, search, etc.) can intercept or augment calls.
- The `Editor` talks exclusively through `Controller&`, never directly to `Document`.

### Frontend (`src/frontend/`)
Cross-platform graphical editor built on **SDL2**, **FreeType**, **HarfBuzz**, and **ICU**.

- No widget toolkit — all text layout and rendering is done internally.
- Full Unicode support: Latin, CJK, Arabic (BiDi via ICU), color emoji.
- **Font fallback chain**: primary font → Noto Sans Mono → DejaVu Sans Mono.
- **Glyph atlas**: shelf-packed RGBA texture for fast GPU blitting.
- **LRU line cache** with FNV-1a hash for stale detection.
- HiDPI aware (fonts rasterized at `logical_size × dpi_scale`).
- Runtime font zoom via Ctrl+scroll (8–72 logical px).
- Text selection and clipboard support.
- Lazy shaping: `shape_line()` can truncate at a max pixel width.

Key public headers: `application.h`, `editor.h`, `events.h`, `viewport.h`, `text_layout.h`, `line_cache.h`, `renderer.h`, `window.h`, `input_handler.h`.

## Build

```bash
cmake -B build
cmake --build build -j$(nproc)
```

Run: `./build/src/frontend/sprawn [file]`

Tests: `cd build && ctest --output-on-failure`

Requires C++20. Dependencies: SDL2, FreeType, HarfBuzz, ICU.

## Project layout

```
include/sprawn/           Public headers
  document.h              Document API (line, insert, erase, line_count)
  encoding.h              Encoding detection and conversion
  source.h                Abstract data source
  frontend/               Frontend public headers
  middleware/controller.h  Controller interface
src/
  backend/                Document, PieceTable, MappedFile, LineIndex, Encoding
  middleware/              Controller implementation
  frontend/               Application, Editor, Window, Renderer, TextLayout, …
  main.cpp                Entry point → sprawn::run_application(filepath)
tests/                    doctest unit tests (PieceTable, LineIndex, Document)
```

