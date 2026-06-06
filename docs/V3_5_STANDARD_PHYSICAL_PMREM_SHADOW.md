# v3.5 Standard / Physical / PMREM / Shadow Focus

This version focuses on the renderer features that most affect visual parity with three.js WebGLRenderer.

## MeshStandardMaterial

Added or tightened the following paths:

- baseColor/map, alphaMap/alphaTest
- roughness/metalness and roughnessMap/metalnessMap channels
- normalMap with derivative TBN fallback
- aoMap/lightMap/emissiveMap intensity uniforms
- texture UV transform path
- ACES tone mapping and linear/output conversion
- split-sum BRDF approximation for environment specular

## PMREM / HDR IBL

The renderer now exposes a more three.js-like PMREM/IBL path:

- `TextureFactory::makeStudioHDRI()` procedural HDRI-style equirectangular test source
- `PMREMGenerator::fromEquirectangular()` keeps background/environment ownership consistent
- shader uses a split-sum BRDF approximation (`PMREM_BRDF_Approx`) instead of plain Fresnel-only env specular
- roughness now affects the analytic PMREM fallback more aggressively

This is still not a full GPU cubemap convolution pipeline. The next PMREM step is real equirectangular-to-cubemap, irradiance convolution, GGX prefiltered mip chain, and a physically integrated BRDF LUT.

## MeshPhysicalMaterial

The public material shape is closer to three.js:

- clearcoat / clearcoatRoughness / clearcoat maps
- sheen / sheenColor / sheenRoughness maps
- transmission / thickness / attenuation
- ior / specularIntensity / specularColor
- iridescence / iridescence thickness maps
- anisotropy / anisotropyMap / anisotropyRotation
- dispersion parameter for transmission chromatic offset

The shader includes approximate paths for clearcoat, sheen, iridescence, anisotropy, transmission framebuffer sampling, and dispersion. These are forward-renderer approximations; they are not yet a byte-for-byte clone of three.js shader chunks.

## ShadowMap

- DirectionalLight and SpotLight shadow path retained
- PCF uses per-shadow-map texel size
- shadow camera parameters are exposed on `LightShadow`
- skinned and morph target casters are now wired into the depth shader permutation
- moving caster / moving light test added

PointLight cubemap shadow is still intentionally not marked complete.

## Tests

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 25_standard_physical_pmrem_shadow_lab
xmake run 26_physical_material_parity_lab
xmake run 27_shadow_completeness_lab
```

## New examples

- `25_standard_physical_pmrem_shadow_lab`: Standard + Physical materials under HDRI/PMREM fallback and multi-light shadows.
- `26_physical_material_parity_lab`: clearcoat, sheen, transmission, iridescence, anisotropy, dispersion.
- `27_shadow_completeness_lab`: Directional/Spot shadows, moving lights and moving casters.
