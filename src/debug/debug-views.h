#pragma once
#include "common.h"
#include "core/scene.h"
#include "core/renderable.h"
#include "material/material.h"

namespace THREE {

enum class DebugViewMode {
    None,
    BaseColor,
    Normal,
    Roughness,
    Metalness,
    Wireframe,
    UV,
    LightComplexity,
    MorphInfluence,
    SkinWeights
};

struct DebugViewState {
    DebugViewMode mode = DebugViewMode::None;
    bool forceDoubleSide = false;
    bool forceWireframe = false;
    float normalScale = 1.0f;
};

class MaterialOverrideGuard {
public:
    struct Entry { Material* material = nullptr; bool wireframe = false; Side side = Side::FrontSide; };
    std::vector<Entry> entries;

    void apply(Scene& scene, const DebugViewState& state) {
        restore();
        scene.traverse([&](Object3D& object) {
            auto* renderable = dynamic_cast<RenderableObject*>(&object);
            if (!renderable || !renderable->material) return;
            auto* mat = renderable->material.get();
            entries.push_back({mat, mat->wireframe, mat->side});
            if (state.forceWireframe || state.mode == DebugViewMode::Wireframe) mat->wireframe = true;
            if (state.forceDoubleSide) mat->side = Side::DoubleSide;
        });
    }

    void restore() {
        for (auto& e : entries) {
            if (!e.material) continue;
            e.material->wireframe = e.wireframe;
            e.material->side = e.side;
        }
        entries.clear();
    }

    ~MaterialOverrideGuard() { restore(); }
};

} // namespace THREE
