#pragma once
#include "common.h"

namespace THREE {

enum class ObjectKind {
    Object3D,
    Scene,
    Camera,
    Mesh,
    InstancedMesh,
    SkinnedMesh,
    Line,
    LineSegments,
    LineLoop,
    FatLine,
    FatLineSegments,
    Points,
    Sprite,
    Bone,
    Light
};

class Object3D : public std::enable_shared_from_this<Object3D> {
public:
    ObjectId id = next_object_id();
    std::string name;
    ObjectKind kind = ObjectKind::Object3D;

    glm::vec3 position{0.0f};
    // Euler rotation in radians, XYZ order. Kept for three.js-style example code.
    // Quaternion remains the primary orientation path for animation/lookAt; updateMatrix() composes both.
    glm::vec3 rotation{0.0f};
    glm::quat quaternion{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};

    glm::mat4 matrix{1.0f};
    glm::mat4 matrixWorld{1.0f};

    bool visible = true;
    bool matrixAutoUpdate = true;
    bool matrixWorldNeedsUpdate = true;
    bool frustumCulled = true;
    float renderOrder = 0.0f;
    std::uint32_t layers = 1u;

    bool testLayers(const Object3D& camera) const { return (layers & camera.layers) != 0u; }

    // three.js owns children from the parent side, but parent must be non-owning.
    // Store it as weak_ptr to avoid parent<->child reference cycles. For stack-allocated
    // roots used by examples, parentRaw is a non-owning fallback only; it never owns.
    std::weak_ptr<Object3D> parent;
    std::vector<std::shared_ptr<Object3D>> children;

    Object3D* parentObject() const noexcept;

private:
    Object3D* parentRaw = nullptr;

public:

    virtual ~Object3D() = default;

    void add(const std::shared_ptr<Object3D>& child);
    void remove(const std::shared_ptr<Object3D>& child);
    void clear();

    virtual void updateMatrix();
    virtual void updateMatrixWorld(bool force = false);
    void lookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));
    virtual void traverse(const std::function<void(Object3D&)>& visitor);
    virtual void traverseVisible(const std::function<void(Object3D&)>& visitor);
};

} // namespace THREE
