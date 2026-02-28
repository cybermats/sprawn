#pragma once

#include "text_layout.h"

#include <cstddef>
#include <cstdint>
#include <list>
#include <optional>
#include <unordered_map>

namespace sprawn {

struct CacheEntry {
    size_t   line;
    uint64_t hash;
    GlyphRun run;
};

class LineCache {
public:
    explicit LineCache(size_t capacity = 512);

    // Returns a pointer to the cached GlyphRun if the line is present and
    // the stored hash matches. Returns nullptr on miss or stale entry.
    const GlyphRun* get(size_t line, uint64_t hash);

    void put(size_t line, uint64_t hash, GlyphRun run);

    void invalidate(size_t line);

    // After a multi-line insert/delete:
    //   - evict entries in [first, first+removed_count)
    //   - shift remaining entries with key >= first+removed_count by line_delta
    void invalidate_range(size_t first, size_t removed_count, int line_delta);

    void clear();

private:
    size_t capacity_;
    std::list<CacheEntry>                                   lru_;
    std::unordered_map<size_t, std::list<CacheEntry>::iterator> index_;

    void evict_if_full();
};

} // namespace sprawn
