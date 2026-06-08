#pragma once
#include "geometry/buffer-attribute.h"

namespace THREE {

struct BoundingBox { glm::vec3 min{0}; glm::vec3 max{0}; bool valid = false; };
struct BoundingSphere { glm::vec3 center{0}; float radius = 0; bool valid = false; };

struct GeometryGroup {
    int start = 0;
    int count = 0;
    int materialIndex = 0;
};

struct DrawRange {
    int start = 0;
    int count = std::numeric_limits<int>::max();
};

class BufferGeometry {
public:
    ObjectId id = next_object_id();
    std::string name;

    std::unordered_map<std::string, BufferAttribute> attributes;

    // three.js-compatible morph target storage.
    // Key examples: "position", "normal", "color".
    // Each vector entry is one morph target attribute with the same itemSize/count
    // as the base attribute. When morphTargetsRelative is false, morph target
    // attributes are interpreted as absolute target values; otherwise they are
    // interpreted as deltas.
    std::unordered_map<std::string, std::vector<BufferAttribute>> morphAttributes;
    bool morphTargetsRelative = false;

    // Morph target metadata for three.js parity work. Render backends may use
    // attribute mode for a small number of active targets and CPU/texture fallback
    // for large facial rigs such as RobotExpressive-style assets.
    bool morphTargetsNeedUpdate = true;
    bool morphTextureFallbackPreferred = false;
    int morphTextureStride = 0;

    std::vector<std::uint32_t> indices;
    std::vector<GeometryGroup> groups;
    DrawRange drawRange;
    BoundingBox boundingBox;
    BoundingSphere boundingSphere;

    // Incremented whenever CPU-side geometry data changes.
    // Render backends can use this to invalidate VAO/VBO/IBO caches, similar to
    // three.js geometry attribute versioning / needsUpdate flow.
    std::uint64_t version = 0;

    void markNeedsUpdate();
    void clear();

    bool hasAttribute(std::string_view name) const;
    BufferAttribute* getAttribute(std::string_view name);
    const BufferAttribute* getAttribute(std::string_view name) const;
    void setAttribute(std::string name, BufferAttribute attr);
    void setMorphAttribute(std::string name, std::span<const BufferAttribute> attrs);
    void setMorphAttribute(std::string name, std::initializer_list<BufferAttribute> attrs);
    bool hasMorphAttribute(std::string_view name) const { return getMorphAttributes(name) != nullptr; }
    const std::vector<BufferAttribute>* getMorphAttributes(std::string_view name) const;
    int morphTargetCount(std::string_view name = "position") const;
    void setIndex(std::span<const std::uint32_t> idx);
    void setIndex(std::initializer_list<std::uint32_t> idx);

    void addGroup(int start, int count, int materialIndex = 0);
    void clearGroups();
    void setDrawRange(int start, int count);
    int drawStart() const { return std::max(0, drawRange.start); }
    int drawCountLimit() const { return drawRange.count < 0 ? 0 : drawRange.count; }

    int vertexCount() const;
    int indexCount() const { return static_cast<int>(indices.size()); }
    void computeBoundingBox();
    void computeBoundingSphere();
    void computeLineDistances(bool segments = false);

    static std::shared_ptr<BufferGeometry> makeBox(float size = 1.0f);
    static std::shared_ptr<BufferGeometry> makeLineGrid(int halfExtent = 10, float step = 1.0f);
    static std::shared_ptr<BufferGeometry> makeRandomPoints(int count, float radius = 1.0f);
};

} // namespace THREE
