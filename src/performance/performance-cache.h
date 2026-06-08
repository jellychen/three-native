#pragma once
#include "common.h"
#include "core/renderable.h"
#include "renderer/render-list.h"
#include "renderer/program-cache.h"
#include "renderer/uniform-cache.h"
#include "renderer/gl-resources.h"
#include <unordered_set>

namespace THREE {

// v5.0 performance/parity metadata. These types are deliberately lightweight:
// they mirror the pieces three.js tracks in WebGLInfo/WebGLRenderLists/
// WebGLPrograms/WebGLTextures/WebGLGeometries, but they do not own GL state.
// Renderer subsystems can fill these counters without coupling profiling UI
// to the GL implementation.
struct RendererCacheFrameStats {
    int renderItems = 0;
    int opaqueItems = 0;
    int transmissiveItems = 0;
    int transparentItems = 0;

    int programCacheHits = 0;
    int programCacheMisses = 0;
    int livePrograms = 0;

    int geometryUploads = 0;
    int geometryCacheHits = 0;
    int textureUploads = 0;
    int textureCacheHits = 0;
    int vaoBinds = 0;
    int vaoBindSkips = 0;

    int drawCalls = 0;
    int instancedDrawCalls = 0;
    int triangles = 0;
    int lines = 0;
    int points = 0;

    void resetFrame() { *this = {}; }
};

struct LargeSceneProfile {
    int objectCount = 0;
    int meshCount = 0;
    int materialCount = 0;
    int geometryCount = 0;
    int textureCount = 0;
    int transparentCount = 0;
    int transmissiveCount = 0;
    int instancedCount = 0;
};

inline LargeSceneProfile analyze_large_scene(Object3D& root) {
    LargeSceneProfile p;
    std::unordered_set<ObjectId> materials;
    std::unordered_set<ObjectId> geometries;
    std::unordered_set<ObjectId> textures;
    root.traverse([&](Object3D& object) {
        ++p.objectCount;
        if (auto* r = dynamic_cast<RenderableObject*>(&object)) {
            if (r->geometry) geometries.insert(r->geometry->id);
            if (r->material) {
                materials.insert(r->material->id);
                if (r->material->transparent) ++p.transparentCount;
                if (RenderList::classify(*r->material) == RenderQueueBucket::Transmissive) ++p.transmissiveCount;
                auto addTex = [&](const std::shared_ptr<Texture>& t) { if (t) textures.insert(t->id); };
                if (auto* m = dynamic_cast<MeshStandardMaterial*>(r->material.get())) {
                    addTex(m->map); addTex(m->normalMap); addTex(m->roughnessMap); addTex(m->metalnessMap); addTex(m->aoMap); addTex(m->emissiveMap);
                }
            }
            if (object.kind == ObjectKind::Mesh || object.kind == ObjectKind::SkinnedMesh) ++p.meshCount;
        }
    });
    p.materialCount = static_cast<int>(materials.size());
    p.geometryCount = static_cast<int>(geometries.size());
    p.textureCount = static_cast<int>(textures.size());
    return p;
}

// Stable key for multi-material/drawRange experiments. It maps to three.js
// BufferGeometry.groups + material array concepts. Current renderer still draws
// one RenderItem per material group, but this key gives cache/sort code a stable
// identity and documents the intended path.
struct DrawRangeKey {
    ObjectId geometryId = 0;
    ObjectId materialId = 0;
    int start = 0;
    int count = -1;
    int materialIndex = 0;
    bool operator==(const DrawRangeKey& o) const {
        return geometryId == o.geometryId && materialId == o.materialId && start == o.start && count == o.count && materialIndex == o.materialIndex;
    }
};

struct DrawRangeKeyHash {
    std::size_t operator()(const DrawRangeKey& k) const noexcept {
        std::size_t h = std::hash<ObjectId>{}(k.geometryId);
        h ^= std::hash<ObjectId>{}(k.materialId) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(k.start) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(k.count) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(k.materialIndex) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
        return h;
    }
};

class RenderListCache {
    std::unordered_map<std::uint64_t, RenderListStats> cachedStats;
public:
    void clear() { cachedStats.clear(); }
    void store(std::uint64_t sceneSignature, const RenderList& list) { cachedStats[sceneSignature] = list.stats(); }
    std::optional<RenderListStats> find(std::uint64_t sceneSignature) const {
        auto it = cachedStats.find(sceneSignature);
        if (it == cachedStats.end()) return std::nullopt;
        return it->second;
    }
    int size() const { return static_cast<int>(cachedStats.size()); }
};

inline std::uint64_t scene_cache_signature(Object3D& root) {
    std::uint64_t h = 1469598103934665603ull;
    root.traverse([&](Object3D& o) {
        h ^= static_cast<std::uint64_t>(o.id) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
        h ^= static_cast<std::uint64_t>(o.visible ? 1 : 0) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
        h ^= static_cast<std::uint64_t>(o.layers) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
        if (auto* r = dynamic_cast<RenderableObject*>(&o)) {
            h ^= static_cast<std::uint64_t>(r->geometry ? r->geometry->version : 0) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
            h ^= static_cast<std::uint64_t>(r->material ? r->material->version : 0) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
        }
    });
    return h;
}

} // namespace THREE
