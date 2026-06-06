#include "controls/OrbitControls.hpp"

namespace threecpp {

OrbitControls::OrbitControls(PerspectiveCamera& camera, Window& window)
    : camera_(&camera), window_(&window) {
    syncFromCamera();
    lastCursor_ = window_->cursorPosition();
}

void OrbitControls::setTarget(const glm::vec3& value) {
    target = value;
    syncFromCamera();
    applyToCamera();
}

void OrbitControls::setDistance(float value) {
    distance = glm::clamp(value, minDistance, maxDistance);
    applyToCamera();
}

void OrbitControls::syncFromCamera() {
    if (!camera_) return;
    glm::vec3 offset = camera_->position - target;
    distance = glm::length(offset);
    if (distance <= std::numeric_limits<float>::epsilon()) {
        distance = minDistance;
        theta_ = 0.0f;
        phi_ = glm::half_pi<float>();
        return;
    }
    theta_ = std::atan2(offset.x, offset.z);
    phi_ = std::acos(glm::clamp(offset.y / distance, -1.0f, 1.0f));
    phi_ = glm::clamp(phi_, minPolarAngle, maxPolarAngle);
    distance = glm::clamp(distance, minDistance, maxDistance);
}

void OrbitControls::applyToCamera() {
    if (!camera_) return;
    const float sinPhi = std::sin(phi_);
    glm::vec3 offset{
        distance * sinPhi * std::sin(theta_),
        distance * std::cos(phi_),
        distance * sinPhi * std::cos(theta_)
    };
    camera_->position = target + offset;
    camera_->lookAt(target);
    camera_->matrixWorldNeedsUpdate = true;
}

void OrbitControls::rotate(float dx, float dy) {
    theta_ -= dx * rotateSpeed;
    phi_ -= dy * rotateSpeed;
    phi_ = glm::clamp(phi_, minPolarAngle, maxPolarAngle);
}

void OrbitControls::dolly(float wheelY) {
    if (std::abs(wheelY) <= std::numeric_limits<float>::epsilon()) return;
    const float factor = std::pow(1.0f - glm::clamp(zoomSpeed, 0.01f, 0.95f), wheelY);
    distance = glm::clamp(distance * factor, minDistance, maxDistance);
}

void OrbitControls::pan(float dx, float dy) {
    if (!camera_) return;
    glm::vec3 forward = glm::normalize(target - camera_->position);
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    if (!std::isfinite(right.x) || glm::dot(right, right) < 1e-6f) right = {1.0f, 0.0f, 0.0f};
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    auto fb = window_->framebufferSize();
    const float worldPerPixel = 2.0f * distance * std::tan(glm::radians(camera_->fov) * 0.5f) / float(std::max(fb.y, 1));
    target += (-right * dx + up * dy) * worldPerPixel * panSpeed;
}

void OrbitControls::update() {
    if (!enabled || !camera_ || !window_) return;

    auto fb = window_->framebufferSize();
    camera_->aspect = float(fb.x) / float(std::max(fb.y, 1));
    camera_->updateProjectionMatrix();

    const glm::dvec2 cursor = window_->cursorPosition();
    if (!initialized_) {
        lastCursor_ = cursor;
        initialized_ = true;
    }

    const glm::dvec2 delta = cursor - lastCursor_;
    lastCursor_ = cursor;

    const bool left = window_->mouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
    const bool middle = window_->mouseButtonPressed(GLFW_MOUSE_BUTTON_MIDDLE);
    const bool right = window_->mouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);

    if (left && enableRotate) {
        rotate(static_cast<float>(delta.x), static_cast<float>(delta.y));
    } else if ((middle || right) && enablePan) {
        pan(static_cast<float>(delta.x), static_cast<float>(delta.y));
    }

    const glm::dvec2 scroll = window_->consumeScrollDelta();
    if (enableZoom) {
        dolly(static_cast<float>(scroll.y));
    }

    applyToCamera();
}

} // namespace threecpp
