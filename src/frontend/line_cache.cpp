#include <sprawn/frontend/line_cache.h>

#include <algorithm>

namespace sprawn {

LineCache::LineCache(size_t capacity) : capacity_(capacity) {}

const GlyphRun* LineCache::get(size_t line, uint64_t hash) {
    auto it = index_.find(line);
    if (it == index_.end()) return nullptr;

    auto list_it = it->second;
    if (list_it->hash != hash) return nullptr;

    // Move to front (most recently used)
    lru_.splice(lru_.begin(), lru_, list_it);
    return &list_it->run;
}

void LineCache::put(size_t line, uint64_t hash, GlyphRun run) {
    auto it = index_.find(line);
    if (it != index_.end()) {
        // Update existing entry
        auto list_it = it->second;
        list_it->hash = hash;
        list_it->run  = std::move(run);
        lru_.splice(lru_.begin(), lru_, list_it);
        return;
    }

    evict_if_full();

    lru_.push_front(CacheEntry{line, hash, std::move(run)});
    index_[line] = lru_.begin();
}

void LineCache::invalidate(size_t line) {
    auto it = index_.find(line);
    if (it == index_.end()) return;
    lru_.erase(it->second);
    index_.erase(it);
}

void LineCache::invalidate_range(size_t first, size_t removed_count,
                                  int line_delta)
{
    // Collect keys to remove and keys to renumber
    std::vector<size_t>                      to_remove;
    std::vector<std::pair<size_t, size_t>>   to_renumber; // {old_key, new_key}

    for (auto& [key, _] : index_) {
        if (key >= first && key < first + removed_count) {
            to_remove.push_back(key);
        } else if (key >= first + removed_count && line_delta != 0) {
            size_t new_key = static_cast<size_t>(
                static_cast<long long>(key) + line_delta);
            to_renumber.emplace_back(key, new_key);
        }
    }

    for (size_t k : to_remove) {
        lru_.erase(index_[k]);
        index_.erase(k);
    }

    // Sort to avoid key collisions during renumbering.
    if (line_delta > 0) {
        std::sort(to_renumber.begin(), to_renumber.end(),
                  [](const auto& a, const auto& b) { return a.first > b.first; });
    } else {
        std::sort(to_renumber.begin(), to_renumber.end());
    }

    for (auto& [old_k, new_k] : to_renumber) {
        auto it = index_[old_k];
        it->line = new_k;
        index_.erase(old_k);
        index_[new_k] = it;
    }
}

void LineCache::clear() {
    lru_.clear();
    index_.clear();
}

void LineCache::evict_if_full() {
    while (lru_.size() >= capacity_) {
        auto& last = lru_.back();
        index_.erase(last.line);
        lru_.pop_back();
    }
}

} // namespace sprawn
