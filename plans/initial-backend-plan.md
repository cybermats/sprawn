# Backend Framework Plan

## Context

Sprawn currently has only a Frontend module. The backend's job is to read data from various sources (files, streams, websockets, etc.) and provide it in a structured form (line-based document) that the frontend can consume. This plan creates the framework with two abstract interfaces (Source and Document) and their simplest concrete implementations (FileSource and StringArrayDocument).

## Architecture

```
Source (abstract)          Document (abstract)
  │                            │
  ├─ FileSource               ├─ StringArrayDocument
  ├─ (future: StreamSource)   ├─ (future: RopeDocument)
  └─ (future: WebsocketSource)└─ (future: PieceTableDocument)

Backend (Pimpl) — coordinates Source → Document loading
```

**Flow:** `Source` reads raw bytes → `Backend::Impl::load()` splits into lines → `Document` stores them → Frontend reads via `Document` interface.

## New Files

### Public headers (`include/sprawn/`)

1. **`source.h`** — Abstract `Source` interface + `SourceInfo` struct
   - `read(char* buf, size_t max)` → returns bytes read
   - `at_end()` → bool
   - `info()` → SourceInfo (name, size_bytes, seekable)

2. **`document.h`** — Abstract `Document` interface (line-based read access)
   - `line_count()` → size_t
   - `line(size_t index)` → string_view
   - `size_bytes()` → size_t
   - `is_fully_loaded()` → bool

3. **`backend.h`** — `Backend` class with Pimpl + `BackendConfig` struct
   - `open_file(const string& path)` — convenience for files
   - `load_from(unique_ptr<Source>)` — generic loading
   - `document()` → `const Document*`
   - `source_info()` → SourceInfo

### Internal files (`src/backend/`)

4. **`CMakeLists.txt`** — static library `sprawn_backend`, no external deps
5. **`file_source.h/.cpp`** — `detail::FileSource` using `fopen`/`fread`
6. **`string_array_document.h/.cpp`** — `detail::StringArrayDocument` using `vector<string>`
7. **`backend.cpp`** — `Backend::Impl` with line-splitting load logic

## Modified Files

8. **`CMakeLists.txt`** (root) — add `add_subdirectory(src/backend)` before examples
9. **`include/sprawn/frontend.h`** — add `set_document(const Document& doc)` method
10. **`src/frontend/frontend.cpp`** — implement `set_document()` (builds string from document lines, calls `set_text()`)
11. **`examples/CMakeLists.txt`** — link `sprawn_backend`
12. **`examples/demo.cpp`** — update to load a file via backend (accept argv), keep generated demo text as fallback

## Key Design Decisions

- **Source is byte-oriented**, not line-oriented — works for any data stream
- **Document is line-oriented** — matches what the frontend needs
- **`Document::line()` returns `string_view`** — zero-copy for consumers
- **Backend owns both Source and Document** — simple coordinator pattern
- **`set_document()` on Frontend** builds a string and calls existing `set_text()` — avoids changing frontend internals now; can be optimized later to read directly from Document

## Implementation Order

1. Create `include/sprawn/source.h`
2. Create `include/sprawn/document.h`
3. Create `include/sprawn/backend.h`
4. Create `src/backend/CMakeLists.txt`
5. Create `src/backend/file_source.h` and `file_source.cpp`
6. Create `src/backend/string_array_document.h` and `string_array_document.cpp`
7. Create `src/backend/backend.cpp`
8. Update root `CMakeLists.txt`
9. Add `set_document()` to frontend (header + implementation)
10. Update examples (CMakeLists.txt + demo.cpp)

## Verification

1. `cmake -B build && cmake --build build` — must compile cleanly
2. `./build/examples/sprawn_demo` — runs with generated demo text (no args)
3. `./build/examples/sprawn_demo some_file.txt` — opens and displays the file
