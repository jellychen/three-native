#include "shader/shader-lib.h"
#include "shader/shader-chunk.h"

namespace THREE {

static std::string header(const ProgramKey& key) {
    return ShaderChunk::versionHeader() + ShaderChunk::defines(key);
}


static std::string common_fragment() {
    return R"GLSL(
struct GpuLight {
    int type;
    int shadowIndex;
    vec4 colorIntensity;
    vec4 positionRange;
    vec4 directionCone;
    vec4 params;
};
uniform int lightCount;
uniform GpuLight lights[MAX_LIGHTS];
uniform int shadowMapCount;
uniform mat4 shadowMatrix[4];
uniform float shadowBias[4];
uniform float shadowRadius[4];
uniform vec2 shadowMapSize[4];
uniform sampler2D shadowMap0;
uniform sampler2D shadowMap1;
uniform sampler2D shadowMap2;
uniform sampler2D shadowMap3;
uniform int shadowMapIsPoint[4];
uniform float shadowCameraNear[4];
uniform float shadowCameraFar[4];
uniform vec3 shadowLightPosition[4];
uniform samplerCube pointShadowMap0;
uniform samplerCube pointShadowMap1;
uniform samplerCube pointShadowMap2;
uniform samplerCube pointShadowMap3;
#ifdef USE_PBR_DIRECTIONAL_SHADOW
uniform int pbrDirectionalShadowEnabled;
uniform sampler2D pbrDirectionalShadowMap;
uniform mat4 pbrDirectionalShadowMatrix;
uniform vec2 pbrDirectionalShadowMapSize;
uniform float pbrDirectionalShadowBias;
uniform float pbrDirectionalShadowRadius;
uniform int pbrDirectionalShadowDebug;
#endif
#ifdef USE_PBR_SPOT_SHADOW
uniform int pbrSpotShadowEnabled;
uniform sampler2D pbrSpotShadowMap;
uniform mat4 pbrSpotShadowMatrix;
uniform vec2 pbrSpotShadowMapSize;
uniform float pbrSpotShadowBias;
uniform float pbrSpotShadowRadius;
uniform float pbrSpotShadowStrength;
uniform int pbrSpotShadowDebug;
#endif
#ifdef USE_PBR_POINT_SHADOW
uniform int pbrPointShadowEnabled;
uniform samplerCube pbrPointShadowMap;
uniform vec3 pbrPointShadowPosition;
uniform float pbrPointShadowNear;
uniform float pbrPointShadowFar;
uniform float pbrPointShadowBias;
uniform float pbrPointShadowRadius;
uniform vec2 pbrPointShadowMapSize;
uniform float pbrPointShadowStrength;
uniform int pbrPointShadowDebug;
#endif
uniform vec3 ambientLightColor;
uniform vec3 hemisphereSkyColor;
uniform vec3 hemisphereGroundColor;
uniform vec3 envSkyColor;
uniform vec3 envGroundColor;
uniform vec3 envSpecularColor;
uniform mat3 environmentRotation;
uniform vec3 cameraPosition;
uniform float toneMappingExposure;
uniform int toneMappingMode;
uniform int outputColorSpace;
uniform int premultipliedAlpha;
uniform int alphaChannel;
uniform int aoChannel;
uniform float alphaTest;
uniform int materialDebugMode;
#ifdef USE_DASHED_LINE
uniform float dashScale;
uniform float dashSize;
uniform float gapSize;
#endif
vec3 threecppSafeColor(vec3 c, vec3 fallback) {
    if (any(isnan(c)) || any(isinf(c))) return fallback;
    return clamp(c, vec3(0.0), vec3(65504.0));
}
float threecppSafeAlpha(float a) {
    if (isnan(a) || isinf(a)) return 1.0;
    return clamp(a, 0.0, 1.0);
}

#ifdef USE_FOG
uniform vec3 fogColor;
uniform float fogNear;
uniform float fogFar;
uniform float fogDensity;
uniform int fogType;
vec3 applyFog(vec3 color, vec3 worldPos, vec3 camPos) {
    float d = distance(worldPos, camPos);
    float factor = 1.0;
    if (fogType == 0) {
        factor = clamp((fogFar - d) / max(fogFar - fogNear, 0.0001), 0.0, 1.0);
    } else {
        factor = exp(-fogDensity * fogDensity * d * d);
        factor = clamp(factor, 0.0, 1.0);
    }
    factor = clamp(factor, 0.0, 1.0);
    return mix(fogColor, color, factor);
}
#endif
)GLSL" + ShaderChunk::fragmentCore();
}


ShaderSource ShaderLib::build(const ProgramKey& key) {
    const bool isPoints = key.materialType == MaterialType::Points;
    const bool isFat = key.materialType == MaterialType::FatLine;


    if (key.materialType == MaterialType::Distance) {
        return {
            header(key) + R"GLSL(
layout(location=0) in vec3 position;
layout(location=4) in vec4 skinIndex;
layout(location=5) in vec4 skinWeight;
#ifdef USE_MORPHTARGETS
layout(location=7) in vec3 morphTarget0;
layout(location=8) in vec3 morphTarget1;
layout(location=9) in vec3 morphTarget2;
layout(location=10) in vec3 morphTarget3;
uniform float morphTargetInfluences[4];
#endif
#ifdef USE_INSTANCING
layout(location=11) in vec4 instanceMatrix0;
layout(location=12) in vec4 instanceMatrix1;
layout(location=13) in vec4 instanceMatrix2;
layout(location=14) in vec4 instanceMatrix3;
#endif
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform float cameraFar;
#ifdef USE_SKINNING
uniform mat4 bindMatrix;
uniform mat4 bindMatrixInverse;
uniform mat4 boneMatrices[128];
#endif
out float vBackfaceDepth01;
void main() {
    vec3 transformed = position;
#ifdef USE_MORPHTARGETS
#ifdef MORPHTARGETS_RELATIVE
    transformed += morphTarget0 * morphTargetInfluences[0];
    transformed += morphTarget1 * morphTargetInfluences[1];
    transformed += morphTarget2 * morphTargetInfluences[2];
    transformed += morphTarget3 * morphTargetInfluences[3];
#else
    transformed += (morphTarget0 - position) * morphTargetInfluences[0];
    transformed += (morphTarget1 - position) * morphTargetInfluences[1];
    transformed += (morphTarget2 - position) * morphTargetInfluences[2];
    transformed += (morphTarget3 - position) * morphTargetInfluences[3];
#endif
#endif
#ifdef USE_SKINNING
    mat4 skinMatrix = mat4(0.0);
    skinMatrix += skinWeight.x * boneMatrices[int(skinIndex.x)];
    skinMatrix += skinWeight.y * boneMatrices[int(skinIndex.y)];
    skinMatrix += skinWeight.z * boneMatrices[int(skinIndex.z)];
    skinMatrix += skinWeight.w * boneMatrices[int(skinIndex.w)];
    vec4 skinned = bindMatrixInverse * skinMatrix * bindMatrix * vec4(transformed, 1.0);
    transformed = skinned.xyz;
#endif
    mat4 objectMatrix = modelMatrix;
#ifdef USE_INSTANCING
    objectMatrix = modelMatrix * mat4(instanceMatrix0, instanceMatrix1, instanceMatrix2, instanceMatrix3);
#endif
    vec4 mv = viewMatrix * objectMatrix * vec4(transformed, 1.0);
    gl_Position = projectionMatrix * mv;
    vBackfaceDepth01 = clamp((-mv.z) / max(cameraFar, 0.0001), 0.0, 1.0);
}
)GLSL",
            header(key) + R"GLSL(
in float vBackfaceDepth01;
out vec4 outColor;
void main() {
    outColor = vec4(vBackfaceDepth01, vBackfaceDepth01, vBackfaceDepth01, 1.0);
}
)GLSL"
        };
    }
    if (key.materialType == MaterialType::Depth) {
        return {
            header(key) + R"GLSL(
layout(location=0) in vec3 position;
layout(location=4) in vec4 skinIndex;
layout(location=5) in vec4 skinWeight;
layout(location=6) in float lineDistance;
layout(location=7) in vec3 morphTarget0;
layout(location=8) in vec3 morphTarget1;
layout(location=9) in vec3 morphTarget2;
layout(location=10) in vec3 morphTarget3;
#ifdef USE_INSTANCING
layout(location=11) in vec4 instanceMatrix0;
layout(location=12) in vec4 instanceMatrix1;
layout(location=13) in vec4 instanceMatrix2;
layout(location=14) in vec4 instanceMatrix3;
#endif
uniform float morphTargetInfluences[4];
uniform mat4 modelMatrix;
uniform mat4 lightViewProjectionMatrix;
#ifdef USE_SKINNING
uniform mat4 bindMatrix;
uniform mat4 bindMatrixInverse;
uniform mat4 boneMatrices[128];
#endif
void main() {
    vec3 transformed = position;
#ifdef USE_MORPHTARGETS
#ifdef MORPHTARGETS_RELATIVE
    transformed += morphTarget0 * morphTargetInfluences[0];
    transformed += morphTarget1 * morphTargetInfluences[1];
    transformed += morphTarget2 * morphTargetInfluences[2];
    transformed += morphTarget3 * morphTargetInfluences[3];
#else
    transformed += (morphTarget0 - position) * morphTargetInfluences[0];
    transformed += (morphTarget1 - position) * morphTargetInfluences[1];
    transformed += (morphTarget2 - position) * morphTargetInfluences[2];
    transformed += (morphTarget3 - position) * morphTargetInfluences[3];
#endif
#endif
#ifdef USE_SKINNING
    mat4 skinMatrix = mat4(0.0);
    skinMatrix += skinWeight.x * boneMatrices[int(skinIndex.x)];
    skinMatrix += skinWeight.y * boneMatrices[int(skinIndex.y)];
    skinMatrix += skinWeight.z * boneMatrices[int(skinIndex.z)];
    skinMatrix += skinWeight.w * boneMatrices[int(skinIndex.w)];
    vec4 skinned = bindMatrixInverse * skinMatrix * bindMatrix * vec4(transformed, 1.0);
    transformed = skinned.xyz;
#endif
    mat4 objectMatrix = modelMatrix;
#ifdef USE_INSTANCING
    objectMatrix = modelMatrix * mat4(instanceMatrix0, instanceMatrix1, instanceMatrix2, instanceMatrix3);
#endif
    gl_Position = lightViewProjectionMatrix * objectMatrix * vec4(transformed, 1.0);
}
)GLSL",
            header(key) + R"GLSL(
out vec4 outColor;
void main() {
    outColor = vec4(vec3(gl_FragCoord.z), 1.0);
}
)GLSL"};
    }

    if (isFat) {
        return {
            header(key) + R"GLSL(
layout(location=0) in vec3 position;
layout(location=1) in vec3 instanceStart;
layout(location=2) in vec3 instanceEnd;
layout(location=6) in float lineDistance;
layout(location=7) in vec3 morphTarget0;
layout(location=8) in vec3 morphTarget1;
layout(location=9) in vec3 morphTarget2;
layout(location=10) in vec3 morphTarget3;
layout(location=11) in vec3 morphNormal0;
layout(location=12) in vec3 morphNormal1;
layout(location=13) in vec3 morphNormal2;
layout(location=14) in vec3 morphNormal3;
uniform float morphTargetInfluences[4];
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform vec2 resolution;
uniform float linewidth;
out float vLineDistance;
void main() {
    vec4 start = projectionMatrix * viewMatrix * modelMatrix * vec4(instanceStart, 1.0);
    vec4 endp  = projectionMatrix * viewMatrix * modelMatrix * vec4(instanceEnd, 1.0);
    bool useEnd = position.x > 0.0;
    vec4 clip = useEnd ? endp : start;
    vec2 startNdc = start.xy / max(0.000001, start.w);
    vec2 endNdc = endp.xy / max(0.000001, endp.w);
    vec2 dir = endNdc - startNdc;
    if (dot(dir, dir) < 1e-10) dir = vec2(1.0, 0.0);
    dir = normalize(dir);
    vec2 normal = vec2(-dir.y, dir.x);
    normal.x /= max(0.000001, resolution.x / resolution.y);
    vec2 offset = normal * position.y * linewidth / max(1.0, resolution.y);
    clip.xy += offset * clip.w * 2.0;
    gl_Position = clip;
#ifdef USE_INSTANCE_COLOR
    vLineDistance = 0.0;
#else
    vLineDistance = lineDistance;
#endif
}
)GLSL",
            header(key) + common_fragment() + R"GLSL(
uniform vec3 diffuse;
uniform float opacity;
in float vLineDistance;
out vec4 outColor;
void main() {
#ifdef USE_DASHED_LINE
    float dashTotal = max(0.0001, dashSize + gapSize);
    if (mod(vLineDistance * dashScale, dashTotal) > dashSize) discard;
#endif
    if (opacity < alphaTest) discard;
    outColor = finalOutput(diffuse, opacity);
}
)GLSL"};
    }

    std::string vertex = header(key) + R"GLSL(
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;
layout(location=3) in vec3 color;
#ifdef HAS_UV2
layout(location=15) in vec2 uv2;
#endif
layout(location=4) in vec4 skinIndex;
layout(location=5) in vec4 skinWeight;
#ifndef USE_INSTANCE_COLOR
layout(location=6) in float lineDistance;
#endif
#ifdef USE_INSTANCING
layout(location=11) in vec4 instanceMatrix0;
layout(location=12) in vec4 instanceMatrix1;
layout(location=13) in vec4 instanceMatrix2;
layout(location=14) in vec4 instanceMatrix3;
#ifdef USE_INSTANCE_COLOR
layout(location=6) in vec4 instanceColor;
#endif
#endif
#ifdef USE_MORPHTARGETS
layout(location=7) in vec3 morphTarget0;
layout(location=8) in vec3 morphTarget1;
layout(location=9) in vec3 morphTarget2;
layout(location=10) in vec3 morphTarget3;
uniform float morphTargetInfluences[4];
#endif
#ifdef USE_MORPHNORMALS
layout(location=11) in vec3 morphNormal0;
layout(location=12) in vec3 morphNormal1;
layout(location=13) in vec3 morphNormal2;
layout(location=14) in vec3 morphNormal3;
#ifndef USE_MORPHTARGETS
uniform float morphTargetInfluences[4];
#endif
#endif
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat3 normalMatrix;
uniform mat3 uvTransform;
uniform float pointSize;
uniform float pointScale;
#ifdef USE_DISPLACEMENTMAP
uniform sampler2D displacementMap;
uniform float displacementScale;
uniform float displacementBias;
#endif
#ifdef USE_SKINNING
uniform mat4 bindMatrix;
uniform mat4 bindMatrixInverse;
uniform mat4 boneMatrices[128];
#endif
out vec2 vUv;
out vec2 vUv2;
out vec3 vColor;
out vec3 vNormal;
out vec3 vWorldPosition;
out vec3 vViewPosition;
out float vLineDistance;
void main() {
    vec3 transformed = position;
    vec3 transformedNormal = normal;
#ifdef USE_DISPLACEMENTMAP
    transformed += transformedNormal * (texture(displacementMap, (uvTransform * vec3(uv, 1.0)).xy).x * displacementScale + displacementBias);
#endif
#ifdef USE_MORPHTARGETS
#ifdef MORPHTARGETS_RELATIVE
    transformed += morphTarget0 * morphTargetInfluences[0];
    transformed += morphTarget1 * morphTargetInfluences[1];
    transformed += morphTarget2 * morphTargetInfluences[2];
    transformed += morphTarget3 * morphTargetInfluences[3];
#else
    transformed += (morphTarget0 - position) * morphTargetInfluences[0];
    transformed += (morphTarget1 - position) * morphTargetInfluences[1];
    transformed += (morphTarget2 - position) * morphTargetInfluences[2];
    transformed += (morphTarget3 - position) * morphTargetInfluences[3];
#endif
#endif
#ifdef USE_MORPHNORMALS
#ifdef MORPHTARGETS_RELATIVE
    transformedNormal += morphNormal0 * morphTargetInfluences[0];
    transformedNormal += morphNormal1 * morphTargetInfluences[1];
    transformedNormal += morphNormal2 * morphTargetInfluences[2];
    transformedNormal += morphNormal3 * morphTargetInfluences[3];
#else
    transformedNormal += (morphNormal0 - normal) * morphTargetInfluences[0];
    transformedNormal += (morphNormal1 - normal) * morphTargetInfluences[1];
    transformedNormal += (morphNormal2 - normal) * morphTargetInfluences[2];
    transformedNormal += (morphNormal3 - normal) * morphTargetInfluences[3];
#endif
#endif
#ifdef USE_SKINNING
    mat4 skinMatrix = mat4(0.0);
    skinMatrix += skinWeight.x * boneMatrices[int(skinIndex.x)];
    skinMatrix += skinWeight.y * boneMatrices[int(skinIndex.y)];
    skinMatrix += skinWeight.z * boneMatrices[int(skinIndex.z)];
    skinMatrix += skinWeight.w * boneMatrices[int(skinIndex.w)];
    vec4 skinned = bindMatrixInverse * skinMatrix * bindMatrix * vec4(transformed, 1.0);
    transformed = skinned.xyz;
#endif
    mat4 objectMatrix = modelMatrix;
#ifdef USE_INSTANCING
    objectMatrix = modelMatrix * mat4(instanceMatrix0, instanceMatrix1, instanceMatrix2, instanceMatrix3);
#endif
    vec4 world = objectMatrix * vec4(transformed, 1.0);
    vec4 mv = viewMatrix * world;
    gl_Position = projectionMatrix * mv;
#ifdef USE_VERTEX_COLOR
    vColor = color;
#else
    vColor = vec3(1.0);
#endif
#ifdef USE_INSTANCE_COLOR
    vColor *= instanceColor.rgb;
#endif
    vUv = (uvTransform * vec3(uv, 1.0)).xy;
#ifdef HAS_UV2
    vUv2 = uv2;
#else
    vUv2 = vUv;
#endif
#ifdef USE_INSTANCING
    vNormal = normalize(transpose(inverse(mat3(viewMatrix * objectMatrix))) * transformedNormal);
#else
    vNormal = normalize(normalMatrix * transformedNormal);
#endif
    vWorldPosition = world.xyz;
    vViewPosition = -mv.xyz;
#ifdef USE_INSTANCE_COLOR
    vLineDistance = 0.0;
#else
    vLineDistance = lineDistance;
#endif
)GLSL";
    if (isPoints) {
        vertex += R"GLSL(
#ifdef USE_SIZE_ATTENUATION
    gl_PointSize = pointSize * (pointScale / max(0.001, -mv.z));
#else
    gl_PointSize = pointSize;
#endif
)GLSL";
    }
    vertex += "}\n";

    std::string fragment = header(key);
    if (isPoints) {
        fragment += common_fragment() + R"GLSL(
uniform vec3 diffuse;
uniform float opacity;
#ifdef USE_MAP
uniform sampler2D map;
#endif
#ifdef USE_ALPHAMAP
uniform sampler2D alphaMap;
#endif
in vec3 vColor;
out vec4 outColor;
void main() {
    vec2 c = gl_PointCoord - vec2(0.5);
    if (dot(c, c) > 0.25) discard;
    vec4 texel = vec4(1.0);
#ifdef USE_MAP
    texel *= texture(map, gl_PointCoord);
    texel.rgb = srgbToLinear(texel.rgb);
#endif
#ifdef USE_ALPHAMAP
    texel.a *= texture(alphaMap, gl_PointCoord).r;
#endif
    float a = opacity * texel.a;
    if (materialDebugMode == 1) { outColor = vec4(max(diffuse * vColor, vec3(0.15)), max(a, 0.35)); return; }
#ifdef USE_DASHED_LINE
    float dashTotal = max(0.0001, dashSize + gapSize);
    if (mod(vLineDistance * dashScale, dashTotal) > dashSize) discard;
#endif
    if (a < alphaTest) discard;
    vec3 visibleColor = max(diffuse * vColor * texel.rgb, vec3(0.12) * diffuse);
    vec3 foggedColor = threecppSafeColor(visibleColor, diffuse);
#ifdef USE_FOG
    foggedColor = applyFog(foggedColor, vWorldPosition, cameraPosition);
#endif
    outColor = finalOutput(foggedColor, max(threecppSafeAlpha(a), 0.35));
}
)GLSL";
    } else if (key.materialType == MaterialType::MeshLambert || key.materialType == MaterialType::MeshPhong) {
        fragment += common_fragment() + R"GLSL(
uniform vec3 diffuse;
uniform vec3 emissive;
uniform float opacity;
uniform vec3 specularColor;
uniform float specularIntensity;
uniform float roughness;
uniform float lightMapIntensity;
uniform vec2 normalScale;
#ifdef USE_MAP
uniform sampler2D map;
#endif
#ifdef USE_ALPHAMAP
uniform sampler2D alphaMap;
#endif
#ifdef USE_NORMALMAP
uniform sampler2D normalMap;
#endif
#ifdef USE_AOMAP
uniform sampler2D aoMap;
#endif
#ifdef USE_LIGHTMAP
uniform sampler2D lightMap;
#endif
#ifdef USE_EMISSIVEMAP
uniform sampler2D emissiveMap;
#endif
#ifdef USE_SPECULARMAP
uniform sampler2D specularMap;
#endif
in vec2 vUv;
in vec2 vUv2;
in vec3 vColor;
in vec3 vNormal;
in vec3 vWorldPosition;
in vec3 vViewPosition;
out vec4 outColor;
void main() {
    vec4 texel = vec4(1.0);
#ifdef USE_MAP
    texel *= texture(map, vUv);
    texel.rgb = srgbToLinear(texel.rgb);
#endif
    float alpha = opacity * texel.a;
#ifdef USE_ALPHAMAP
    alpha *= scalarChannel(texture(alphaMap, vUv), alphaChannel);
#endif
    if (alpha < alphaTest) discard;
#ifdef USE_CLIPPING
    checkClipping(vWorldPosition);
#endif
    vec3 baseColor = diffuse * vColor * texel.rgb;
    if (materialDebugMode == 1) { outColor = vec4(max(baseColor, vec3(0.15)), max(alpha, 0.35)); return; }
    if (materialDebugMode == 2) { vec3 nn = normalize(vNormal); outColor = vec4(nn * 0.5 + 0.5, 1.0); return; }
    vec3 N = normalize(vNormal);
    vec3 V = normalize(cameraPosition - vWorldPosition);
#ifdef USE_FLAT_SHADING
    N = normalize(cross(dFdx(vWorldPosition), dFdy(vWorldPosition)));
    if (dot(N, V) < 0.0) N = -N;
#endif
#ifdef USE_NORMALMAP
    N = perturbNormal(N, normalize(vViewPosition), vUv, normalMap, normalScale);
#endif
#if defined(USE_BUMPMAP) && !defined(USE_NORMALMAP)
    N = perturbNormalBump(N, normalize(vViewPosition), vUv, bumpMap, bumpScale);
#endif
    vec3 total = baseColor * (ambientLightColor + hemisphereIrradiance(N, hemisphereSkyColor + envSkyColor, hemisphereGroundColor + envGroundColor)) / PI;
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (i >= lightCount) break;
        GpuLight l = lights[i];
        vec3 L = vec3(0.0);
        vec3 radiance = l.colorIntensity.rgb * l.colorIntensity.a;
        if (l.type == 1) {
            L = normalize(-l.directionCone.xyz);
        } else if (l.type == 2 || l.type == 3 || l.type == 4) {
            vec3 toLight = l.positionRange.xyz - vWorldPosition;
            float dist2 = max(dot(toLight, toLight), 0.0001);
            float dist = sqrt(dist2);
            L = toLight / dist;
            float range = l.positionRange.w;
            float decay = l.params.x;
            float attenuation = decay <= 0.0001 ? 1.0 : 1.0 / max(pow(dist, decay), 0.01);
            if (range > 0.0) {
                float cutoff = max(1.0 - pow(dist / range, 4.0), 0.0);
                attenuation *= cutoff * cutoff;
            }
            if (l.type == 3) {
                float spotCos = dot(normalize(l.directionCone.xyz), -L);
                float outerCone = l.directionCone.w;
                float innerCone = l.params.y;
                float coneAttenuation = (innerCone - outerCone) < 0.0001 ? step(outerCone, spotCos) : smoothstep(outerCone, innerCone, spotCos);
                attenuation *= coneAttenuation;
            }
            if (l.type == 4) {
                float area = max(l.params.x * l.params.y, 0.0001);
                float facing = saturate(dot(normalize(l.directionCone.xyz), -L));
                attenuation = facing * area / max(dist2, 0.01);
            }
            radiance *= attenuation;
        }
        float shadow = sampleShadowMap(l.shadowIndex, vWorldPosition);
        radiance *= shadow;
        float NoL = saturate(dot(N, L));
        vec3 lambert = baseColor * radiance * NoL / PI;
#ifdef USE_SPECULARMAP
        vec3 localSpecular = specularColor * srgbToLinear(texture(specularMap, vUv).rgb);
#else
        vec3 localSpecular = specularColor;
#endif
        total += lambert;
#ifdef USE_PHONG
        vec3 H = normalize(V + L);
        float specPower = max(2.0, 2.0 / max(roughness * roughness, 0.0001));
        vec3 phong = localSpecular * specularIntensity * pow(saturate(dot(N, H)), specPower) * radiance * NoL;
        total += phong;
#endif
    }
#ifdef USE_AOMAP
    total *= scalarChannel(texture(aoMap, vUv2), aoChannel);
#endif
#ifdef USE_LIGHTMAP
    total += baseColor * srgbToLinear(texture(lightMap, vUv2).rgb) * lightMapIntensity;
#endif
    vec3 e = emissive;
#ifdef USE_EMISSIVEMAP
    e *= srgbToLinear(texture(emissiveMap, vUv).rgb);
#endif
    total += e;
    total = threecppSafeColor(total, baseColor);
    total = max(total, baseColor * 0.12);
    alpha = max(threecppSafeAlpha(alpha), 0.35);
    #ifdef USE_FOG
    total = applyFog(total, vWorldPosition, cameraPosition);
#endif
    outColor = finalOutput(total, alpha);
}
)GLSL";
    } else if (key.materialType == MaterialType::MeshStandard || key.materialType == MaterialType::MeshPhysical) {
        // v6.0.23: robust Standard/Physical forward path.
        // The previous experimental PBR branch was too large and could produce an
        // invisible frame on macOS Core GL while the draw calls/VAOs were valid.
        // This path keeps Standard material semantics visible first: baseColor,
        // roughness/metalness channels, vertex color, alpha, emissive, AO/lightMap,
        // direct lights and lightweight environment. Physical-only lobes remain
        // approximated until the full chunk path is reintroduced incrementally.
        fragment += common_fragment() + R"GLSL(
uniform vec3 diffuse;
uniform vec3 emissive;
uniform float roughness;
uniform float metalness;
uniform float opacity;
uniform float envMapIntensity;
uniform int pbrIblDebugMode;
uniform float pbrPmremSpecularStrength;
uniform float pmremMipLevels;
uniform float aoMapIntensity;
uniform vec2 normalScale;
uniform float lightMapIntensity;
uniform int roughnessChannel;
uniform int metalnessChannel;
#ifdef USE_PHYSICAL
uniform float ior;
uniform float dispersion;
uniform float transmission;
uniform float thickness;
uniform float attenuationDistance;
uniform vec3 attenuationColor;
uniform float specularIntensity;
uniform vec3 specularColor;
uniform float clearcoat;
uniform float clearcoatRoughness;
uniform float sheen;
uniform vec3 sheenColor;
uniform float sheenRoughness;
#ifdef USE_TRANSMISSIONMAP
uniform sampler2D transmissionMap;
#endif
#ifdef USE_THICKNESSMAP
uniform sampler2D thicknessMap;
#endif
// three.js MeshPhysicalMaterial keeps transmission scalar/volume inputs available
// even when the renderer-level transmission render target is not active.  This
// shader also uses transmissionResolution for gl_FragCoord normalization and
// transmissionDebugMode for non-render-target debug branches, so they must not
// be hidden behind USE_TRANSMISSION_RENDERTARGET.
uniform vec2 transmissionResolution;
uniform int transmissionDebugMode;
#ifdef USE_TRANSMISSION_RENDERTARGET
uniform sampler2D transmissionSamplerMap;
uniform sampler2D transmissionBackfaceMap;
uniform float transmissionSamplerMapLevel;
uniform float transmissionCameraFar;
uniform int transmissionUseBackfaceMap;
#endif
#endif
#ifdef USE_MAP
uniform sampler2D map;
#endif
#ifdef USE_ALPHAMAP
uniform sampler2D alphaMap;
#endif
#ifdef USE_NORMALMAP
uniform sampler2D normalMap;
#endif
#ifdef USE_BUMPMAP
uniform sampler2D bumpMap;
uniform float bumpScale;
#endif
#ifdef USE_ROUGHNESSMAP
uniform sampler2D roughnessMap;
#endif
#ifdef USE_METALNESSMAP
uniform sampler2D metalnessMap;
#endif
#ifdef USE_AOMAP
uniform sampler2D aoMap;
#endif
#ifdef USE_LIGHTMAP
uniform sampler2D lightMap;
#endif
#ifdef USE_EMISSIVEMAP
uniform sampler2D emissiveMap;
#endif
#ifdef USE_ENVMAP_EQUIRECT
uniform sampler2D envMapEquirect;
#endif
#ifdef USE_PMREM
uniform samplerCube irradianceMap;
uniform samplerCube prefilteredEnvMap;
uniform sampler2D brdfLUT;
#endif
in vec2 vUv;
in vec2 vUv2;
in vec3 vColor;
in vec3 vNormal;
in vec3 vWorldPosition;
in vec3 vViewPosition;
out vec4 outColor;

vec3 safeNormalize(vec3 v, vec3 fallback) {
    float l2 = dot(v, v);
    if (l2 <= 1e-12 || isnan(l2) || isinf(l2)) return fallback;
    return v * inversesqrt(l2);
}

#ifdef USE_PBR_DIRECTIONAL_SHADOW
float samplePBRDirectionalShadow(vec3 worldPosition) {
    if (pbrDirectionalShadowEnabled == 0) return 1.0;
    vec4 sc = pbrDirectionalShadowMatrix * vec4(worldPosition, 1.0);
    if (abs(sc.w) < 1e-6) return 1.0;
    vec3 proj = sc.xyz / sc.w;
    if (proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0 || proj.z < 0.0 || proj.z > 1.0) return 1.0;
    vec2 texel = 1.0 / max(pbrDirectionalShadowMapSize, vec2(1.0));
    float radius = clamp(pbrDirectionalShadowRadius, 1.0, 4.0);
    float bias = clamp(pbrDirectionalShadowBias, 0.0, 0.02);
    float sum = 0.0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            vec2 uv = proj.xy + vec2(float(x), float(y)) * texel * radius;
            float closest = texture(pbrDirectionalShadowMap, uv).r;
            sum += (proj.z - bias) <= closest ? 1.0 : 0.0;
        }
    }
    float v = sum / 9.0;
    if (isnan(v) || isinf(v)) return 1.0;
    return clamp(v, 0.0, 1.0);
}
#endif

#ifdef USE_PBR_SPOT_SHADOW
float samplePBRSpotShadow(vec3 worldPosition) {
    if (pbrSpotShadowEnabled == 0) return 1.0;
    vec4 sc = pbrSpotShadowMatrix * vec4(worldPosition, 1.0);
    if (abs(sc.w) < 1e-6) return 1.0;
    vec3 proj = sc.xyz / sc.w;
    if (proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0 || proj.z < 0.0 || proj.z > 1.0) return 1.0;
    vec2 texel = 1.0 / max(pbrSpotShadowMapSize, vec2(1.0));
    float radius = clamp(pbrSpotShadowRadius, 1.0, 4.0);
    float bias = clamp(pbrSpotShadowBias, 0.0, 0.02);
    float sum = 0.0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            vec2 uv = proj.xy + vec2(float(x), float(y)) * texel * radius;
            float closest = texture(pbrSpotShadowMap, uv).r;
            sum += (proj.z - bias) <= closest ? 1.0 : 0.0;
        }
    }
    float v = sum / 9.0;
    if (isnan(v) || isinf(v)) return 1.0;
    return clamp(v, 0.0, 1.0);
}
#endif

#ifdef USE_PBR_POINT_SHADOW
// OpenGL depth cubemap stores the hardware perspective depth of each 90-degree
// cube-face camera, not a linear radial distance. For a point shadow receiver
// we must compare against the face-space depth of the dominant cube axis:
// max(abs(lightToFragment.xyz)). Comparing length(lightToFragment) / far to the
// hardware depth produces thin, unstable lines and most shadows disappear.
float pbrPointFaceDepth(vec3 lightToFragment) {
    float d = max(max(abs(lightToFragment.x), abs(lightToFragment.y)), abs(lightToFragment.z));
    return d;
}

float pbrPerspectiveDepth01(float viewDistance, float nearPlane, float farPlane) {
    float d = clamp(viewDistance, nearPlane + 1e-5, farPlane - 1e-5);
    // Equivalent to gl_FragCoord.z for an OpenGL perspective projection with
    // positive camera-space distance d.
    return clamp(farPlane / (farPlane - nearPlane) - (farPlane * nearPlane) / ((farPlane - nearPlane) * d), 0.0, 1.0);
}

float pbrPointShadowCompare(vec3 sampleDir, float receiverDepth) {
    float closest = texture(pbrPointShadowMap, safeNormalize(sampleDir, vec3(1.0, 0.0, 0.0))).r;
    if (isnan(closest) || isinf(closest)) return 1.0;
    return receiverDepth <= closest ? 1.0 : 0.0;
}

float samplePBRPointShadow(vec3 worldPosition) {
    if (pbrPointShadowEnabled == 0) return 1.0;
    vec3 fromLight = worldPosition - pbrPointShadowPosition;
    float radialDist = length(fromLight);
    float nearPlane = max(pbrPointShadowNear, 0.001);
    float farPlane = max(pbrPointShadowFar, nearPlane + 0.001);
    if (radialDist <= nearPlane || radialDist >= farPlane) return 1.0;

    vec3 dir = safeNormalize(fromLight, vec3(1.0, 0.0, 0.0));

    // Bias is specified in world units. Apply it before projection into the
    // nonlinear hardware depth domain. This makes the bias scale consistently
    // with camera near/far and removes the previous depth-space mismatch.
    float faceDepth = pbrPointFaceDepth(fromLight);
    float compareDepth = pbrPerspectiveDepth01(max(faceDepth - max(pbrPointShadowBias, 0.0), nearPlane), nearPlane, farPlane);

    float mapSize = max(max(pbrPointShadowMapSize.x, pbrPointShadowMapSize.y), 1.0);
    float texel = 1.0 / mapSize;
    float radius = clamp(pbrPointShadowRadius, 0.0, 8.0) * texel;

    vec3 up = abs(dir.y) < 0.92 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = safeNormalize(cross(up, dir), vec3(1.0, 0.0, 0.0));
    vec3 bitangent = safeNormalize(cross(dir, tangent), vec3(0.0, 1.0, 0.0));

    float sum = 0.0;
    sum += pbrPointShadowCompare(dir, compareDepth);
    sum += pbrPointShadowCompare(dir + radius * tangent, compareDepth);
    sum += pbrPointShadowCompare(dir - radius * tangent, compareDepth);
    sum += pbrPointShadowCompare(dir + radius * bitangent, compareDepth);
    sum += pbrPointShadowCompare(dir - radius * bitangent, compareDepth);
    sum += pbrPointShadowCompare(dir + radius * (tangent + bitangent), compareDepth);
    sum += pbrPointShadowCompare(dir + radius * (tangent - bitangent), compareDepth);
    sum += pbrPointShadowCompare(dir + radius * (-tangent + bitangent), compareDepth);
    sum += pbrPointShadowCompare(dir - radius * (tangent + bitangent), compareDepth);

    float v = sum / 9.0;
    if (isnan(v) || isinf(v)) return 1.0;
    return clamp(v, 0.0, 1.0);
}
#endif

void main() {
    vec4 texel = vec4(1.0);
#ifdef USE_MAP
    texel = texture(map, vUv);
    texel.rgb = srgbToLinear(texel.rgb);
#endif
    float alpha = opacity * texel.a;
#ifdef USE_ALPHAMAP
    alpha *= scalarChannel(texture(alphaMap, vUv), alphaChannel);
#endif
    if (alpha < alphaTest) discard;

    vec3 baseColor = max(diffuse * vColor * texel.rgb, vec3(0.0));
    float r = clamp(roughness, 0.045, 1.0);
    float m = clamp(metalness, 0.0, 1.0);
#ifdef USE_ROUGHNESSMAP
    r = clamp(r * scalarChannel(texture(roughnessMap, vUv), roughnessChannel), 0.045, 1.0);
#endif
#ifdef USE_METALNESSMAP
    m = clamp(m * scalarChannel(texture(metalnessMap, vUv), metalnessChannel), 0.0, 1.0);
#endif

    if (materialDebugMode == 1) { outColor = vec4(max(baseColor, vec3(0.12)), max(alpha, 0.35)); return; }
    if (materialDebugMode == 2) { vec3 nn = safeNormalize(vNormal, vec3(0.0, 0.0, 1.0)); outColor = vec4(nn * 0.5 + 0.5, 1.0); return; }

    vec3 N = safeNormalize(vNormal, vec3(0.0, 0.0, 1.0));
    vec3 V = safeNormalize(cameraPosition - vWorldPosition, vec3(0.0, 0.0, 1.0));
#ifdef USE_FLAT_SHADING
    vec3 flatN = cross(dFdx(vWorldPosition), dFdy(vWorldPosition));
    N = safeNormalize(flatN, N);
    if (dot(N, V) < 0.0) N = -N;
#endif
#ifdef USE_NORMALMAP
    // v6.0.25: restore normal-map support in the safe PBR path.
    // perturbNormal uses screen-space derivatives, so it works even when
    // imported glTF/FBX meshes do not provide explicit tangents.
    N = safeNormalize(perturbNormal(N, V, vUv, normalMap, normalScale), N);
#endif
#if defined(USE_BUMPMAP) && !defined(USE_NORMALMAP)
    N = safeNormalize(perturbNormalBump(N, V, vUv, bumpMap, bumpScale), N);
#endif

    // Always keep a non-zero ambient floor. This is intentional for the v6.0
    // regression phase so valid draw calls never produce a fully invisible mesh.
    vec3 hemi = hemisphereIrradiance(N, hemisphereSkyColor + envSkyColor * envMapIntensity,
                                        hemisphereGroundColor + envGroundColor * envMapIntensity);
    vec3 total = baseColor * max(ambientLightColor + hemi, vec3(0.10)) * (1.0 - 0.35 * m);

    vec3 f0 = mix(vec3(0.04), baseColor, m);
#ifdef USE_PHYSICAL
    float eta = max(1.0, ior);
    float dielectricF0 = pow((eta - 1.0) / (eta + 1.0), 2.0);
    f0 = mix(vec3(dielectricF0) * specularColor * specularIntensity, baseColor, m);
#endif

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (i >= lightCount) break;
        GpuLight l = lights[i];
        vec3 L = vec3(0.0, 1.0, 0.0);
        vec3 radiance = l.colorIntensity.rgb * l.colorIntensity.a;
        if (l.type == 1) {
            L = safeNormalize(-l.directionCone.xyz, vec3(0.0, 1.0, 0.0));
        } else if (l.type == 2 || l.type == 3 || l.type == 4) {
            vec3 toLight = l.positionRange.xyz - vWorldPosition;
            float dist2 = max(dot(toLight, toLight), 0.0001);
            float dist = sqrt(dist2);
            L = toLight / dist;
            float range = l.positionRange.w;
            float decay = l.params.x;
            float attenuation = decay <= 0.0001 ? 1.0 : 1.0 / max(pow(dist, decay), 0.01);
            if (range > 0.0) {
                float cutoff = max(1.0 - pow(dist / range, 4.0), 0.0);
                attenuation *= cutoff * cutoff;
            }
            if (l.type == 3) {
                float spotCos = dot(safeNormalize(l.directionCone.xyz, vec3(0.0, -1.0, 0.0)), -L);
                float outerCone = l.directionCone.w;
                float innerCone = l.params.y;
                float coneAttenuation = (innerCone - outerCone) < 0.0001 ? step(outerCone, spotCos) : smoothstep(outerCone, innerCone, spotCos);
                attenuation *= coneAttenuation;
            }
            if (l.type == 4) {
                float area = max(l.params.x * l.params.y, 0.0001);
                float facing = saturate(dot(safeNormalize(l.directionCone.xyz, vec3(0.0, -1.0, 0.0)), -L));
                attenuation = facing * area / max(dist2, 0.01);
            }
            radiance *= attenuation;
        }
#ifdef USE_PBR_DIRECTIONAL_SHADOW
        // v6.0.29: only restore the first directional shadow in the experimental
        // PBR path. Do not touch spot/point/cube shadow branches here. This keeps
        // PBR visible while reintroducing shadows one safe step at a time.
        if (l.type == 1) {
            float pbrShadow = samplePBRDirectionalShadow(vWorldPosition);
            if (pbrDirectionalShadowDebug != 0) { outColor = vec4(vec3(0.12 + 0.88 * pbrShadow), 1.0); return; }
            // Keep a small amount of the light contribution visible even if the
            // shadow factor is 0. This makes standalone shadow labs debuggable
            // when the shadow camera/bias is still being tuned.
            radiance *= mix(1.0, max(pbrShadow, 0.22), 0.82);
        }
#endif
#ifdef USE_PBR_SPOT_SHADOW
        // v6.0.30: restore only the first SpotLight shadow through an isolated
        // sampler. Keep the legacy shadow-array and point-cube paths disabled.
        if (l.type == 3) {
            float pbrSpot = samplePBRSpotShadow(vWorldPosition);
            if (pbrSpotShadowDebug != 0) { outColor = vec4(vec3(0.12 + 0.88 * pbrSpot), 1.0); return; }
            float spotShadowStrength = clamp(pbrSpotShadowStrength, 0.0, 1.0);
            radiance *= mix(1.0, max(pbrSpot, 0.20), spotShadowStrength);
        }
#endif
#ifdef USE_PBR_POINT_SHADOW
        // v6.0.32: restore only the first PointLight cubemap shadow through an
        // isolated samplerCube. Keep the legacy shadow-array/cube branch off.
        if (l.type == 2) {
            float pbrPoint = samplePBRPointShadow(vWorldPosition);
            if (pbrPointShadowDebug != 0) { outColor = vec4(vec3(pbrPoint), 1.0); return; }
            radiance *= mix(1.0, pbrPoint, clamp(pbrPointShadowStrength, 0.0, 1.0));
        }
#endif
#ifdef USE_SHADOWMAP
        float pbrShadow = 1.0;
        if (l.shadowIndex >= 0 && l.shadowIndex < shadowMapCount) {
            pbrShadow = sampleShadowMap(l.shadowIndex, vWorldPosition);
            if (isnan(pbrShadow) || isinf(pbrShadow)) pbrShadow = 1.0;
            pbrShadow = clamp(pbrShadow, 0.0, 1.0);
        }
        radiance *= mix(1.0, pbrShadow, 0.85);
#endif
        float NoL = saturate(dot(N, L));
        vec3 H = safeNormalize(V + L, N);
        float NoV = saturate(dot(N, V));
        float NoH = saturate(dot(N, H));
        float VoH = saturate(dot(V, H));
        vec3 F = F_Schlick(f0, VoH);
        float specPower = mix(96.0, 8.0, r);
        vec3 diffuseTerm = (1.0 - F) * (1.0 - m) * baseColor * NoL;
        vec3 specTerm = F * pow(max(NoH, 0.0), specPower) * NoL;
        total += (diffuseTerm + specTerm) * radiance;
#ifdef USE_PHYSICAL
        total += BRDF_Clearcoat(clamp(clearcoat, 0.0, 1.0), clamp(clearcoatRoughness, 0.045, 1.0), N, V, L, radiance);
        total += BRDF_Sheen(clamp(sheen, 0.0, 1.0), sheenColor, clamp(sheenRoughness, 0.0, 1.0), N, V, L, radiance);
#endif
    }

#ifdef USE_IBL
    vec3 R = reflect(-V, N);
    vec3 iblDiffuse = hemisphereIrradiance(N, envSkyColor, envGroundColor) * baseColor * (1.0 - m);
    vec3 iblSpec = approximateSpecularIBL(R, r, envSpecularColor) * mix(vec3(0.04), baseColor, m);
    float pmremDebugLod = 0.0;
#ifdef USE_PMREM
    vec3 pmremN = safeNormalize(environmentRotation * N, vec3(0.0, 1.0, 0.0));
    vec3 pmremR = safeNormalize(environmentRotation * R, vec3(0.0, 0.0, 1.0));
    vec3 pmremDiffuseRaw = texture(irradianceMap, pmremN).rgb;
    vec3 pmremSpecularRaw = samplePMREMPrefilter(prefilteredEnvMap, pmremR, r, pmremMipLevels);
    pmremDiffuseRaw = threecppSafeColor(pmremDiffuseRaw, envSkyColor * 0.35 + envGroundColor * 0.65);
    pmremSpecularRaw = threecppSafeColor(pmremSpecularRaw, envSpecularColor);
    vec2 brdf = texture(brdfLUT, vec2(saturate(dot(N, V)), r)).rg;
    if (isnan(brdf.x) || isinf(brdf.x)) brdf.x = 1.0;
    if (isnan(brdf.y) || isinf(brdf.y)) brdf.y = 0.0;
    iblDiffuse = pmremDiffuseRaw * baseColor * (1.0 - m);
    iblSpec = pmremSpecularRaw * (f0 * brdf.x + brdf.y) * pbrPmremSpecularStrength;
    pmremDebugLod = clamp(r * max(pmremMipLevels - 1.0, 1.0), 0.0, max(pmremMipLevels - 1.0, 1.0));
#elif defined(USE_ENVMAP_EQUIRECT)
    vec3 eqDiffuseRaw = sampleEnvEquirect(envMapEquirect, environmentRotation * N);
    vec3 eqSpecularRaw = sampleEnvEquirect(envMapEquirect, environmentRotation * R);
    eqDiffuseRaw = threecppSafeColor(eqDiffuseRaw, envSkyColor);
    eqSpecularRaw = threecppSafeColor(eqSpecularRaw, envSpecularColor);
    iblDiffuse = eqDiffuseRaw * baseColor * (1.0 - m);
    iblSpec = eqSpecularRaw * mix(vec3(0.04), baseColor, m) * (1.0 - r * 0.65);
#endif
    if (pbrIblDebugMode == 1) { outColor = vec4(clamp(iblDiffuse * envMapIntensity, 0.0, 1.0), 1.0); return; }
    if (pbrIblDebugMode == 2) { outColor = vec4(clamp(iblSpec * envMapIntensity, 0.0, 1.0), 1.0); return; }
    if (pbrIblDebugMode == 3) { outColor = vec4(vec3(pmremDebugLod / max(pmremMipLevels - 1.0, 1.0)), 1.0); return; }
    total += envMapIntensity * (iblDiffuse * 0.35 + iblSpec);
#endif

#ifdef USE_AOMAP
    float ao = mix(1.0, scalarChannel(texture(aoMap, vUv2), aoChannel), aoMapIntensity);
    total *= ao;
#endif
#ifdef USE_LIGHTMAP
    total += baseColor * srgbToLinear(texture(lightMap, vUv2).rgb) * lightMapIntensity;
#endif
    vec3 e = emissive;
#ifdef USE_EMISSIVEMAP
    e *= srgbToLinear(texture(emissiveMap, vUv).rgb);
#endif
    total += e;
#ifdef USE_PHYSICAL
    float transmissionFactor = clamp(transmission, 0.0, 1.0);
    float materialTransmissionVisibility = 0.0;
#ifdef USE_TRANSMISSIONMAP
    transmissionFactor *= scalarChannel(texture(transmissionMap, vUv), alphaChannel);
#endif
    float thicknessFactor = max(thickness, 0.0);
#ifdef USE_THICKNESSMAP
    thicknessFactor *= max(texture(thicknessMap, vUv).g, 0.0);
#endif
    if (transmissionFactor > 0.0) {
        // v6.0.47: make screen-space transmission read as thick glass rather than
        // a flat transparent decal.  We approximate volume thickness by scaling the
        // material thickness by the view angle: grazing rays travel farther through
        // the object and therefore absorb more color.
        float NoVTransmission = max(abs(dot(N, V)), 0.12);
        float viewThickness = thicknessFactor / NoVTransmission;
        vec2 screenUv = gl_FragCoord.xy / max(transmissionResolution, vec2(1.0));
        float backsideThickness = 0.0;
#ifdef USE_TRANSMISSION_RENDERTARGET
        if (transmissionUseBackfaceMap != 0) {
            float backDepth01 = texture(transmissionBackfaceMap, clamp(screenUv, vec2(0.001), vec2(0.999))).r;
            float frontDepth01 = clamp(vViewPosition.z / max(transmissionCameraFar, 0.0001), 0.0, 1.0);
            // If the backface pass did not cover this pixel, the target is clear=1.
            // Treat that as unavailable and fall back to material thickness.
            if (backDepth01 < 0.999) {
                backsideThickness = max((backDepth01 - frontDepth01) * transmissionCameraFar, 0.0);
            }
        }
#endif
        // three.js-style volume approximation: geometry thickness from a
        // backside depth pass dominates when available; material thickness and
        // thicknessMap still scale the result.  This prevents GLB transmission
        // assets from looking like flat alpha shells and gives thick areas a
        // real volume response.
        float combinedThickness = backsideThickness > 0.0001
            ? backsideThickness * max(thicknessFactor, 0.001)
            : viewThickness;
        // v6.0.56: approximate three.js transmission more closely.  The
        // backside depth difference is in view/world scale, while glTF volume
        // thickness is authored for material space.  Multiplying both directly
        // makes DragonDispersion-style assets far too dense/opaque in a
        // single-pass approximation.  Use the depth target to modulate shape,
        // but soften the optical path until a true three.js backface-thickness
        // pipeline with the same camera-space units is in place.
        float opticalThickness = thicknessFactor <= 0.0001 ? 0.0 : min(combinedThickness * 0.22, 8.0);
        vec3 transmittance = beerLambert(attenuationColor, attenuationDistance, opticalThickness);
        float fresnelGlass = pow(1.0 - saturate(dot(N, V)), 5.0);
#ifdef USE_TRANSMISSION_RENDERTARGET
        // Stronger refraction at the edges and for thicker objects gives spheres a
        // visible lensing silhouette.  Roughness reduces coherent offset but adds blur.
        float edgeBoost = 0.35 + 1.65 * fresnelGlass;
        float refractStrength = clamp((ior - 1.0) * 0.08 + opticalThickness * 0.020, 0.0, 0.18);
        vec2 offset = N.xy * refractStrength * edgeBoost * (1.0 - r * 0.35);
        vec2 refractUv = clamp(screenUv + offset, vec2(0.001), vec2(0.999));
        float maxLod = max(transmissionSamplerMapLevel, 0.0);
        float lod = clamp((r * r * 1.75 + opticalThickness * 0.035) * maxLod, 0.0, maxLod);
        vec2 texel = 1.0 / max(transmissionResolution, vec2(1.0));
        float blurRadius = (1.0 + r * 10.0 + opticalThickness * 1.5);
        // Cheap stable 5-tap blur: enough to communicate rough transmission without
        // making the test scene too expensive.  textureLod remains the primary blur
        // when mip levels exist; these taps help when the target has few mips.
        // v6.0.53: opaque transmissive glass path.  Sample the captured opaque
        // background with a stronger lens offset and optional chromatic dispersion
        // instead of relying on alpha blending.  This makes GLB assets such as
        // DragonDispersion behave much closer to three.js MeshPhysicalMaterial:
        // the dragon remains a solid refractive object, not a ghosted overlay.
        vec2 baseOffset = offset;
        float dispersionAmount = clamp(dispersion * 0.010, 0.0, 0.065) * (0.35 + fresnelGlass * 1.5 + opticalThickness * 0.06);
        vec2 dispersionDir = length(baseOffset) > 1e-5 ? normalize(baseOffset) : normalize(N.xy + vec2(0.173, -0.117));
        vec2 uvR = clamp(refractUv + dispersionDir * dispersionAmount, vec2(0.001), vec2(0.999));
        vec2 uvG = refractUv;
        vec2 uvB = clamp(refractUv - dispersionDir * dispersionAmount, vec2(0.001), vec2(0.999));

        vec3 refracted;
        refracted.r = textureLod(transmissionSamplerMap, uvR, lod).r;
        refracted.g = textureLod(transmissionSamplerMap, uvG, lod).g;
        refracted.b = textureLod(transmissionSamplerMap, uvB, lod).b;
        refracted = refracted * 0.46;
        refracted += textureLod(transmissionSamplerMap, clamp(refractUv + vec2( texel.x, 0.0) * blurRadius, vec2(0.001), vec2(0.999)), lod).rgb * 0.135;
        refracted += textureLod(transmissionSamplerMap, clamp(refractUv + vec2(-texel.x, 0.0) * blurRadius, vec2(0.001), vec2(0.999)), lod).rgb * 0.135;
        refracted += textureLod(transmissionSamplerMap, clamp(refractUv + vec2(0.0,  texel.y) * blurRadius, vec2(0.001), vec2(0.999)), lod).rgb * 0.135;
        refracted += textureLod(transmissionSamplerMap, clamp(refractUv + vec2(0.0, -texel.y) * blurRadius, vec2(0.001), vec2(0.999)), lod).rgb * 0.135;
        refracted = threecppSafeColor(refracted, envSpecularColor);
        if (transmissionDebugMode == 1) { outColor = vec4(clamp(refracted, 0.0, 1.0), 1.0); return; }
        if (transmissionDebugMode == 2) { outColor = vec4(refractUv, 0.0, 1.0); return; }
        if (transmissionDebugMode == 3) { outColor = vec4(clamp(transmittance, 0.0, 1.0), 1.0); return; }
        if (transmissionDebugMode == 4) { outColor = vec4(vec3(clamp(opticalThickness / 4.0, 0.0, 1.0)), 1.0); return; }
        if (transmissionDebugMode == 5) { outColor = vec4(vec3(clamp(fresnelGlass, 0.0, 1.0)), 1.0); return; }
        if (transmissionDebugMode == 6) { outColor = vec4(vec3(clamp(backsideThickness / 4.0, 0.0, 1.0)), 1.0); return; }
        // v6.0.55: GLB transmission parity tuning.
        // KHR_materials_volume assets such as DragonDispersion use very small
        // attenuationDistance with a high thicknessFactor. A strict single-pass
        // Beer-Lambert evaluation can therefore drive the refracted background
        // almost to black, which reads as an opaque plastic shell. three.js gets a
        // much richer result from its renderer-level transmission capture,
        // specular environment, and volume heuristics. Until a true backside
        // thickness pass is available, use a softened transmittance for the
        // screen-space background term, while keeping the raw transmittance for
        // debug and visible tinting.
        vec3 softTransmittance = pow(clamp(transmittance, vec3(0.0), vec3(1.0)), vec3(0.18));
        softTransmittance = mix(softTransmittance, vec3(1.0), 0.34);
        vec3 transmitted = refracted * softTransmittance;

        // Fresnel reflection from IBL/environment. Use IOR-derived F0 so glass has
        // a visible center reflection and strong silhouette reflection instead of
        // only appearing as alpha-blended transparency.
        vec3 Rg = reflect(-V, N);
        vec3 edgeReflection = envSpecularColor;
#ifdef USE_PMREM
        edgeReflection = threecppSafeColor(samplePMREMPrefilter(prefilteredEnvMap, safeNormalize(environmentRotation * Rg, vec3(0.0, 0.0, 1.0)), max(r, 0.02), pmremMipLevels), envSpecularColor);
#elif defined(USE_ENVMAP_EQUIRECT)
        edgeReflection = threecppSafeColor(sampleEnvEquirect(envMapEquirect, environmentRotation * Rg), envSpecularColor);
#endif
        float iorF0 = pow((ior - 1.0) / max(ior + 1.0, 0.001), 2.0);
        float fresnelIOR = iorF0 + (1.0 - iorF0) * fresnelGlass;

        // Fake in-scattering/volume body color: helps thick glass retain body and
        // tint without becoming a flat opaque diffuse surface.
        float absorptionAmount = 1.0 - dot(clamp(transmittance, 0.0, 1.0), vec3(0.3333));
        vec3 volumeScatter = attenuationColor * absorptionAmount * 0.08 * (0.25 + envMapIntensity);

        float glassMix = clamp(transmissionFactor * (1.0 - 0.10 * m), 0.0, 1.0);
        vec3 refractiveBody = transmitted + volumeScatter;
        vec3 glassColor = mix(refractiveBody, edgeReflection, clamp(fresnelIOR * (0.55 + 0.35 * transmissionFactor), 0.0, 0.92));
        // Keep a small amount of local lighting for shape readability, but let
        // transmission dominate so GLB glass no longer looks opaque.
        // Match the three.js strategy more closely: transmission replaces the
        // local diffuse body for transmissive materials rather than alpha
        // blending a faded shell over it.  Keep only a very small local lighting
        // contribution for silhouettes; refraction + Fresnel reflection should
        // dominate the dragon body.
        total = mix(total * 0.04, glassColor, glassMix);
        total += edgeReflection * fresnelIOR * (0.28 + 0.22 * transmissionFactor);
        materialTransmissionVisibility = max(materialTransmissionVisibility, glassMix);
        // three.js transmission is an opaque refractive surface, not ordinary
        // alpha transparency. Keep alpha at 1 so depth/order remain stable.
        alpha = 1.0;
#else
        if (transmissionDebugMode == 4) { outColor = vec4(vec3(clamp(opticalThickness / 4.0, 0.0, 1.0)), 1.0); return; }
        if (transmissionDebugMode == 5) { outColor = vec4(vec3(clamp(fresnelGlass, 0.0, 1.0)), 1.0); return; }
        if (transmissionDebugMode == 6) { outColor = vec4(vec3(0.0), 1.0); return; }
        vec3 fallbackGlass = baseColor * transmittance * (ambientLightColor + envMapIntensity * vec3(0.75));
        fallbackGlass = mix(fallbackGlass, envSpecularColor, fresnelGlass * 0.5);
        total = mix(total * 0.08, fallbackGlass, transmissionFactor * (1.0 - m));
        materialTransmissionVisibility = max(materialTransmissionVisibility, transmissionFactor);
        alpha = 1.0;
#endif
    }
#endif
    total = threecppSafeColor(total, max(baseColor, vec3(0.15)));
#ifdef USE_PHYSICAL
    if (materialTransmissionVisibility < 0.001) {
        total = max(total, baseColor * 0.18 + vec3(0.02));
    } else {
        // Do not force a white diffuse floor on transmissive glTF glass; that
        // makes opaque milky plastic instead of a refractive surface.
        total = max(total, vec3(0.0));
    }
#else
    total = max(total, baseColor * 0.18 + vec3(0.02));
#endif
    #ifdef USE_FOG
    total = applyFog(total, vWorldPosition, cameraPosition);
#endif
    outColor = finalOutput(total, max(threecppSafeAlpha(alpha), 0.35));
}
)GLSL";

    } else if (key.materialType == MaterialType::MeshNormal) {
        fragment += common_fragment() + R"GLSL(
uniform vec3 diffuse;
uniform float opacity;
uniform vec2 normalScale;
#ifdef USE_BUMPMAP
uniform sampler2D bumpMap;
uniform float bumpScale;
#endif
#ifdef USE_NORMALMAP
uniform sampler2D normalMap;
#endif
in vec2 vUv;
in vec3 vNormal;
in vec3 vViewPosition;
in vec3 vWorldPosition;
out vec4 outColor;
void main() {
    float a = opacity;
    if (a < alphaTest) discard;
#ifdef USE_CLIPPING
    checkClipping(vWorldPosition);
#endif
    vec3 N = normalize(vNormal);
#ifdef USE_FLAT_SHADING
    vec3 flatN = cross(dFdx(vWorldPosition), dFdy(vWorldPosition));
    N = normalize(flatN);
    if (dot(N, normalize(cameraPosition - vWorldPosition)) < 0.0) N = -N;
#endif
#ifdef USE_NORMALMAP
    N = perturbNormal(N, normalize(vViewPosition), vUv, normalMap, normalScale);
#elif defined(USE_BUMPMAP)
    N = perturbNormalBump(N, normalize(vViewPosition), vUv, bumpMap, bumpScale);
#endif
    vec3 normalColor = N * 0.5 + 0.5;
    vec3 foggedColor = normalColor;
#ifdef USE_FOG
    foggedColor = applyFog(foggedColor, vWorldPosition, cameraPosition);
#endif
    outColor = finalOutput(foggedColor, max(a, 0.35));
}
)GLSL";
    } else if (key.materialType == MaterialType::MeshMatcap) {
        fragment += common_fragment() + R"GLSL(
uniform vec3 diffuse;
uniform float opacity;
uniform vec2 normalScale;
#ifdef USE_BUMPMAP
uniform sampler2D bumpMap;
uniform float bumpScale;
#endif
#ifdef USE_NORMALMAP
uniform sampler2D normalMap;
#endif
uniform sampler2D matcap;
#ifdef USE_MAP
uniform sampler2D map;
#endif
#ifdef USE_ALPHAMAP
uniform sampler2D alphaMap;
#endif
in vec2 vUv;
in vec3 vColor;
in vec3 vNormal;
in vec3 vViewPosition;
in vec3 vWorldPosition;
out vec4 outColor;
void main() {
    vec4 texel = vec4(1.0);
#ifdef USE_MAP
    texel *= texture(map, vUv);
    texel.rgb = srgbToLinear(texel.rgb);
#endif
    float a = opacity * texel.a;
#ifdef USE_ALPHAMAP
    a *= texture(alphaMap, vUv).r;
#endif
    if (a < alphaTest) discard;
    vec3 N = normalize(vNormal);
#ifdef USE_FLAT_SHADING
    vec3 flatN = cross(dFdx(vWorldPosition), dFdy(vWorldPosition));
    N = normalize(flatN);
    if (dot(N, normalize(cameraPosition - vWorldPosition)) < 0.0) N = -N;
#endif
#ifdef USE_NORMALMAP
    N = perturbNormal(N, normalize(vViewPosition), vUv, normalMap, normalScale);
#elif defined(USE_BUMPMAP)
    N = perturbNormalBump(N, normalize(vViewPosition), vUv, bumpMap, bumpScale);
#endif
    vec3 V = normalize(cameraPosition - vWorldPosition);
    vec3 R = reflect(-V, N);
    float m = 2.0 * sqrt(R.x * R.x + R.y * R.y + (R.z + 1.0) * (R.z + 1.0));
    vec2 matUv = vec2(R.x / m + 0.5, R.y / m + 0.5);
    vec3 matcapColor = srgbToLinear(texture(matcap, matUv).rgb);
    vec3 finalColor = diffuse * vColor * texel.rgb * matcapColor;
    vec3 foggedColor = threecppSafeColor(finalColor, diffuse);
#ifdef USE_FOG
    foggedColor = applyFog(foggedColor, vWorldPosition, cameraPosition);
#endif
    outColor = finalOutput(foggedColor, max(a, 0.35));
}
)GLSL";
    } else if (key.materialType == MaterialType::MeshToon) {
        fragment += common_fragment() + R"GLSL(
uniform vec3 diffuse;
uniform vec3 emissive;
uniform float opacity;
uniform vec3 specularColor;
uniform float specularIntensity;
uniform vec2 normalScale;
uniform float lightMapIntensity;
uniform float aoMapIntensity;
#ifdef USE_BUMPMAP
uniform sampler2D bumpMap;
uniform float bumpScale;
#endif
#ifdef USE_NORMALMAP
uniform sampler2D normalMap;
#endif
#ifdef USE_MAP
uniform sampler2D map;
#endif
#ifdef USE_ALPHAMAP
uniform sampler2D alphaMap;
#endif
#ifdef USE_EMISSIVEMAP
uniform sampler2D emissiveMap;
#endif
#ifdef USE_AOMAP
uniform sampler2D aoMap;
#endif
#ifdef USE_LIGHTMAP
uniform sampler2D lightMap;
#endif
in vec2 vUv;
in vec2 vUv2;
in vec3 vColor;
in vec3 vNormal;
in vec3 vWorldPosition;
in vec3 vViewPosition;
out vec4 outColor;
void main() {
    vec4 texel = vec4(1.0);
#ifdef USE_MAP
    texel *= texture(map, vUv);
    texel.rgb = srgbToLinear(texel.rgb);
#endif
    float a = opacity * texel.a;
#ifdef USE_ALPHAMAP
    a *= texture(alphaMap, vUv).r;
#endif
    if (a < alphaTest) discard;
    vec3 baseColor = diffuse * vColor * texel.rgb;
    vec3 N = normalize(vNormal);
    vec3 V = normalize(cameraPosition - vWorldPosition);
#ifdef USE_FLAT_SHADING
    vec3 flatN = cross(dFdx(vWorldPosition), dFdy(vWorldPosition));
    N = normalize(flatN);
    if (dot(N, V) < 0.0) N = -N;
#endif
#ifdef USE_NORMALMAP
    N = perturbNormal(N, normalize(vViewPosition), vUv, normalMap, normalScale);
#elif defined(USE_BUMPMAP)
    N = perturbNormalBump(N, normalize(vViewPosition), vUv, bumpMap, bumpScale);
#endif
    vec3 total = baseColor * (ambientLightColor + hemisphereIrradiance(N, hemisphereSkyColor + envSkyColor, hemisphereGroundColor + envGroundColor)) / PI;
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (i >= lightCount) break;
        GpuLight l = lights[i];
        vec3 L = vec3(0.0);
        vec3 radiance = l.colorIntensity.rgb * l.colorIntensity.a;
        if (l.type == 1) {
            L = normalize(-l.directionCone.xyz);
        } else if (l.type == 2 || l.type == 3 || l.type == 4) {
            vec3 toLight = l.positionRange.xyz - vWorldPosition;
            float dist2 = max(dot(toLight, toLight), 0.0001);
            float dist = sqrt(dist2);
            L = toLight / dist;
            float range = l.positionRange.w;
            float decay = l.params.x;
            float attenuation = decay <= 0.0001 ? 1.0 : 1.0 / max(pow(dist, decay), 0.01);
            if (range > 0.0) {
                float cutoff = max(1.0 - pow(dist / range, 4.0), 0.0);
                attenuation *= cutoff * cutoff;
            }
            if (l.type == 3) {
                float spotCos = dot(normalize(l.directionCone.xyz), -L);
                float outerCone = l.directionCone.w;
                float innerCone = l.params.y;
                float coneAttenuation = (innerCone - outerCone) < 0.0001 ? step(outerCone, spotCos) : smoothstep(outerCone, innerCone, spotCos);
                attenuation *= coneAttenuation;
            }
            radiance *= attenuation;
        }
        float shadow = 1.0;
#ifdef USE_SHADOWMAP
        float pbrShadow = sampleShadowMap(l.shadowIndex, vWorldPosition);
        if (!isnan(pbrShadow) && !isinf(pbrShadow)) shadow = clamp(pbrShadow, 0.0, 1.0);
#endif
        radiance *= mix(1.0, shadow, 0.85);
        float NoL = saturate(dot(N, L));
        float toon = smoothstep(0.0, 0.08, NoL);
        total += baseColor * radiance * toon / PI;
        vec3 H = normalize(V + L);
        float NoH = saturate(dot(N, H));
        float spec = pow(NoH, mix(8.0, 96.0, 0.5));
        total += specularColor * specularIntensity * spec * radiance * toon * 0.5;
    }
#ifdef USE_AOMAP
    total *= mix(1.0, texture(aoMap, vUv2).r, aoMapIntensity);
#endif
#ifdef USE_LIGHTMAP
    total += baseColor * srgbToLinear(texture(lightMap, vUv2).rgb) * lightMapIntensity;
#endif
#ifdef USE_EMISSIVEMAP
    total += emissive * srgbToLinear(texture(emissiveMap, vUv).rgb);
#else
    total += emissive;
#endif
    total = threecppSafeColor(total, baseColor);
    total = max(total, baseColor * 0.12);
#ifdef USE_FOG
    total = applyFog(total, vWorldPosition, cameraPosition);
#endif
    outColor = finalOutput(total, max(a, 0.35));
}
)GLSL";
    } else if (key.materialType == MaterialType::Shadow) {
        fragment += common_fragment() + R"GLSL(
uniform vec3 diffuse;
uniform float opacity;
in vec3 vWorldPosition;
out vec4 outColor;
void main() {
    float a = opacity;
    if (a < alphaTest) discard;
    vec3 shadowColor = diffuse;
    float sh = 0.0;
    int numShadowsFound = 0;
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (i >= lightCount) break;
        GpuLight l = lights[i];
        if (l.shadowIndex >= 0 && l.shadowIndex < shadowMapCount) {
            float s = sampleShadowMap(l.shadowIndex, vWorldPosition);
            if (!isnan(s) && !isinf(s)) { sh += clamp(s, 0.0, 1.0); numShadowsFound++; }
        }
    }
    if (numShadowsFound > 0) sh /= float(numShadowsFound);
    vec3 finalColor = mix(shadowColor, vec3(1.0), sh);
    vec3 foggedColor = threecppSafeColor(finalColor, shadowColor);
#ifdef USE_FOG
    foggedColor = applyFog(foggedColor, vWorldPosition, cameraPosition);
#endif
    outColor = finalOutput(foggedColor, max(a, 0.35));
}
)GLSL";
    } else {
        fragment += common_fragment() + R"GLSL(
uniform vec3 diffuse;
uniform float opacity;
#ifdef USE_MAP
uniform sampler2D map;
#endif
#ifdef USE_ALPHAMAP
uniform sampler2D alphaMap;
#endif
in vec2 vUv;
in vec2 vUv2;
in vec3 vColor;
in float vLineDistance;
out vec4 outColor;
void main() {
    vec4 texel = vec4(1.0);
#ifdef USE_MAP
    texel *= texture(map, vUv);
    texel.rgb = srgbToLinear(texel.rgb);
#endif
#ifdef USE_ALPHAMAP
    texel.a *= texture(alphaMap, vUv).r;
#endif
    float a = opacity * texel.a;
    if (materialDebugMode == 1) { outColor = vec4(max(diffuse * vColor, vec3(0.15)), max(a, 0.35)); return; }
#ifdef USE_DASHED_LINE
    float dashTotal = max(0.0001, dashSize + gapSize);
    if (mod(vLineDistance * dashScale, dashTotal) > dashSize) discard;
#endif
    if (a < alphaTest) discard;
    vec3 visibleColor = max(diffuse * vColor * texel.rgb, vec3(0.12) * diffuse);
    vec3 foggedColor = threecppSafeColor(visibleColor, diffuse);
#ifdef USE_FOG
    foggedColor = applyFog(foggedColor, vWorldPosition, cameraPosition);
#endif
    outColor = finalOutput(foggedColor, max(threecppSafeAlpha(a), 0.35));
}
)GLSL";
    }
    return {vertex, fragment};
}

} // namespace THREE
