#pragma once
#include "platform/window.h"
#include "core/camera.h"

namespace THREE {

// Minimal three.js-style orbit control for examples/model viewer.
// - Left mouse drag: orbit around target.
// - Right mouse drag or middle mouse drag: pan target in camera plane.
// - Mouse wheel: dolly in/out by changing spherical radius.
// This class intentionally stays outside GLRenderer; it only mutates the camera transform.
class OrbitControls {
public:
    OrbitControls(PerspectiveCamera& camera, Window& window);
    OrbitControls(Window& window, PerspectiveCamera& camera) : OrbitControls(camera, window) {}

    glm::vec3 target{0.0f, 0.0f, 0.0f};
    bool enabled = true;
    bool enableRotate = true;
    bool enableZoom = true;
    bool enablePan = true;

    float minDistance = 0.05f;
    float maxDistance = 1000.0f;
    float rotateSpeed = 0.008f;
    float zoomSpeed = 0.12f;
    float panSpeed = 1.0f;
    float minPolarAngle = 0.001f;
    float maxPolarAngle = glm::pi<float>() - 0.001f;

    // three.js-style public distance property. Assigning controls.distance = x
    // is applied on the next update(); setDistance(x) applies immediately.
    float distance = 1.0f;

    void setTarget(const glm::vec3& value);
    void setDistance(float value);
    float getDistance() const { return distance; }
    void update();

private:
    PerspectiveCamera* camera_ = nullptr;
    Window* window_ = nullptr;
    bool initialized_ = false;
    bool dragging_ = false;
    glm::dvec2 lastCursor_{0.0, 0.0};
    float theta_ = 0.0f; // azimuth around Y
    float phi_ = glm::half_pi<float>(); // polar from Y+

    void syncFromCamera();
    void applyToCamera();
    void rotate(float dx, float dy);
    void dolly(float wheelY);
    void pan(float dx, float dy);
};

} // namespace THREE
