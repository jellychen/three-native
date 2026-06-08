#pragma once
#include "common.h"
#include "material/material.h"
#include "renderer/primitive-mode.h"
#include "platform/gl-headers.h"

namespace THREE {

struct ProgramKey {
    MaterialType materialType = MaterialType::MeshBasic;
    PrimitiveMode primitiveMode = PrimitiveMode::Triangles;
    bool useMap = false;
    bool useAlphaMap = false;
    bool useVertexColor = false;
    bool hasUv2 = false;
    bool useNormalMap = false;
    bool useBumpMap = false;
    bool useDisplacementMap = false;
    bool useFlatShading = false;
    bool usePremultipliedAlpha = false;
    bool useInstancing = false;
    bool useInstanceColor = false;
    bool useRoughnessMap = false;
    bool useMetalnessMap = false;
    bool useAOMap = false;
    bool useLightMap = false;
    bool useEmissiveMap = false;
    bool useSkinning = false;
    bool useMorphTargets = false;
    bool useMorphNormals = false;
    bool morphTargetsRelative = false;
    int morphTargetCount = 0;
    bool useIBL = false;
    bool useEnvMapEquirect = false;
    bool usePMREM = false;
    bool useShadowMap = false;
    bool usePBRDirectionalShadow = false;
    bool usePBRSpotShadow = false;
    bool usePBRPointShadow = false;
    bool useDashedLine = false;
    bool useSizeAttenuation = false;
    bool useClipping = false;

    bool usePhysical = false;
    bool useTransmission = false;
    bool useTransmissionMap = false;
    bool useTransmissionRenderTarget = false;
    bool useThicknessMap = false;
    bool useSpecularMap = false;
    bool useClearcoat = false;
    bool useClearcoatMap = false;
    bool useClearcoatRoughnessMap = false;
    bool useClearcoatNormalMap = false;
    bool useSheen = false;
    bool useSheenColorMap = false;
    bool useSheenRoughnessMap = false;
    bool useIridescence = false;
    bool useIridescenceMap = false;
    bool useIridescenceThicknessMap = false;
    bool useAnisotropy = false;
    bool useAnisotropyMap = false;
    bool useDispersion = false;

    int numDirLights = 0;
    int numPointLights = 0;
    int numSpotLights = 0;

    bool operator==(const ProgramKey& o) const = default;
};

struct ProgramKeyHash {
    std::size_t operator()(const ProgramKey& k) const noexcept {
        std::size_t h = static_cast<std::size_t>(k.materialType) * 131 + static_cast<std::size_t>(k.primitiveMode);
        auto mix = [&](auto v) { h = h * 1315423911u + static_cast<std::size_t>(v) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2); };
        mix(k.useMap); mix(k.useAlphaMap); mix(k.useVertexColor); mix(k.hasUv2); mix(k.useInstancing); mix(k.useInstanceColor); mix(k.useNormalMap); mix(k.useBumpMap); mix(k.useDisplacementMap); mix(k.useFlatShading); mix(k.usePremultipliedAlpha); mix(k.useRoughnessMap); mix(k.useMetalnessMap); mix(k.useAOMap); mix(k.useLightMap); mix(k.useEmissiveMap);
        mix(k.useSkinning); mix(k.useMorphTargets); mix(k.useMorphNormals); mix(k.morphTargetsRelative); mix(k.morphTargetCount); mix(k.useIBL); mix(k.useEnvMapEquirect); mix(k.usePMREM); mix(k.useShadowMap); mix(k.usePBRDirectionalShadow); mix(k.usePBRSpotShadow); mix(k.usePBRPointShadow); mix(k.useDashedLine); mix(k.useSizeAttenuation);
        mix(k.usePhysical); mix(k.useTransmission); mix(k.useTransmissionMap); mix(k.useTransmissionRenderTarget); mix(k.useThicknessMap); mix(k.useSpecularMap);
        mix(k.useClearcoat); mix(k.useClearcoatMap); mix(k.useClearcoatRoughnessMap); mix(k.useClearcoatNormalMap);
        mix(k.useSheen); mix(k.useSheenColorMap); mix(k.useSheenRoughnessMap);
        mix(k.useIridescence); mix(k.useIridescenceMap); mix(k.useIridescenceThicknessMap);
        mix(k.useAnisotropy); mix(k.useAnisotropyMap); mix(k.useDispersion);
        mix(k.numDirLights); mix(k.numPointLights); mix(k.numSpotLights);
        return h;
    }
};

class GLProgram {
public:
    GLuint id = 0;
    ProgramKey key;
    std::unordered_map<std::string, GLint> uniforms;

    GLProgram() = default;
    GLProgram(ProgramKey key, std::string_view vertex, std::string_view fragment);
    GLProgram(const GLProgram&) = delete;
    GLProgram& operator=(const GLProgram&) = delete;
    GLProgram(GLProgram&& other) noexcept;
    GLProgram& operator=(GLProgram&& other) noexcept;
    ~GLProgram();

    GLint uniform(const std::string& name);

private:
    GLuint compile(GLenum type, std::string_view src);
};

} // namespace THREE
