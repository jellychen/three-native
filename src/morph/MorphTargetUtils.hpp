#pragma once
#include "core/Renderable.hpp"

namespace threecpp {

// CPU-side utilities that mirror the data semantics used by three.js morph targets.
// They are intentionally backend-independent, so the renderer, shadow pass,
// validation tools, and tests can all agree on how absolute/relative morph data is
// evaluated before more advanced GPU texture morph fallback is enabled.
class MorphTargetUtils {
public:
    static int targetCount(const BufferGeometry& geometry) {
        return std::max({geometry.morphTargetCount("position"), geometry.morphTargetCount("normal"), geometry.morphTargetCount("color"), geometry.morphTargetCount("tangent")});
    }

    static std::vector<float> normalizedInfluences(const Mesh& mesh) {
        const int n = mesh.geometry ? targetCount(*mesh.geometry) : static_cast<int>(mesh.morphTargetInfluences.size());
        std::vector<float> out(static_cast<std::size_t>(std::max(0, n)), 0.0f);
        for (std::size_t i = 0; i < out.size() && i < mesh.morphTargetInfluences.size(); ++i) out[i] = mesh.morphTargetInfluences[i];
        return out;
    }

    static std::vector<int> topInfluenceIndices(const Mesh& mesh, int maxCount) {
        std::vector<float> influences = normalizedInfluences(mesh);
        std::vector<int> ids(influences.size());
        for (std::size_t i = 0; i < ids.size(); ++i) ids[i] = static_cast<int>(i);
        std::stable_sort(ids.begin(), ids.end(), [&](int a, int b) { return std::abs(influences[static_cast<std::size_t>(a)]) > std::abs(influences[static_cast<std::size_t>(b)]); });
        if (maxCount >= 0 && static_cast<int>(ids.size()) > maxCount) ids.resize(static_cast<std::size_t>(maxCount));
        return ids;
    }

    static glm::vec3 evaluateVec3(const BufferGeometry& geometry, std::string_view attributeName, int vertexIndex, const std::vector<float>& influences, const glm::vec3& base) {
        auto targets = geometry.getMorphAttributes(attributeName);
        if (!targets) return base;
        glm::vec3 result = base;
        const int n = std::min(static_cast<int>(targets->size()), static_cast<int>(influences.size()));
        for (int i = 0; i < n; ++i) {
            float w = influences[static_cast<std::size_t>(i)];
            if (w == 0.0f) continue;
            const BufferAttribute& attr = (*targets)[static_cast<std::size_t>(i)];
            if (attr.type != AttributeType::Float32 || attr.itemSize < 3 || vertexIndex < 0 || vertexIndex >= attr.count) continue;
            auto s = attr.asSpan<float>();
            const std::size_t off = static_cast<std::size_t>(vertexIndex * attr.itemSize);
            glm::vec3 v{s[off + 0], s[off + 1], s[off + 2]};
            result += geometry.morphTargetsRelative ? v * w : (v - base) * w;
        }
        return result;
    }

    static void bakeActiveMorphsToBase(Mesh& mesh) {
        if (!mesh.geometry) return;
        auto influences = normalizedInfluences(mesh);
        auto* pos = mesh.geometry->getAttribute("position");
        if (pos && pos->type == AttributeType::Float32 && pos->itemSize >= 3) {
            std::vector<float> values(pos->asSpan<float>().begin(), pos->asSpan<float>().end());
            for (int i = 0; i < pos->count; ++i) {
                const std::size_t off = static_cast<std::size_t>(i * pos->itemSize);
                glm::vec3 base{values[off + 0], values[off + 1], values[off + 2]};
                glm::vec3 morphed = evaluateVec3(*mesh.geometry, "position", i, influences, base);
                values[off + 0] = morphed.x; values[off + 1] = morphed.y; values[off + 2] = morphed.z;
            }
            *pos = BufferAttribute::fromVector(values, pos->itemSize, AttributeType::Float32, pos->normalized);
        }
        auto* nrm = mesh.geometry->getAttribute("normal");
        if (nrm && nrm->type == AttributeType::Float32 && nrm->itemSize >= 3) {
            std::vector<float> values(nrm->asSpan<float>().begin(), nrm->asSpan<float>().end());
            for (int i = 0; i < nrm->count; ++i) {
                const std::size_t off = static_cast<std::size_t>(i * nrm->itemSize);
                glm::vec3 base{values[off + 0], values[off + 1], values[off + 2]};
                glm::vec3 morphed = glm::normalize(evaluateVec3(*mesh.geometry, "normal", i, influences, base));
                values[off + 0] = morphed.x; values[off + 1] = morphed.y; values[off + 2] = morphed.z;
            }
            *nrm = BufferAttribute::fromVector(values, nrm->itemSize, AttributeType::Float32, nrm->normalized);
        }
        std::fill(mesh.morphTargetInfluences.begin(), mesh.morphTargetInfluences.end(), 0.0f);
        mesh.geometry->markNeedsUpdate();
    }
};

} // namespace threecpp
