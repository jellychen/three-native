# v0.9 Gap Closure Toward three.js Rendering Capability

This release starts closing the gap module-by-module instead of adding empty API shells.

## Current honest capability estimate

| Area | v0.8.1 estimate | v0.9 estimate | Notes |
|---|---:|---:|---|
| macOS GLFW native OpenGL test | 35% | 40% | Still needs user-side build/run validation on macOS. |
| Scene/Object3D/Camera | 35% | 35% | Basic matrix and camera flow exists. Missing layers/frustum/clipping. |
| Basic materials | 20% | 30% | MeshBasic already existed; v0.9 adds lit Lambert/Phong shader paths. |
| Line/Points/FatLine | 20% | 25% | Still not full Line2 parity; screen-space fatline exists. |
| MeshStandardMaterial PBR | 15% | 22% | Better hemisphere/env fallback IBL, direct lights, texture hooks. |
| MeshPhysicalMaterial | 10% | 15% | Parameters and approximate transmission exist. Full three.js framebuffer path is still incomplete. |
| IBL/environment | 5% | 15% | v0.9 adds procedural environment fallback uniforms. Full PMREM is still missing. |
| Lights | 15% | 22% | Ambient/Hemisphere/Directional/Point/Spot are more consistently consumed by shaders. |
| Shadows | 5% | 5% | Still mostly interface-level. |
| Assimp/glTF import | 10% | 10% | Not advanced in this release. |
| Skeletal animation | 8% | 8% | Not advanced in this release. |
| Overall renderer parity | <20% | ~20% | Still far from three.js, but the real rendering path is getting thicker. |

## What was concretely improved in v0.9

### 1. Environment fallback usable by PBR

`Environment` now has analytic fallback fields:

- `skyColor`
- `groundColor`
- `specularColor`
- `backgroundIntensity`
- `backgroundBlurriness`

The shader now receives:

- `envSkyColor`
- `envGroundColor`
- `envSpecularColor`
- `hemisphereSkyColor`
- `hemisphereGroundColor`

This is not PMREM yet, but PBR no longer depends only on flat ambient color.

### 2. PBR IBL approximation improved

The previous IBL fallback used a constant scalar approximation. v0.9 now uses:

- normal-based hemisphere irradiance for diffuse IBL
- reflection-vector based approximate specular environment
- roughness-aware specular attenuation
- F0/Fresnel-aware diffuse/specular split

This is still approximate, but closer to the structure three.js uses before replacing it with real PMREM textures.

### 3. MeshLambertMaterial and MeshPhongMaterial shader paths

v0.9 adds non-PBR lit material shader paths instead of treating them like basic materials.

Supported pieces:

- diffuse color
- map
- emissive / emissiveMap
- aoMap for Lambert
- normalMap for Phong
- specular / specularMap / shininess approximation for Phong
- Ambient / Hemisphere / Directional / Point / Spot lights

### 4. Scene background fallback

When there is no background texture pass yet:

- `Scene::backgroundColor` clears the framebuffer.
- If `Scene::environment` exists, its `skyColor * backgroundIntensity` is used as a temporary background color.

This is a bridge until a proper WebGLBackground-equivalent pass is implemented.

## Remaining one-by-one closure order

1. **v1.0: make macOS GLFW test compile/run cleanly**
   - Fix all real local compile errors.
   - Validate shader compilation.
   - Add visible debug output for GL version and renderer info.

2. **v1.1: finish MeshStandardMaterial direct lighting**
   - Strict color-space pipeline.
   - Correct direct-light energy terms.
   - Full normal/tangent pipeline.
   - Texture transform.

3. **v1.2: real HDR background + equirectangular sampler**
   - stb_image HDR loading.
   - Background sphere/triangle shader.
   - Equirectangular env lookup.

4. **v1.3: real PMREM-style IBL**
   - equirectangular -> cubemap.
   - irradiance convolution.
   - roughness prefilter mip chain.
   - BRDF LUT.

5. **v1.4: shadow maps**
   - directional shadow.
   - spot shadow.
   - point cubemap shadow.
   - PCF / bias / normalBias.

6. **v1.5: glTF-focused Assimp import**
   - PBR material mapping.
   - embedded/relative textures.
   - alpha modes.
   - skin and animation import.

7. **v1.6: skeletal animation real path**
   - animation sampling.
   - bone hierarchy.
   - GPU skinning validation.
   - bone texture fallback for large skeletons.

8. **v1.7: MeshPhysicalMaterial parity pass**
   - real transmission render target exclusion.
   - roughness mip transmission blur.
   - volume attenuation.
   - clearcoat normal.
   - sheen/iridescence/anisotropy refinements.

9. **v1.8+: renderer systems**
   - clipping.
   - layers.
   - morph targets.
   - instancing.
   - multi-material groups.
   - WebGLRenderLists/WebGLPrograms/WebGLTextures-style cache parity.
