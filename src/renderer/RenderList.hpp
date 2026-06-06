#pragma once
#include "core/Renderable.hpp"
#include "renderer/PrimitiveMode.hpp"

namespace threecpp {

enum class RenderQueueBucket { Opaque, Transmissive, Transparent };

// Mirrors the essential three.js render item fields used for stable sorting.
// groupStart/groupCount are already intersected with BufferGeometry::drawRange
// before an item is inserted.
struct RenderItem {
    Object3D* object = nullptr;
    BufferGeometry* geometry = nullptr;
    Material* material = nullptr;
    PrimitiveMode primitiveMode = PrimitiveMode::Triangles;
    RenderQueueBucket bucket = RenderQueueBucket::Opaque;
    float z = 0.0f;
    float renderOrder = 0.0f;
    ObjectId objectId = 0;
    ObjectId geometryId = 0;
    ObjectId materialId = 0;
    std::uint64_t materialVersion = 0;
    std::uint64_t sortKey = 0;
    bool receivesTransmissionBackground = false;
    bool writesDepth = true;
    bool alphaTested = false;
    int groupStart = 0;
    int groupCount = -1;
    int materialIndex = 0;
};

struct RenderListStats {
    int opaque = 0;
    int transmissive = 0;
    int transparent = 0;
    int total() const { return opaque + transmissive + transparent; }
};

class RenderList {
public:
    std::vector<RenderItem> opaque;
    std::vector<RenderItem> transmissive;
    std::vector<RenderItem> transparent;

    void clear();
    void push(Object3D& object, BufferGeometry& geometry, Material& material, PrimitiveMode mode, float z, int groupStart = 0, int groupCount = -1, int materialIndex = 0);
    void sort(bool sortObjects = true);
    RenderListStats stats() const;

    static bool isTransmissiveMaterial(const Material& material);
    static bool isTransparentMaterial(const Material& material);
    static RenderQueueBucket classify(const Material& material);
};

PrimitiveMode primitive_for_object(const Object3D& object);

} // namespace threecpp
