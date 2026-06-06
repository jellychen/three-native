# v1.0 Renderer Gap Closure

This iteration continues closing the gap between the C++ renderer and the rendering behavior expected from three.js `WebGLRenderer`.

## Implemented in v1.0

### 1. Frustum culling

`GLRenderer::projectObject()` now extracts a camera frustum from:

```cpp
camera.projectionMatrix * camera.matrixWorldInverse
```

Renderable objects with a valid `geometry.boundingSphere` are culled before being inserted into `RenderList` when `Object3D::frustumCulled == true`.

This mirrors the high-level behavior of three.js object projection and prevents invisible meshes, lines and point clouds from entering the draw queue.

### 2. Real equirectangular environment sampling path

Before v1.0, `scene.environment` mostly drove analytic fallback lighting through `skyColor`, `groundColor` and `specularColor`.

v1.0 adds:

```cpp
Environment::equirectangularMap
PMREMGenerator::fromEquirectangular(texture)
ProgramKey::useEnvMapEquirect
USE_ENVMAP_EQUIRECT shader define
sampler2D envMapEquirect
```

The PBR shader now samples the environment texture directly using equirectangular coordinates derived from world-space directions:

```glsl
vec2 equirectUv(vec3 dir);
vec3 sampleEnvEquirect(sampler2D tex, vec3 dir);
```

This is still not full PMREM, but it is the first material-driven IBL texture path. Roughness blends the sharp equirectangular specular sample toward the analytic fallback until a proper prefiltered mip chain is implemented.

### 3. PBR example for environment response

New example:

```txt
examples/09_pbr_envmap
```

It creates a procedural equirectangular checker texture and uses it as `scene.environment`, then renders a roughness/metalness sphere grid.

Run on macOS native OpenGL path:

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 09_pbr_envmap
```

## Current three.js parity estimate after v1.0

Approximate current completion by subsystem:

```txt
Scene/Object3D/Camera:        40%
RenderList/state/programs:    32%
Basic materials:              35%
MeshStandardMaterial:         28%
MeshPhysicalMaterial:         18%
IBL/environment:              22%
Lights:                       25%
Shadows:                       5%
Line/Points/FatLine:          28%
Assimp/glTF import:           12%
Skinning/animation:           10%
Overall three.js parity:      23-25%
```

## Still missing for three.js-level PBR/IBL

The new equirectangular path is useful for testing but does not replace three.js PMREM.

Still required:

```txt
HDR / EXR loader
float / half-float texture pipeline validation
equirectangular -> cubemap rendering
irradiance convolution
prefiltered GGX mip chain
BRDF LUT
roughness-to-mip mapping
scene.background texture pass
background blurriness
background/environment rotation
```

## Suggested next iteration

v1.1 should focus on a real PMREM pipeline:

```txt
1. RenderTarget cubemap abstraction
2. Equirectangular-to-cubemap shader
3. Irradiance cubemap convolution
4. Prefiltered specular mip chain
5. BRDF LUT generation or embedded LUT
6. MeshStandardMaterial sampling from PMREM
```
