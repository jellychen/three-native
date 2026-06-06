#pragma once
#include "renderer/GLRenderer.hpp"

namespace threecpp {

class Pass {
public:
    bool enabled = true;
    bool needsSwap = true;
    virtual ~Pass() = default;
    virtual void render(GLRenderer& renderer, Scene& scene, Camera& camera) = 0;
};

class RenderPass : public Pass {
public:
    void render(GLRenderer& renderer, Scene& scene, Camera& camera) override { renderer.render(scene, camera); }
};

class EffectComposer {
public:
    explicit EffectComposer(GLRenderer& renderer) : renderer(renderer) {}
    template <class T, class... Args>
    T& addPass(Args&&... args) {
        auto pass = std::make_unique<T>(std::forward<Args>(args)...);
        auto& ref = *pass;
        passes.push_back(std::move(pass));
        return ref;
    }
    void render(Scene& scene, Camera& camera) {
        for (auto& pass : passes) if (pass->enabled) pass->render(renderer, scene, camera);
    }
private:
    GLRenderer& renderer;
    std::vector<std::unique_ptr<Pass>> passes;
};

} // namespace threecpp
