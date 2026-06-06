#pragma once
#include "core/Object3D.hpp"

namespace threecpp {

class Camera : public Object3D {
public:
    glm::mat4 matrixWorldInverse{1.0f};
    glm::mat4 projectionMatrix{1.0f};
    glm::mat4 projectionMatrixInverse{1.0f};

    Camera() { kind = ObjectKind::Camera; }
    void updateMatrixWorld(bool force = false) override;
    virtual void updateProjectionMatrix() {}
};

class PerspectiveCamera : public Camera {
public:
    float fov = 50.0f;
    float aspect = 1.0f;
    float nearPlane = 0.1f;
    float farPlane = 2000.0f;

    PerspectiveCamera(float fovDegrees = 50.0f, float aspectRatio = 1.0f, float nearZ = 0.1f, float farZ = 2000.0f);
    void updateProjectionMatrix() override;
};

class OrthographicCamera : public Camera {
public:
    float left = -1, right = 1, top = 1, bottom = -1, nearPlane = 0.1f, farPlane = 2000.0f;
    OrthographicCamera(float left, float right, float top, float bottom, float nearZ, float farZ);
    void updateProjectionMatrix() override;
};

} // namespace threecpp
