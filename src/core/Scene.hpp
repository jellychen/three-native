#pragma once
#include "core/Object3D.hpp"
#include "ibl/Environment.hpp"
#include "material/Material.hpp"
#include <initializer_list>

namespace threecpp {

struct SceneBackgroundColor {
    glm::vec3 rgb{0.0f};
    float alpha = 1.0f;

    SceneBackgroundColor() = default;
    SceneBackgroundColor(const glm::vec3& color) : rgb(color), alpha(1.0f) {}
    SceneBackgroundColor(const glm::vec4& color) : rgb(color), alpha(color.a) {}

    SceneBackgroundColor& operator=(const glm::vec3& color) {
        rgb = color;
        return *this;
    }

    SceneBackgroundColor& operator=(const glm::vec4& color) {
        rgb = glm::vec3(color);
        alpha = color.a;
        return *this;
    }

    SceneBackgroundColor& operator=(std::initializer_list<float> values) {
        auto it = values.begin();
        if (it != values.end()) rgb.r = *it++;
        if (it != values.end()) rgb.g = *it++;
        if (it != values.end()) rgb.b = *it++;
        if (it != values.end()) alpha = *it++;
        return *this;
    }

    operator glm::vec3() const { return rgb; }
};

class Scene : public Object3D {
public:
    SceneBackgroundColor backgroundColor{};
    std::shared_ptr<Texture> background;
    std::shared_ptr<Environment> environment;
    // Same semantic as THREE.Scene.overrideMaterial: when set, every renderable
    // object in the normal scene render uses this material instead of its own
    // mesh/group material. Geometry groups still split draw calls, but each group
    // resolves to overrideMaterial.
    std::shared_ptr<Material> overrideMaterial;
    float environmentIntensity = 1.0f;
    Scene() { kind = ObjectKind::Scene; }
};

} // namespace threecpp
