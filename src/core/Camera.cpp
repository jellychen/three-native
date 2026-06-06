#include "core/Camera.hpp"

namespace threecpp {

void Camera::updateMatrixWorld(bool force) {
    Object3D::updateMatrixWorld(force);
    matrixWorldInverse = glm::inverse(matrixWorld);
}

PerspectiveCamera::PerspectiveCamera(float fovDegrees, float aspectRatio, float nearZ, float farZ) {
    fov = fovDegrees; aspect = aspectRatio; nearPlane = nearZ; farPlane = farZ;
    updateProjectionMatrix();
}

void PerspectiveCamera::updateProjectionMatrix() {
    projectionMatrix = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
    projectionMatrixInverse = glm::inverse(projectionMatrix);
}

OrthographicCamera::OrthographicCamera(float l, float r, float t, float b, float n, float f) {
    left = l; right = r; top = t; bottom = b; nearPlane = n; farPlane = f;
    updateProjectionMatrix();
}

void OrthographicCamera::updateProjectionMatrix() {
    projectionMatrix = glm::ortho(left, right, bottom, top, nearPlane, farPlane);
    projectionMatrixInverse = glm::inverse(projectionMatrix);
}

} // namespace threecpp
