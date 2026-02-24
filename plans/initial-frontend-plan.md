# Sprawn Frontend Library — Implementation Plan

## Context

Sprawn needs a cross-platform frontend library that opens a window and renders caller-provided text with full Unicode support. We handle text layout ourselves (no widget toolkit textbox). The library is the "Frontend" module in sprawn's architecture; Backend and Middleware are separate, future modules.

**Language**: C++ (C++17)
**Dependencies**: SDL2 (windowing/input/rendering), FreeType2 (glyph rasterization), HarfBuzz (Unicode text shaping)
**Build system**: CMake with pkg-config

---

## Directory Structure

```
sprawn/
├── CMakeLists.txt                     # Top-level: find deps, add subdirs
├── include/sprawn/frontend.h          # Public API (only header callers need)
├── src/frontend/
│   ├── CMakeLists.txt                 # Builds libsprawn_frontend (static)
│   ├── frontend.cpp                   # Implements public API (pimpl)
│   ├── window.h / window.cpp         # SDL2 window + renderer wrapper
│   ├── font.h / font.cpp             # FreeType + HarfBuzz integration
│   ├── glyph_cache.h / glyph_cache.cpp  # Caches rasterized glyphs as SDL textures
│   ├── text_layout.h / text_layout.cpp  # Line splitting, viewport, lazy shaping
│   ├── renderer.h / renderer.cpp      # Draws shaped text via SDL_Renderer
│   └── util.h                         # Small helpers (UTF-8 iteration)
└── examples/
    ├── CMakeLists.txt
    └── demo.cpp                       # Opens window, shows Unicode text, scrolls
```

All headers in `src/frontend/` are internal. Only `include/sprawn/frontend.h` is public.

---

## Key Components

### 1. Public API (`include/sprawn/frontend.h`)

Pimpl pattern — callers never see SDL/FreeType/HarfBuzz headers.

- `FrontendConfig` struct: window size, title, font path, font size, line spacing
- `Frontend` class:
  - `set_text(string_view utf8_text)` — set content to display
  - `scroll_to_line(size_t line)` — jump to a line
  - `run()` — blocking event loop (returns on window close)
  - `process_events()` / `render()` — non-blocking alternatives

### 2. Window (`window.h`)

SDL2 wrapper. Creates window with `SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI`. Creates accelerated renderer with vsync, software fallback. Exposes `SDL_Renderer*` and handles resize/quit events.

### 3. Font System (`font.h`)

- Loads font via FreeType, creates HarfBuzz font with `hb_ft_font_create`
- `shape(string_view)` → `ShapedRun` (vector of glyph IDs + positions from HarfBuzz)
- `rasterize(glyph_id)` → alpha bitmap via FreeType
- Reports `line_height()` and `ascender()` for layout

### 4. Glyph Cache (`glyph_cache.h`)

- `unordered_map<uint32_t glyph_id, CachedGlyph>` — keyed by glyph ID (not codepoint, because HarfBuzz contextual shaping)
- On first access: rasterize glyph, upload as SDL_Texture
- On repeat access: return cached texture

### 5. Text Layout Engine (`text_layout.h`)

- Splits text on `\n` into line entries (byte offset + length)
- **Lazy shaping**: only shapes lines when they enter the viewport (~50 lines at a time)
- Viewport math: `first_visible = scroll_y / line_height` (simple integer division since all lines are same height)
- Scroll clamping to `[0, total_height - viewport_height]`
- No word-wrap in v1 (each `\n` = one layout line)

### 6. Renderer (`renderer.h`)

- `draw_run()`: iterate ShapedGlyphs, lookup cached texture, `SDL_RenderCopy` with correct bearing/offset
- `draw_frame()`: get visible lines from layout, draw each run
- Uses `SDL_SetTextureColorMod` for text color (no re-rasterization needed)
- Dark background, light text by default

### 7. Frontend Impl (`frontend.cpp`)

Ties everything together. Event handling:
- `SDL_QUIT` → close
- `SDL_WINDOWEVENT_RESIZED` → update viewport
- `SDL_MOUSEWHEEL` → `layout.scroll_by(delta * line_height * 3)`
- Arrow keys, Page Up/Down, Home/End

---

## Implementation Order

### Phase 1: Window + build system
- `CMakeLists.txt` (top-level + src/frontend + examples) with pkg-config for SDL2, FreeType, HarfBuzz
- `window.cpp` — SDL init, create window + renderer
- Skeleton `frontend.cpp` — event loop that shows a dark window and handles quit
- **Verify**: window opens, dark screen, closes cleanly

### Phase 2: Font loading and shaping
- `font.cpp` — FreeType init, load face, HarfBuzz font, `shape()`, `rasterize()`, metrics
- **Verify**: shaping "Hello" produces 5 glyphs with reasonable advances (debug print)

### Phase 3: Glyph cache + basic rendering
- `glyph_cache.cpp` — rasterize → SDL_Texture cache
- `renderer.cpp` — `draw_run()` blits glyph textures
- Hardcode one line on screen
- **Verify**: "Hello, World!" appears in the window

### Phase 4: Text layout with scrolling
- `text_layout.cpp` — line splitting, lazy shaping, viewport, scroll
- Wire `renderer.draw_frame()` to layout's visible lines
- **Verify**: multi-line text displays, mouse wheel scrolls

### Phase 5: Public API + demo
- Complete `frontend.cpp` with `set_text()`, `scroll_to_line()`, full event handling
- `demo.cpp` — shows Latin, Greek, Russian, CJK text + 10k lorem ipsum lines
- **Verify**: demo runs, Unicode renders, smooth scrolling through 10k+ lines

### Phase 6: Polish
- Window resize handling
- Scroll bar indicator (thin bar on right edge)
- Error handling (missing font, SDL init failure)

---

## Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| Pimpl pattern | Keeps SDL/FreeType/HarfBuzz out of public headers |
| Lazy shaping | Only ~50 visible lines shaped at a time → instant open for million-line files |
| Cache by glyph ID, not codepoint | Correct for contextual shaping (ligatures, alternates) |
| Owned string copy in v1 | Simple. v2 upgrades to line-provider callback for streaming large files |
| No word-wrap in v1 | Avoids Unicode line break complexity (UAX #14). Lines end at `\n` |
| One SDL_Texture per glyph | Simple. Texture atlas can be added inside GlyphCache later if needed |

---

## Verification

1. **Build**: `cmake -B build && cmake --build build` succeeds on Linux
2. **Run demo**: `./build/examples/sprawn_demo` opens a window
3. **Unicode**: Latin, Cyrillic, Greek, CJK characters all render correctly
4. **Scrolling**: mouse wheel scrolls smoothly through 10k+ lines
5. **Resize**: window resizes without crash, text reflows viewport
6. **Clean exit**: closing window exits without leaks (run with `valgrind` or ASan)

---

## Dependencies to Install

```bash
# Ubuntu/Debian
sudo apt install libsdl2-dev libfreetype-dev libharfbuzz-dev

# macOS
brew install sdl2 freetype harfbuzz
```
