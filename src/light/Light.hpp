#pragma once
#include "core/Object3D.hpp"
#include "texture/Texture.hpp"

namespace threecpp {

enum class LightType { Ambient, Hemisphere, Directional, Point, Spot, RectArea };

struct LightShadow {
    bool enabled = false;
    // three.js-style mapSize plus legacy X/Y fields used by older examples.
    // GLShadowMap resolves both so examples can write either:
    //   shadow.mapSize = {2048, 2048};
    // or
    //   shadow.mapSizeX = 2048; shadow.mapSizeY = 2048;
    glm::ivec2 mapSize{1024, 1024};
    int mapSizeX = 1024;
    int mapSizeY = 1024;
    float bias = 0.0005f;
    float normalBias = 0.0f;
    float radius = 1.0f;
    float intensity = 1.0f; // three.js-style shadow strength, 0 disables darkening, 1 full shadow.

    // Directional-light shadow camera, matching the practical three.js knobs.
    // These are intentionally stored on the shadow object so tests can tune the
    // shadow frustum without modifying the light transform.
    float cameraLeft = -20.0f;
    float cameraRight = 20.0f;
    float cameraBottom = -20.0f;
    float cameraTop = 20.0f;
    float cameraNear = 0.1f;
    float cameraFar = 200.0f;

    glm::mat4 matrix{1.0f};
    std::shared_ptr<Texture> map;
};

class Light : public Object3D {
public:
    LightType lightType = LightType::Directional;
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    bool castShadow = false;
    LightShadow shadow;
    Light() { kind = ObjectKind::Light; }
};

class AmbientLight : public Light {
public:
    AmbientLight() { lightType = LightType::Ambient; }
};

class HemisphereLight : public Light {
public:
    // three.js names this constructor parameter/semantic as skyColor.
    // Keep Light::color for existing code, but expose skyColor so examples and
    // importers can use the familiar API. Renderer prefers skyColor.
    glm::vec3 skyColor{1.0f};
    glm::vec3 groundColor{0.5f};
    HemisphereLight() { lightType = LightType::Hemisphere; }
};

class DirectionalLight : public Light {
public:
    glm::vec3 target{0.0f};
    DirectionalLight() { lightType = LightType::Directional; }
};

class PointLight : public Light {
public:
    float distance = 0.0f; // 0 means infinite, same convention as three.js.
    float decay = 2.0f;    // physically-correct inverse-square by default.
    PointLight() { lightType = LightType::Point; }
};

class SpotLight : public Light {
public:
    float distance = 0.0f; // 0 means infinite cutoff.
    float angle = glm::radians(30.0f); // outer cone half-angle.
    float penumbra = 0.0f; // 0 hard edge, 1 very soft edge.
    float decay = 2.0f;
    glm::vec3 target{0.0f};
    SpotLight() { lightType = LightType::Spot; }
};

class RectAreaLight : public Light {
public:
    float width = 10.0f;
    float height = 10.0f;
    glm::vec3 target{0.0f};
    RectAreaLight() { lightType = LightType::RectArea; }
};

class LightProbe : public Light {
public:
    // Lightweight spherical-harmonics placeholder. The renderer currently folds
    // coefficient 0 into ambient lighting, so scenes can already import and
    // preserve probe intent while LTC/SH evaluation is expanded later.
    std::array<glm::vec3, 9> sh{};
    LightProbe() { lightType = LightType::Ambient; }
};

struct GpuLight {
    int type = 0;
    int shadowIndex = -1;
    glm::vec2 padding0{0.0f};
    glm::vec4 colorIntensity{1,1,1,1};
    glm::vec4 positionRange{0,0,0,0};
    glm::vec4 directionCone{0,-1,0,0};
    glm::vec4 params{0,0,0,0};
};

class LightCache {
public:
    glm::vec3 ambient{0.0f};
    glm::vec3 hemisphereSky{0.0f};
    glm::vec3 hemisphereGround{0.0f};
    std::vector<GpuLight> gpuLights;
    void reset() { ambient = {}; hemisphereSky = {}; hemisphereGround = {}; gpuLights.clear(); }
};

} // namespace threecpp
