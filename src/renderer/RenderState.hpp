#pragma once
#include "common.hpp"
#include "core/Scene.hpp"
#include "core/Camera.hpp"
#include "renderer/RenderList.hpp"

namespace threecpp {

struct ViewportRect {
    int x = 0;
    int y = 0;
    int width = 1;
    int height = 1;
};

struct ScissorRect {
    int x = 0;
    int y = 0;
    int width = 1;
    int height = 1;
    bool enabled = false;
};

struct RenderState {
    Scene* scene = nullptr;
    Camera* camera = nullptr;
    ViewportRect viewport;
    ScissorRect scissor;
    RenderListStats listStats;
    std::uint32_t cameraLayers = 1u;

    void reset(Scene& s, Camera& c) {
        scene = &s;
        camera = &c;
        cameraLayers = c.layers;
        listStats = {};
    }
};

class RenderStateStack {
    std::vector<RenderState> stack;
public:
    RenderState& push(Scene& scene, Camera& camera) {
        stack.emplace_back();
        stack.back().reset(scene, camera);
        return stack.back();
    }
    void pop() { if (!stack.empty()) stack.pop_back(); }
    RenderState* current() { return stack.empty() ? nullptr : &stack.back(); }
    const RenderState* current() const { return stack.empty() ? nullptr : &stack.back(); }
};

} // namespace threecpp
