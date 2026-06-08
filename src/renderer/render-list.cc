#include "renderer/render-list.h"

namespace THREE {

namespace {
static bool is_transmissive_impl(const Material& material) {
    if (auto* physical = dynamic_cast<const MeshPhysicalMaterial*>(&material)) {
        return physical->transmission > 0.0f || physical->transmissionMap || physical->thickness > 0.0f || physical->thicknessMap;
    }
    return false;
}

static bool is_transparent_impl(const Material& material) {
    // three.js default material.blending is NormalBlending, but that alone does
    // NOT make a material part of the transparent render list. Only explicit
    // material.transparent, opacity < 1, or non-normal/custom blend modes should
    // enter the transparent queue. Keeping all default PBR/Basic materials in
    // the transparent queue caused later examples to render only the background
    // on macOS when combined with shadow/transmission state changes.
    if (material.alphaTest > 0.0f && !material.transparent && material.opacity >= 1.0f) return false;
    if (material.transparent || material.opacity < 1.0f) return true;
    return material.blending != Blending::None && material.blending != Blending::Normal;
}

static std::uint64_t make_sort_key(const RenderItem& item) {
    std::uint64_t k = 0;
    k ^= (static_cast<std::uint64_t>(item.material->type) & 0xffu) << 56u;
    k ^= (static_cast<std::uint64_t>(item.primitiveMode) & 0xffu) << 48u;
    k ^= (item.materialId & 0xffffu) << 24u;
    k ^= (item.geometryId & 0xffffu);
    return k;
}

static bool opaque_less(const RenderItem& a, const RenderItem& b) {
    if (a.renderOrder != b.renderOrder) return a.renderOrder < b.renderOrder;
    if (a.sortKey != b.sortKey) return a.sortKey < b.sortKey;
    if (a.z != b.z) return a.z > b.z; // camera-space z is usually negative; greater is nearer.
    return a.objectId < b.objectId;
}

static bool transparent_less(const RenderItem& a, const RenderItem& b) {
    if (a.renderOrder != b.renderOrder) return a.renderOrder < b.renderOrder;
    // Back-to-front for camera-space z. More negative is farther.
    if (a.z != b.z) return a.z < b.z;
    if (a.materialId != b.materialId) return a.materialId < b.materialId;
    return a.objectId < b.objectId;
}
}


void RenderList::clear() {
    opaque.clear();
    transmissive.clear();
    transparent.clear();
}

bool RenderList::isTransmissiveMaterial(const Material& material) {
    return is_transmissive_impl(material);
}

bool RenderList::isTransparentMaterial(const Material& material) {
    return is_transparent_impl(material);
}

RenderQueueBucket RenderList::classify(const Material& material) {
    if (is_transmissive_impl(material)) return RenderQueueBucket::Transmissive;
    if (is_transparent_impl(material)) return RenderQueueBucket::Transparent;
    return RenderQueueBucket::Opaque;
}

void RenderList::push(Object3D& object, BufferGeometry& geometry, Material& material, PrimitiveMode mode, float z, int groupStart, int groupCount, int materialIndex) {
    RenderItem item;
    item.object = &object;
    item.geometry = &geometry;
    item.material = &material;
    item.primitiveMode = mode;
    item.bucket = classify(material);
    item.z = z;
    item.renderOrder = object.renderOrder;
    item.objectId = object.id;
    item.geometryId = geometry.id;
    item.materialId = material.id;
    item.materialVersion = material.version;
    item.receivesTransmissionBackground = is_transmissive_impl(material);
    item.writesDepth = material.depthWrite;
    item.alphaTested = material.alphaTest > 0.0f;
    item.groupStart = std::max(0, groupStart);
    item.groupCount = groupCount;
    item.materialIndex = materialIndex;
    item.sortKey = make_sort_key(item);
    switch (item.bucket) {
        case RenderQueueBucket::Opaque: opaque.push_back(item); break;
        case RenderQueueBucket::Transmissive: transmissive.push_back(item); break;
        case RenderQueueBucket::Transparent: transparent.push_back(item); break;
    }
}

void RenderList::sort(bool sortObjects) {
    if (!sortObjects) return;
    std::stable_sort(opaque.begin(), opaque.end(), opaque_less);
    std::stable_sort(transmissive.begin(), transmissive.end(), transparent_less);
    std::stable_sort(transparent.begin(), transparent.end(), transparent_less);
}

RenderListStats RenderList::stats() const {
    return {static_cast<int>(opaque.size()), static_cast<int>(transmissive.size()), static_cast<int>(transparent.size())};
}

PrimitiveMode primitive_for_object(const Object3D& object) {
    switch (object.kind) {
        case ObjectKind::Line: return PrimitiveMode::LineStrip;
        case ObjectKind::LineSegments: return PrimitiveMode::Lines;
        case ObjectKind::LineLoop: return PrimitiveMode::LineLoop;
        case ObjectKind::Points: return PrimitiveMode::Points;
        case ObjectKind::FatLine:
        case ObjectKind::FatLineSegments: return PrimitiveMode::FatLines;
        default: return PrimitiveMode::Triangles;
    }
}

} // namespace THREE
