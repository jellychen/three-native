#pragma once
#include "core/object-3d.h"
#include "ibl/environment.h"
#include "material/material.h"
#include <initializer_list>
#include <variant>

namespace THREE {

struct Fog {
    glm::vec3 color{0.62f, 0.62f, 0.62f};
    float near = 1.0f;
    float far = 1000.0f;
    bool isFogExp2 = false;
};

struct FogExp2 {
    glm::vec3 color{0.62f, 0.62f, 0.62f};
    float density = 0.00025f;
    bool isFogExp2 = true;
};

using FogVariant = std::variant<Fog, FogExp2>;

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
    std::shared_ptr<Material> overrideMaterial;
    float environmentIntensity = 1.0f;
    std::shared_ptr<FogVariant> fog;
    Scene() { kind = ObjectKind::Scene; }
};

} // namespace THREE
