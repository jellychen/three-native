#pragma once
#include "postprocessing/EffectComposer.hpp"
#include "material/Material.hpp"

namespace threecpp {

class ShaderPass : public Pass {
public:
    std::string name = "ShaderPass";
    std::shared_ptr<ShaderMaterial> material;
    explicit ShaderPass(std::shared_ptr<ShaderMaterial> mat = {}) : material(std::move(mat)) {}
    void render(GLRenderer& renderer, Scene& scene, Camera& camera) override {
        // Placeholder pass: v3.3 defines the API and keeps the render chain valid.
        // A full-screen triangle implementation can replace this without changing examples.
        renderer.render(scene, camera);
    }
};

class ToneMappingPass : public Pass {
public:
    ToneMapping toneMapping = ToneMapping::ACESFilmic;
    float exposure = 1.0f;
    ToneMappingPass(ToneMapping mode = ToneMapping::ACESFilmic, float exposure = 1.0f) : toneMapping(mode), exposure(exposure) {}
    void render(GLRenderer& renderer, Scene& scene, Camera& camera) override {
        renderer.setToneMapping(toneMapping, exposure);
        renderer.render(scene, camera);
    }
};

class BloomPass : public Pass {
public:
    float threshold = 1.0f;
    float strength = 0.35f;
    float radius = 0.35f;
    BloomPass(float threshold = 1.0f, float strength = 0.35f, float radius = 0.35f)
        : threshold(threshold), strength(strength), radius(radius) {}
    void render(GLRenderer& renderer, Scene& scene, Camera& camera) override {
        renderer.render(scene, camera);
    }
};

class OutlinePass : public Pass {
public:
    glm::vec3 visibleEdgeColor{1.0f, 0.72f, 0.22f};
    float edgeStrength = 2.5f;
    void render(GLRenderer& renderer, Scene& scene, Camera& camera) override {
        renderer.render(scene, camera);
    }
};

class FXAAPass : public Pass {
public:
    void render(GLRenderer& renderer, Scene& scene, Camera& camera) override { renderer.render(scene, camera); }
};

} // namespace threecpp
