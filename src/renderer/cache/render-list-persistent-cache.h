#pragma once
#include "renderer/render-list.h"

namespace THREE {

struct RenderListCacheKey {
    ObjectId sceneId = 0;
    ObjectId cameraId = 0;
    std::uint64_t sceneVersion = 0;
    std::uint64_t cameraVersion = 0;

    bool operator==(const RenderListCacheKey& rhs) const {
        return sceneId == rhs.sceneId && cameraId == rhs.cameraId && sceneVersion == rhs.sceneVersion && cameraVersion == rhs.cameraVersion;
    }
};

struct RenderListCacheKeyHash {
    std::size_t operator()(const RenderListCacheKey& k) const noexcept {
        std::size_t h = std::hash<ObjectId>{}(k.sceneId);
        h ^= std::hash<ObjectId>{}(k.cameraId) + 0x9e3779b9u + (h << 6u) + (h >> 2u);
        h ^= std::hash<std::uint64_t>{}(k.sceneVersion) + 0x9e3779b9u + (h << 6u) + (h >> 2u);
        h ^= std::hash<std::uint64_t>{}(k.cameraVersion) + 0x9e3779b9u + (h << 6u) + (h >> 2u);
        return h;
    }
};

struct RenderListPersistentCacheStats {
    int requests = 0;
    int hits = 0;
    int misses = 0;
    int invalidations = 0;
    int liveEntries = 0;
};

// Stores sorted render queues when the scene/camera/material/geometry signature
// did not change. This mirrors three.js' tendency to reuse render lists while
// still allowing explicit invalidation when versions change.
class RenderListPersistentCache {
public:
    const RenderList* find(const RenderListCacheKey& key) {
        ++stats.requests;
        auto it = lists.find(key);
        if (it == lists.end()) { ++stats.misses; return nullptr; }
        ++stats.hits;
        return &it->second;
    }

    void store(const RenderListCacheKey& key, const RenderList& list) {
        lists[key] = list;
        stats.liveEntries = static_cast<int>(lists.size());
    }

    void invalidateScene(ObjectId sceneId) {
        for (auto it = lists.begin(); it != lists.end();) {
            if (it->first.sceneId == sceneId) { it = lists.erase(it); ++stats.invalidations; }
            else ++it;
        }
        stats.liveEntries = static_cast<int>(lists.size());
    }

    void clear() {
        lists.clear();
        stats.liveEntries = 0;
    }

    RenderListPersistentCacheStats getStats() const {
        auto out = stats;
        out.liveEntries = static_cast<int>(lists.size());
        return out;
    }

private:
    std::unordered_map<RenderListCacheKey, RenderList, RenderListCacheKeyHash> lists;
    RenderListPersistentCacheStats stats;
};

} // namespace THREE
