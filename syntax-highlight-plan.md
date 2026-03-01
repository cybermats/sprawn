# Plan: Syntax Highlighting & Decorations System Design

## Context

The CLAUDE.md notes that middleware `Controller` methods are `virtual` so "future plugins (syntax highlighting, search, etc.) can intercept or augment calls." This plan designs the full decoration pipeline — the data types, interfaces, rendering changes, and caching strategy — so that syntax highlighting, search highlights, diagnostic underlines, and selection can all flow through one unified system.

This is a **design document / architectural plan**, not an implementation plan for writing all the code at once. It defines the interfaces and data flow, then proposes incremental implementation phases.

---

## 1. Core Data Types

New file: `include/sprawn/decoration.h` (shared between middleware and frontend)

```cpp
struct TextStyle {
    Color fg{220, 220, 220, 255};
    Color bg{0, 0, 0, 0};            // alpha > 0 → fill background rect
    bool  bold{false};                // reserved — ignored until font variants exist
    bool  italic{false};              // reserved — ignored until font variants exist
    bool  underline{false};
    Color underline_color{0,0,0,0};   // alpha == 0 → use fg
};

struct StyledSpan {
    int       byte_start;   // inclusive, byte offset into line UTF-8
    int       byte_end;     // exclusive
    TextStyle style;
    int       priority{0};  // higher wins on overlap
};

struct LineDecoration {
    std::vector<StyledSpan> spans;  // sorted by byte_start
};
```

**Why byte offsets?** `GlyphEntry.cluster` (`text_layout.h:20`) is already a byte index into the source UTF-8. Byte offsets mean zero conversion cost at render time.

**Why `bold`/`italic` now?** Defining them in the struct avoids a breaking interface change later. They are simply ignored until Phase E adds font variants.

---

## 2. DecorationSource Interface (Middleware)

New file: `include/sprawn/middleware/decoration_source.h`

```cpp
class DecorationSource {
public:
    virtual ~DecorationSource() = default;

    virtual LineDecoration decorate(size_t line_number) const = 0;
    virtual std::string_view name() const = 0;
    virtual int base_priority() const { return 0; }

    // Called by Controller after insert/erase so source can update state.
    virtual void on_edit(size_t line, size_t col,
                         std::string_view text, bool is_insert) {}
};
```

Concrete implementations (each a future phase):
- `SyntaxHighlighter` — regex or Tree-sitter tokenizer
- `SearchHighlighter` — marks search matches
- `DiagnosticDecorator` — error/warning underlines

---

## 3. Controller Extension

Modified file: `include/sprawn/middleware/controller.h`

Add a decoration source registry and a query method:

```cpp
// New methods on Controller:
void add_decoration_source(std::shared_ptr<DecorationSource> source);
void remove_decoration_source(std::string_view name);
virtual LineDecoration decorations(size_t line_number) const;
```

`decorations()` iterates all registered sources, collects spans (adjusting each span's priority by the source's `base_priority()`), and returns a merged `LineDecoration`.

The existing `insert()`/`erase()` methods gain a loop that calls `src->on_edit(...)` on each registered source after forwarding to `Document`.

---

## 4. DecorationCompositor (Frontend)

New file: `include/sprawn/frontend/decoration_compositor.h`

```cpp
class DecorationCompositor {
public:
    // Resolve overlapping spans into a flat, non-overlapping sequence
    // covering [0, line_byte_len). Gaps get default_style.
    static std::vector<StyledSpan> flatten(
        const LineDecoration& deco,
        int line_byte_len,
        const TextStyle& default_style = TextStyle{});
};
```

**Compositing rules:**

| Property | Rule |
|---|---|
| `fg` | Highest priority wins |
| `bg` | Highest priority with `bg.a > 0` wins |
| `bold`/`italic` | Highest priority wins |
| `underline` | Union — any active span enables it |
| `underline_color` | Highest priority underlined span wins |

**Algorithm**: sweep-line over span boundaries, O(n log n) in number of input spans — negligible vs. shaping cost.

---

## 5. Rendering Changes

### 5a. New `draw_run` overload

Modified file: `include/sprawn/frontend/text_layout.h`

```cpp
// Existing (kept for gutter line numbers):
void draw_run(Renderer& r, const GlyphRun& run, int x, int y, Color tint);

// New:
void draw_run(Renderer& r, const GlyphRun& run, int x, int y,
              const std::vector<StyledSpan>& flat_spans);
```

Implementation uses a **two-pointer scan** — both glyphs and flat_spans are sorted by byte offset, so it's O(glyphs + spans):

1. **Background pass**: for each span with `bg.a > 0`, draw `fill_rect` from span's first glyph x to last glyph x.
2. **Glyph pass**: for each glyph, advance span pointer by `ge.cluster`, blit with `span.style.fg`.
3. **Underline pass**: for each span with `underline`, draw `fill_rect` at `baseline + 2px`, 1px tall.

### 5b. Editor render loop change

Modified file: `src/frontend/editor.cpp`

Current code for each visible line:
```cpp
layout_.draw_run(renderer_, *run_ptr, text_x, y, Color{220,220,220,255});
```

Becomes:
```cpp
LineDecoration deco = ctrl_.decorations(L);
auto flat = DecorationCompositor::flatten(deco, utf8.size());
// inject selection as high-priority bg span (priority 1000)
layout_.draw_run(renderer_, *run_ptr, text_x, y, flat);
```

When no decoration sources are registered, `decorations()` returns empty → `flatten()` produces one span with the default style → behavior identical to today.

---

## 6. Caching Strategy

**Key insight**: shaping is style-independent. A glyph's position doesn't change based on its color. Therefore:

- **LineCache** (`line_cache.h`) continues to cache `GlyphRun` keyed by `(line_number, content_hash)`. No changes needed.
- **Decorations are not cached** initially. `flatten()` is computed per-frame for visible lines only (typically 30–60 lines, a few dozen spans each). This is negligible compared to shaping.
- A decoration cache can be added later if profiling shows need, using a version counter on each `DecorationSource`.

A color-only change (e.g., typing changes token boundaries) **never evicts shape caches**.

---

## 7. Multi-Line Invalidation (Block Comments, etc.)

Inside a `SyntaxHighlighter` implementation (not in the core pipeline):

- Each line records its **entry state** (e.g., "inside block comment" vs. "normal").
- After an edit, re-tokenize forward from the edited line until a line's computed entry state matches its previously recorded state.
- `on_edit()` triggers this re-scan.
- The core pipeline doesn't need to know about this — it just calls `decorate(line)` for each visible line.

---

## 8. Data Flow

```
                     Edit (insert/erase)
                            │
                            ▼
                   Controller::insert()
                    ├── Document::insert()           (content)
                    └── DecorationSource::on_edit()   (re-tokenize)

                        Render frame
                            │
              for each visible line L:
                            │
  ┌─────────────────────────┼──────────────────────────┐
  │                         │                          │
  ▼                         ▼                          ▼
ctrl_.line(L)       ctrl_.decorations(L)         (selection state)
  │                         │                          │
  ▼                         │                          │
shape_line(utf8)            │                          │
  │  (cached in LineCache)  │                          │
  ▼                         ▼                          ▼
GlyphRun              LineDecoration          inject selection span
                            │                          │
                            └──────────┬───────────────┘
                                       ▼
                          DecorationCompositor::flatten()
                                       │
                                       ▼
                            draw_run(run, flat_spans)
                              ├── bg rects
                              ├── glyph blits (per-span fg color)
                              └── underline rects
```

---

## 9. Implementation Phases

### Phase A: Core pipeline (no visual change)
Create `decoration.h`, `decoration_source.h`, `decoration_compositor.h/.cpp`. Extend `Controller` with registry + `decorations()`. Add new `draw_run` overload. Wire `Editor::render()` to use it. With zero sources registered, output is identical to today.

### Phase B: Selection as decoration
Add `bg` rendering to `draw_run`. Remove hardcoded selection `fill_rect`. Inject selection as a `StyledSpan{priority=1000, bg={65,120,200,160}}`.

### Phase C: Regex syntax highlighter
Implement `RegexSyntaxHighlighter : DecorationSource` in middleware. Simple pattern table (keywords, strings, comments, numbers). State-carry for multi-line constructs (block comments). Register in `application.cpp`.

### Phase D: Search highlights + diagnostics
`SearchHighlighter` (base_priority=10, bg color), `DiagnosticDecorator` (base_priority=20, underline). Tests for overlapping decorations.

### Phase E: Bold/italic font variants (future)
Extend `FontChain` with bold/italic slots. `draw_run` selects font variant per span. Requires reshaping — shape cache key must include style hash.

### Phase F: Tree-sitter integration (future)
`TreeSitterHighlighter : DecorationSource` wrapping Tree-sitter. Incremental parsing via `on_edit`. Tight `dirty_range()` tracking.

---

## 10. Files Affected (Phases A–B)

| File | Action |
|------|--------|
| `include/sprawn/decoration.h` | **Create** — `TextStyle`, `StyledSpan`, `LineDecoration` |
| `include/sprawn/middleware/decoration_source.h` | **Create** — `DecorationSource` interface |
| `include/sprawn/frontend/decoration_compositor.h` | **Create** — `DecorationCompositor` |
| `src/frontend/decoration_compositor.cpp` | **Create** — `flatten()` implementation |
| `include/sprawn/middleware/controller.h` | **Edit** — add registry + `decorations()` |
| `src/middleware/controller.cpp` | **Edit** — implement registry, `on_edit` dispatch |
| `include/sprawn/frontend/text_layout.h` | **Edit** — add new `draw_run` overload |
| `src/frontend/text_layout.cpp` | **Edit** — implement two-pointer `draw_run` |
| `src/frontend/editor.cpp` | **Edit** — use `decorations()` + `flatten()` in render loop |
| `src/frontend/CMakeLists.txt` | **Edit** — add `decoration_compositor.cpp` |
| `tests/test_decoration_compositor.cpp` | **Create** — unit tests for `flatten()` |
| `tests/CMakeLists.txt` | **Edit** — add compositor test target |

## Verification

Phase A: editor compiles and renders identically to current behavior (no decoration sources registered).

Phase B: selection highlight works through the decoration pipeline. Manual visual test: select text, verify highlight appearance matches current behavior.

Unit tests for `DecorationCompositor::flatten()`:
- Empty input → single default span
- Non-overlapping spans → pass through with gaps filled
- Overlapping spans → higher priority wins for fg/bg
- Underline union semantics
- Edge cases: zero-length spans, spans exceeding line length