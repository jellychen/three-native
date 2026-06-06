#include "core/Object3D.hpp"

namespace threecpp {

Object3D* Object3D::parentObject() const noexcept {
    if (auto p = parent.lock()) return p.get();
    return parentRaw;
}

void Object3D::add(const std::shared_ptr<Object3D>& child) {
    if (!child || child.get() == this) return;
    if (Object3D* oldParent = child->parentObject()) oldParent->remove(child);

    child->parentRaw = this;
    child->parent.reset();
    // weak_from_this() is valid when the parent itself is shared_ptr-owned.
    // Many examples keep Scene on the stack; parentRaw keeps that usage working
    // without making parent an owning raw pointer.
    if (auto self = weak_from_this(); !self.expired()) child->parent = self;

    children.push_back(child);
}

void Object3D::remove(const std::shared_ptr<Object3D>& child) {
    if (!child) return;
    auto it = std::remove(children.begin(), children.end(), child);
    if (it != children.end()) {
        child->parent.reset();
        child->parentRaw = nullptr;
        children.erase(it, children.end());
    }
}

void Object3D::clear() {
    for (auto& c : children) {
        c->parent.reset();
        c->parentRaw = nullptr;
    }
    children.clear();
}

void Object3D::updateMatrix() {
    const glm::quat eulerQuat = glm::quat(rotation);
    const glm::quat composedRotation = quaternion * eulerQuat;
    matrix = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(composedRotation) * glm::scale(glm::mat4(1.0f), scale);
    matrixWorldNeedsUpdate = true;
}


void Object3D::lookAt(const glm::vec3& target, const glm::vec3& up) {
    // Match the usual camera/object convention used by three.js: local -Z points at target.
    const glm::vec3 eye = position;
    glm::vec3 forward = target - eye;
    if (glm::dot(forward, forward) <= std::numeric_limits<float>::epsilon()) {
        return;
    }

    glm::mat4 view = glm::lookAt(eye, target, up);
    glm::mat4 world = glm::inverse(view);
    quaternion = glm::quat_cast(world);
    matrixWorldNeedsUpdate = true;
}

void Object3D::updateMatrixWorld(bool force) {
    if (matrixAutoUpdate) updateMatrix();
    if (matrixWorldNeedsUpdate || force) {
        if (Object3D* p = parentObject()) matrixWorld = p->matrixWorld * matrix;
        else matrixWorld = matrix;
        matrixWorldNeedsUpdate = false;
        force = true;
    }
    for (auto& child : children) child->updateMatrixWorld(force);
}

void Object3D::traverse(const std::function<void(Object3D&)>& visitor) {
    visitor(*this);
    for (auto& child : children) child->traverse(visitor);
}

void Object3D::traverseVisible(const std::function<void(Object3D&)>& visitor) {
    if (!visible) return;
    visitor(*this);
    for (auto& child : children) child->traverseVisible(visitor);
}

} // namespace threecpp
