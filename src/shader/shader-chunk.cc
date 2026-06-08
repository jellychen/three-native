#include "shader/shader-chunk.h"

namespace THREE {

std::vector<ShaderChunkInfo> ShaderChunk::registry() {
    return {
        {"common", ShaderStage::Both, "constants, saturate helpers, scalar channel helpers"},
        {"colorspace", ShaderStage::Fragment, "sRGB/linear conversion and output color space"},
        {"tonemapping", ShaderStage::Fragment, "linear/reinhard/ACES tone mapping and finalOutput"},
        {"packing", ShaderStage::Fragment, "scalar channel and depth packing helpers"},
        {"pbr_math", ShaderStage::Fragment, "GGX/Smith/Schlick BRDF primitives"},
        {"ibl", ShaderStage::Fragment, "hemisphere, equirectangular and PMREM sampling helpers"},
        {"shadow", ShaderStage::Fragment, "2D shadow maps, point cubemap shadows and PCF helpers"},
        {"normal_perturb", ShaderStage::Fragment, "normalMap and bumpMap tangent-space perturbation"},
        {"physical", ShaderStage::Fragment, "clearcoat, sheen, transmission, attenuation helpers"},
        {"morph_vertex", ShaderStage::Vertex, "morph target position/normal declarations and evaluation"},
        {"skinning_vertex", ShaderStage::Vertex, "skinning matrices and vertex deformation"},
        {"instancing_vertex", ShaderStage::Vertex, "instance matrix/color attributes and object matrix evaluation"},
        {"project_vertex", ShaderStage::Vertex, "model/view/projection and varyings"},
        {"lights_fragment", ShaderStage::Fragment, "direct light loop and attenuation"},
        {"envmap_fragment", ShaderStage::Fragment, "environment contribution for Standard/Physical"},
        {"output_fragment", ShaderStage::Fragment, "final output encoding and premultiplied alpha"}
    };
}

std::string ShaderChunk::versionHeader() {
#if THREECPP_USE_ANGLE
    return "#version 300 es\nprecision highp float;\nprecision highp int;\n";
#else
    return "#version 330 core\n";
#endif
}

std::string ShaderChunk::defines(const ProgramKey& key) {
    std::string h;
    auto def = [&](bool enabled, const char* name) {
        if (enabled) {
            h += "#define ";
            h += name;
            h += "\n";
        }
    };
    def(key.useMap, "USE_MAP");
    def(key.useAlphaMap, "USE_ALPHAMAP");
    def(key.useVertexColor, "USE_VERTEX_COLOR");
    def(key.useInstancing, "USE_INSTANCING");
    def(key.useInstanceColor, "USE_INSTANCE_COLOR");
    def(key.hasUv2, "HAS_UV2");
    def(key.useNormalMap, "USE_NORMALMAP");
    def(key.useBumpMap, "USE_BUMPMAP");
    def(key.useDisplacementMap, "USE_DISPLACEMENTMAP");
    def(key.useFlatShading, "USE_FLAT_SHADING");
    def(key.usePremultipliedAlpha, "USE_PREMULTIPLIED_ALPHA");
    def(key.useRoughnessMap, "USE_ROUGHNESSMAP");
    def(key.useMetalnessMap, "USE_METALNESSMAP");
    def(key.useAOMap, "USE_AOMAP");
    def(key.useLightMap, "USE_LIGHTMAP");
    def(key.useEmissiveMap, "USE_EMISSIVEMAP");
    def(key.useSizeAttenuation, "USE_SIZE_ATTENUATION");
    def(key.useSkinning, "USE_SKINNING");
    def(key.useMorphTargets, "USE_MORPHTARGETS");
    def(key.useMorphNormals, "USE_MORPHNORMALS");
    def(key.morphTargetsRelative, "MORPHTARGETS_RELATIVE");
    def(key.useIBL, "USE_IBL");
    def(key.useEnvMapEquirect, "USE_ENVMAP_EQUIRECT");
    def(key.usePMREM, "USE_PMREM");
    def(key.useShadowMap, "USE_SHADOWMAP");
    def(key.usePBRDirectionalShadow, "USE_PBR_DIRECTIONAL_SHADOW");
    def(key.usePBRSpotShadow, "USE_PBR_SPOT_SHADOW");
    def(key.usePBRPointShadow, "USE_PBR_POINT_SHADOW");
    def(key.useDashedLine, "USE_DASHED_LINE");
    def(key.useClipping, "USE_CLIPPING");
    def(key.usePhysical, "USE_PHYSICAL");
    def(key.useTransmission, "USE_TRANSMISSION");
    def(key.useTransmissionMap, "USE_TRANSMISSIONMAP");
    def(key.useTransmissionRenderTarget, "USE_TRANSMISSION_RENDERTARGET");
    def(key.useThicknessMap, "USE_THICKNESSMAP");
    def(key.useSpecularMap, "USE_SPECULARMAP");
    def(key.useClearcoat, "USE_CLEARCOAT");
    def(key.useClearcoatMap, "USE_CLEARCOATMAP");
    def(key.useClearcoatRoughnessMap, "USE_CLEARCOAT_ROUGHNESSMAP");
    def(key.useClearcoatNormalMap, "USE_CLEARCOAT_NORMALMAP");
    def(key.useSheen, "USE_SHEEN");
    def(key.useSheenColorMap, "USE_SHEEN_COLORMAP");
    def(key.useSheenRoughnessMap, "USE_SHEEN_ROUGHNESSMAP");
    def(key.useIridescence, "USE_IRIDESCENCE");
    def(key.useIridescenceMap, "USE_IRIDESCENCEMAP");
    def(key.useIridescenceThicknessMap, "USE_IRIDESCENCE_THICKNESSMAP");
    def(key.useAnisotropy, "USE_ANISOTROPY");
    def(key.useAnisotropyMap, "USE_ANISOTROPYMAP");
    def(key.useDispersion, "USE_DISPERSION");
    def(key.materialType == MaterialType::MeshStandard || key.materialType == MaterialType::MeshPhysical, "USE_PBR");
    def(key.materialType == MaterialType::MeshPhong, "USE_PHONG");
    def(key.materialType == MaterialType::MeshLambert, "USE_LAMBERT");
    h += "#define MAX_LIGHTS 16\n";
    return h;
}

std::string ShaderChunk::begin(std::string_view name) {
    return "\n// <chunk:" + std::string(name) + ">\n";
}

std::string ShaderChunk::end(std::string_view name) {
    return "\n// </chunk:" + std::string(name) + ">\n";
}

std::string ShaderChunk::common() {
    return R"GLSL(
const float PI = 3.141592653589793;
float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec3 saturate3(vec3 x) { return clamp(x, vec3(0.0), vec3(1.0)); }
float scalarChannel(vec4 v, int ch) {
    if (ch == 1) return v.g;
    if (ch == 2) return v.b;
    if (ch == 3) return v.a;
    return v.r;
}
)GLSL";
}

std::string ShaderChunk::colorSpace() {
    return R"GLSL(
vec3 srgbToLinear(vec3 c) { return pow(max(c, vec3(0.0)), vec3(2.2)); }
vec3 linearToOutput(vec3 c) { return pow(max(c, vec3(0.0)), vec3(1.0 / 2.2)); }
vec3 applyOutputColorSpace(vec3 c) {
    return outputColorSpace == 1 ? linearToOutput(c) : c;
}
)GLSL";
}

std::string ShaderChunk::toneMapping() {
    return R"GLSL(
vec3 aces(vec3 x) {
    x *= toneMappingExposure;
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}
vec3 applyToneMapping(vec3 c) {
    if (toneMappingMode == 0) return c;
    if (toneMappingMode == 1) return c * toneMappingExposure;
    if (toneMappingMode == 2) {
        c *= toneMappingExposure;
        return c / (vec3(1.0) + c);
    }
    if (toneMappingMode == 3) return aces(c);
    return aces(c);
}
vec4 finalOutput(vec3 linearColor, float alpha) {
    vec3 c = applyOutputColorSpace(applyToneMapping(max(linearColor, vec3(0.0))));
    if (premultipliedAlpha != 0) c *= alpha;
    return vec4(c, alpha);
}
)GLSL";
}

std::string ShaderChunk::packing() {
    return R"GLSL(
float pointShadowDepthToLinear(float depth, float nearPlane, float farPlane) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * nearPlane * farPlane) / max(farPlane + nearPlane - z * (farPlane - nearPlane), 0.00001);
}
)GLSL";
}

std::string ShaderChunk::pbrMath() {
    return R"GLSL(
float D_GGX(float NoH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float d = (NoH * a2 - NoH) * NoH + 1.0;
    return a2 / max(PI * d * d, 1e-7);
}
float V_SmithGGXCorrelated(float NoV, float NoL, float roughness) {
    float a = roughness * roughness;
    float gv = NoL * sqrt(max(NoV * (NoV - NoV * a) + a, 0.0));
    float gl = NoV * sqrt(max(NoL * (NoL - NoL * a) + a, 0.0));
    return 0.5 / max(gv + gl, 1e-7);
}
vec3 F_Schlick(vec3 f0, float VoH) {
    float f = pow(1.0 - VoH, 5.0);
    return f + f0 * (1.0 - f);
}
vec3 F_Schlick_Roughness(vec3 f0, float NoV, float roughness) {
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(1.0 - NoV, 5.0);
}
vec3 BRDFWithF0(vec3 baseColor, float metalness, float roughness, vec3 f0, vec3 N, vec3 V, vec3 L, vec3 radiance) {
    vec3 H = normalize(V + L);
    float NoV = saturate(dot(N, V)) + 1e-5;
    float NoL = saturate(dot(N, L));
    float NoH = saturate(dot(N, H));
    float VoH = saturate(dot(V, H));
    vec3 F = F_Schlick(f0, VoH);
    float r = clamp(roughness, 0.045, 1.0);
    vec3 specular = F * D_GGX(NoH, r) * V_SmithGGXCorrelated(NoV, NoL, r);
    vec3 diffuse = (1.0 - F) * (1.0 - metalness) * baseColor / PI;
    return (diffuse + specular) * radiance * NoL;
}
vec3 BRDF(vec3 baseColor, float metalness, float roughness, vec3 N, vec3 V, vec3 L, vec3 radiance) {
    return BRDFWithF0(baseColor, metalness, roughness, mix(vec3(0.04), baseColor, metalness), N, V, L, radiance);
}
)GLSL";
}

std::string ShaderChunk::physical() {
    return R"GLSL(
vec3 BRDF_Clearcoat(float clearcoat, float clearcoatRoughness, vec3 N, vec3 V, vec3 L, vec3 radiance) {
    if (clearcoat <= 0.0) return vec3(0.0);
    vec3 H = normalize(V + L);
    float NoL = saturate(dot(N, L));
    float NoV = saturate(dot(N, V)) + 1e-5;
    float NoH = saturate(dot(N, H));
    float VoH = saturate(dot(V, H));
    float r = clamp(clearcoatRoughness, 0.045, 1.0);
    float F = F_Schlick(vec3(0.04), VoH).x;
    float spec = F * D_GGX(NoH, r) * V_SmithGGXCorrelated(NoV, NoL, r);
    return radiance * spec * NoL * clearcoat;
}
vec3 BRDF_Sheen(float sheen, vec3 sheenColor, float sheenRoughness, vec3 N, vec3 V, vec3 L, vec3 radiance) {
    if (sheen <= 0.0) return vec3(0.0);
    vec3 H = normalize(V + L);
    float NoL = saturate(dot(N, L));
    float VoH = saturate(dot(V, H));
    float soft = pow(1.0 - VoH, 5.0) * mix(1.0, 0.35, clamp(sheenRoughness, 0.0, 1.0));
    return sheen * sheenColor * radiance * NoL * soft;
}
vec3 beerLambert(vec3 attenuationColor, float attenuationDistance, float thickness) {
    if (attenuationDistance <= 0.0 || attenuationDistance > 1e20 || thickness <= 0.0) return vec3(1.0);
    vec3 absorption = -log(max(attenuationColor, vec3(0.0001))) / attenuationDistance;
    return exp(-absorption * thickness);
}
vec3 anisotropyDirection(vec3 N, vec3 V, float rotation) {
    vec3 up = abs(N.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 T = normalize(cross(up, N));
    vec3 B = normalize(cross(N, T));
    float c = cos(rotation);
    float s = sin(rotation);
    return normalize(T * c + B * s + V * 0.0001);
}
)GLSL";
}

std::string ShaderChunk::ibl() {
    return R"GLSL(
vec3 hemisphereIrradiance(vec3 n, vec3 sky, vec3 ground) {
    float h = n.y * 0.5 + 0.5;
    return mix(ground, sky, h);
}
vec2 PMREM_BRDF_Approx(float NoV, float roughness) {
    const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4( 1.0,  0.0425,  1.040, -0.040);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
    return vec2(-1.04, 1.04) * a004 + r.zw;
}
vec2 equirectUv(vec3 dir) {
    dir = normalize(dir);
    float u = atan(dir.z, dir.x) / (2.0 * PI) + 0.5;
    float v = asin(clamp(dir.y, -1.0, 1.0)) / PI + 0.5;
    return vec2(u, v);
}
vec3 sampleEnvEquirect(sampler2D tex, vec3 dir) {
    return srgbToLinear(texture(tex, equirectUv(dir)).rgb);
}
float pmremRoughnessToMip(float roughness, float mipLevels) {
    float r = clamp(roughness, 0.0, 1.0);
    float maxMip = max(mipLevels - 1.0, 0.0);
    float shaped = mix(r * r, sqrt(max(r, 0.0)), 0.72);
    return clamp(shaped * maxMip, 0.0, maxMip);
}
vec3 samplePMREMPrefilter(samplerCube cubeMap, vec3 dir, float roughness, float mipLevels) {
    float lod = pmremRoughnessToMip(roughness, mipLevels);
    return textureLod(cubeMap, normalize(dir), lod).rgb;
}
vec3 approximateSpecularIBL(vec3 R, float roughness, vec3 specularColor) {
    float horizon = clamp(R.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 base = mix(envGroundColor, envSkyColor, horizon);
    return mix(base * specularColor, specularColor, pow(1.0 - roughness, 1.7));
}
)GLSL";
}

std::string ShaderChunk::shadow() {
    return R"GLSL(
float samplePointShadowMap(int shadowIndex, vec3 worldPosition) {
#ifdef USE_SHADOWMAP
    if (shadowIndex < 0 || shadowIndex >= shadowMapCount) return 1.0;
    vec3 toFrag = worldPosition - shadowLightPosition[shadowIndex];
    float dist = length(toFrag);
    float farPlane = max(shadowCameraFar[shadowIndex], 0.001);
    float nearPlane = max(shadowCameraNear[shadowIndex], 0.0001);
    if (dist >= farPlane) return 1.0;
    vec3 dir = normalize(toFrag);
    float result = 0.0;
    float bias = shadowBias[shadowIndex];
    float radius = shadowRadius[shadowIndex] / max(shadowMapSize[shadowIndex].x, 1.0);
    for (int i = 0; i < 4; ++i) {
        vec3 offset = vec3((i == 0 ? 1.0 : -1.0), (i == 1 ? 1.0 : -1.0), (i == 2 ? 1.0 : -1.0)) * radius;
        float rawDepth = 1.0;
        vec3 lookupDir = normalize(dir + offset);
        if (shadowIndex == 0) rawDepth = texture(pointShadowMap0, lookupDir).r;
        else if (shadowIndex == 1) rawDepth = texture(pointShadowMap1, lookupDir).r;
        else if (shadowIndex == 2) rawDepth = texture(pointShadowMap2, lookupDir).r;
        else if (shadowIndex == 3) rawDepth = texture(pointShadowMap3, lookupDir).r;
        float closest = pointShadowDepthToLinear(rawDepth, nearPlane, farPlane);
        result += (dist - bias <= closest) ? 1.0 : 0.0;
    }
    return result * 0.25;
#else
    return 1.0;
#endif
}
float sampleShadowMap(int shadowIndex, vec3 worldPosition) {
#ifdef USE_SHADOWMAP
    if (shadowIndex < 0 || shadowIndex >= shadowMapCount) return 1.0;
    if (shadowMapIsPoint[shadowIndex] != 0) return samplePointShadowMap(shadowIndex, worldPosition);
    vec4 sc = shadowMatrix[shadowIndex] * vec4(worldPosition, 1.0);
    sc.xyz /= max(sc.w, 0.00001);
    if (sc.x <= 0.0 || sc.x >= 1.0 || sc.y <= 0.0 || sc.y >= 1.0 || sc.z <= 0.0 || sc.z >= 1.0) return 1.0;
    vec2 texel = 1.0 / max(shadowMapSize[shadowIndex], vec2(1.0));
    float result = 0.0;
    float bias = shadowBias[shadowIndex];
    float radius = max(shadowRadius[shadowIndex], 1.0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 uv = sc.xy + vec2(x, y) * texel * radius;
            float closest = 1.0;
            if (shadowIndex == 0) closest = texture(shadowMap0, uv).r;
            else if (shadowIndex == 1) closest = texture(shadowMap1, uv).r;
            else if (shadowIndex == 2) closest = texture(shadowMap2, uv).r;
            else if (shadowIndex == 3) closest = texture(shadowMap3, uv).r;
            result += (sc.z - bias <= closest) ? 1.0 : 0.0;
        }
    }
    return result / 9.0;
#else
    return 1.0;
#endif
}
)GLSL";
}

std::string ShaderChunk::normalPerturb() {
    return R"GLSL(
vec3 perturbNormal(vec3 N, vec3 V, vec2 uv, sampler2D normalMap, vec2 normalScale) {
    vec3 mapN = texture(normalMap, uv).xyz * 2.0 - 1.0;
    mapN.xy *= normalScale;
    vec3 q1 = dFdx(V);
    vec3 q2 = dFdy(V);
    vec2 st1 = dFdx(uv);
    vec2 st2 = dFdy(uv);
    vec3 T = normalize(q1 * st2.t - q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * mapN);
}
vec3 perturbNormalBump(vec3 N, vec3 V, vec2 uv, sampler2D bumpMap, float bumpScale) {
    float height = texture(bumpMap, uv).x * bumpScale;
    float dBx = dFdx(height);
    float dBy = dFdy(height);
    vec3 q1 = dFdx(V);
    vec3 q2 = dFdy(V);
    vec3 R1 = cross(q2, N);
    vec3 R2 = cross(N, q1);
    float fDet = dot(q1, R1);
    vec3 vGrad = sign(fDet) * (dBx * R1 + dBy * R2);
    return normalize(abs(fDet) * N - vGrad);
}
)GLSL";
}


std::string ShaderChunk::fog() {
    return R"GLSL(
#ifdef USE_FOG
uniform vec3 fogColor;
uniform float fogNear;
uniform float fogFar;
uniform float fogDensity;
uniform int fogType;
float applyFogFactor(vec3 worldPos, vec3 camPos) {
    float d = distance(worldPos, camPos);
    float factor = 1.0;
    if (fogType == 0) {
        factor = clamp((fogFar - d) / max(fogFar - fogNear, 0.0001), 0.0, 1.0);
    } else {
        factor = exp(-fogDensity * fogDensity * d * d);
        factor = clamp(factor, 0.0, 1.0);
    }
    return factor;
}
vec3 applyFog(vec3 color, vec3 worldPos, vec3 camPos) {
    float factor = applyFogFactor(worldPos, camPos);
    return mix(fogColor, color, factor);
}
#endif
)GLSL";
}

std::string ShaderChunk::fragmentCore() {
    return begin("common") + common() + end("common") +
           begin("colorspace") + colorSpace() + end("colorspace") +
           begin("tonemapping") + toneMapping() + end("tonemapping") +
           begin("packing") + packing() + end("packing") +
           begin("pbr_math") + pbrMath() + end("pbr_math") +
           begin("physical") + physical() + end("physical") +
           begin("ibl") + ibl() + end("ibl") +
           begin("shadow") + shadow() + end("shadow") +
           begin("fog") + ShaderChunk::fog() + end("fog") +
           begin("normal_perturb") + normalPerturb() + end("normal_perturb");
}

ShaderBuilder::ShaderBuilder(const ProgramKey& key) {
    source_ = ShaderChunk::versionHeader();
    source_ += ShaderChunk::defines(key);
}

ShaderBuilder& ShaderBuilder::addChunk(std::string_view name, const std::string& source) {
    source_ += ShaderChunk::begin(name);
    source_ += source;
    source_ += ShaderChunk::end(name);
    return *this;
}

ShaderBuilder& ShaderBuilder::addRaw(const std::string& source) {
    source_ += source;
    return *this;
}

std::string ShaderBuilder::str() const {
    return source_;
}

} // namespace THREE
