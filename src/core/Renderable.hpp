#pragma once
#include "core/Object3D.hpp"
#include "geometry/BufferGeometry.hpp"
#include "material/Material.hpp"
#include "animation/Skeleton.hpp"

namespace threecpp {

class RenderableObject : public Object3D {
public:
    std::shared_ptr<BufferGeometry> geometry;
    std::shared_ptr<Material> material;
    std::vector<std::shared_ptr<Material>> materials;
    bool frustumCulled = true;
    int renderOrder = 0;

    RenderableObject(std::shared_ptr<BufferGeometry> g = {}, std::shared_ptr<Material> m = {})
        : geometry(std::move(g)), material(std::move(m)) {}

    void setMaterials(std::span<const std::shared_ptr<Material>> ms) {
        materials.assign(ms.begin(), ms.end());
        material = materials.empty() ? nullptr : materials.front();
    }

    void setMaterials(std::initializer_list<std::shared_ptr<Material>> ms) {
        setMaterials(std::span<const std::shared_ptr<Material>>(ms.begin(), ms.size()));
    }

    Material* materialAt(int index) const {
        if (!materials.empty()) {
            int i = glm::clamp(index, 0, static_cast<int>(materials.size()) - 1);
            return materials[static_cast<std::size_t>(i)].get();
        }
        return material.get();
    }

    std::shared_ptr<Material> materialRefAt(int index) const {
        if (!materials.empty()) {
            int i = glm::clamp(index, 0, static_cast<int>(materials.size()) - 1);
            return materials[static_cast<std::size_t>(i)];
        }
        return material;
    }
};

class Mesh : public RenderableObject {
public:
    bool castShadow = false;
    bool receiveShadow = false;

    // three.js-compatible morph target state. The dictionary maps target names
    // to indices in morphTargetInfluences. Renderer-side shader paths consume
    // the first N active influences supported by the backend.
    std::vector<float> morphTargetInfluences;
    std::unordered_map<std::string, int> morphTargetDictionary;
    bool morphTargetsNeedUpdate = true;
    bool morphTargetsUseCpuFallback = false;

    int morphTargetCount() const {
        return geometry ? std::max({geometry->morphTargetCount("position"), geometry->morphTargetCount("normal"), geometry->morphTargetCount("color"), geometry->morphTargetCount("tangent")}) : static_cast<int>(morphTargetInfluences.size());
    }

    Mesh(std::shared_ptr<BufferGeometry> g = {}, std::shared_ptr<Material> m = {}) : RenderableObject(std::move(g), std::move(m)) { kind = ObjectKind::Mesh; syncMorphTargets(); }

    void syncMorphTargets() {
        int count = geometry ? geometry->morphTargetCount("position") : 0;
        if (count <= 0 && geometry) count = geometry->morphTargetCount("normal");
        if (count > 0 && morphTargetInfluences.size() != static_cast<std::size_t>(count)) morphTargetInfluences.assign(static_cast<std::size_t>(count), 0.0f);
        morphTargetsNeedUpdate = false;
    }

    void setMorphTargetInfluence(int index, float value) {
        if (index < 0) return;
        syncMorphTargets();
        if (static_cast<std::size_t>(index) >= morphTargetInfluences.size()) morphTargetInfluences.resize(static_cast<std::size_t>(index) + 1, 0.0f);
        morphTargetInfluences[static_cast<std::size_t>(index)] = value;
        morphTargetsNeedUpdate = true;
    }

    bool setMorphTargetInfluence(const std::string& name, float value) {
        auto it = morphTargetDictionary.find(name);
        if (it == morphTargetDictionary.end()) return false;
        setMorphTargetInfluence(it->second, value);
        return true;
    }
};

class SkinnedMesh : public Mesh {
public:
    std::shared_ptr<Skeleton> skeleton;
    glm::mat4 bindMatrix{1.0f};
    glm::mat4 bindMatrixInverse{1.0f};
    SkinnedMesh(std::shared_ptr<BufferGeometry> g = {}, std::shared_ptr<Material> m = {}) : Mesh(std::move(g), std::move(m)) { kind = ObjectKind::SkinnedMesh; }
    void bind(const std::shared_ptr<Skeleton>& s, const glm::mat4& bind = glm::mat4(1.0f)) {
        skeleton = s; bindMatrix = bind; bindMatrixInverse = glm::inverse(bind);
    }
};

class Line : public RenderableObject {
public:
    Line(std::shared_ptr<BufferGeometry> g = {}, std::shared_ptr<Material> m = {}) : RenderableObject(std::move(g), std::move(m)) { kind = ObjectKind::Line; }
};

class LineSegments : public Line {
public:
    LineSegments(std::shared_ptr<BufferGeometry> g = {}, std::shared_ptr<Material> m = {}) : Line(std::move(g), std::move(m)) { kind = ObjectKind::LineSegments; }
};

class LineLoop : public Line {
public:
    LineLoop(std::shared_ptr<BufferGeometry> g = {}, std::shared_ptr<Material> m = {}) : Line(std::move(g), std::move(m)) { kind = ObjectKind::LineLoop; }
};

class Points : public RenderableObject {
public:
    Points(std::shared_ptr<BufferGeometry> g = {}, std::shared_ptr<Material> m = {}) : RenderableObject(std::move(g), std::move(m)) { kind = ObjectKind::Points; }
};

class FatLine : public RenderableObject {
public:
    FatLine(std::shared_ptr<BufferGeometry> g = {}, std::shared_ptr<Material> m = {}) : RenderableObject(std::move(g), std::move(m)) { kind = ObjectKind::FatLine; }
};

class FatLineSegments : public FatLine {
public:
    FatLineSegments(std::shared_ptr<BufferGeometry> g = {}, std::shared_ptr<Material> m = {}) : FatLine(std::move(g), std::move(m)) { kind = ObjectKind::FatLineSegments; }
};

// three.js examples compatibility aliases. Line2 / LineSegments2 render through
// the same FatLine path: each segment is expanded to two triangles and the
// shader controls screen-space/world-space width.
class Line2 : public FatLine {
public:
    Line2(std::shared_ptr<BufferGeometry> g = {}, std::shared_ptr<Material> m = {}) : FatLine(std::move(g), std::move(m)) { kind = ObjectKind::FatLine; }
};

class LineSegments2 : public FatLineSegments {
public:
    LineSegments2(std::shared_ptr<BufferGeometry> g = {}, std::shared_ptr<Material> m = {}) : FatLineSegments(std::move(g), std::move(m)) { kind = ObjectKind::FatLineSegments; }
};

class Sprite : public RenderableObject {
public:
    Sprite(std::shared_ptr<Material> m = {}) : RenderableObject(make_ref<BufferGeometry>(), std::move(m)) {
        kind = ObjectKind::Sprite;
        if (!material) material = make_ref<SpriteMaterial>();
        std::vector<float> p = {-0.5f,-0.5f,0, 0.5f,-0.5f,0, 0.5f,0.5f,0, -0.5f,0.5f,0};
        std::vector<float> n = {0,0,1,0,0,1,0,0,1,0,0,1};
        std::vector<float> uv = {0,0,1,0,1,1,0,1};
        geometry->setAttribute("position", BufferAttribute::fromVector(p, 3, AttributeType::Float32));
        geometry->setAttribute("normal", BufferAttribute::fromVector(n, 3, AttributeType::Float32));
        geometry->setAttribute("uv", BufferAttribute::fromVector(uv, 2, AttributeType::Float32));
        geometry->setIndex({0,1,2,0,2,3});
        geometry->computeBoundingSphere();
    }
};

class InstancedMesh : public Mesh {
public:
    std::vector<glm::mat4> instanceMatrices;
    std::vector<glm::vec4> instanceColors;
    int count = 0;
    bool instanceMatrixNeedsUpdate = true;
    bool instanceColorNeedsUpdate = true;
    std::uint64_t instanceMatrixVersion = 1;
    std::uint64_t instanceColorVersion = 1;

    InstancedMesh(std::shared_ptr<BufferGeometry> g = {}, std::shared_ptr<Material> m = {}, int instanceCount = 0)
        : Mesh(std::move(g), std::move(m)), count(instanceCount) {
        kind = ObjectKind::InstancedMesh;
        instanceMatrices.assign(static_cast<std::size_t>(std::max(0, instanceCount)), glm::mat4(1.0f));
        instanceColors.assign(static_cast<std::size_t>(std::max(0, instanceCount)), glm::vec4(1.0f));
    }

    void setMatrixAt(int index, const glm::mat4& matrix) {
        if (index < 0) return;
        if (index >= count) {
            count = index + 1;
            instanceMatrices.resize(static_cast<std::size_t>(count), glm::mat4(1.0f));
            instanceColors.resize(static_cast<std::size_t>(count), glm::vec4(1.0f));
        }
        instanceMatrices[static_cast<std::size_t>(index)] = matrix;
        instanceMatrixNeedsUpdate = true;
        ++instanceMatrixVersion;
    }

    void setColorAt(int index, const glm::vec4& color) {
        if (index < 0) return;
        if (index >= count) {
            count = index + 1;
            instanceMatrices.resize(static_cast<std::size_t>(count), glm::mat4(1.0f));
            instanceColors.resize(static_cast<std::size_t>(count), glm::vec4(1.0f));
        }
        instanceColors[static_cast<std::size_t>(index)] = color;
        instanceColorNeedsUpdate = true;
        ++instanceColorVersion;
    }
};

} // namespace threecpp
