# threecpp renderer v2.0 — three.js-like realtime stack milestone

This milestone keeps the project focused on the realtime `WebGLRenderer` direction. Ray tracing was intentionally removed in v1.2.1 and is not part of this branch.

## What v2.0 adds

### Realtime renderer path

- Keeps the macOS GLFW native OpenGL path as the default development target.
- Keeps ANGLE support as an optional backend, but it is not required for local macOS testing.
- Keeps xmake as the primary build path.

### PBR / material system

- `MeshStandardMaterial` remains the canonical PBR material.
- `MeshPhysicalMaterial` keeps the three.js-compatible parameter surface for:
  - transmission
  - thickness
  - attenuation
  - ior
  - clearcoat
  - sheen
  - iridescence
  - anisotropy
  - specular color / intensity
- Shader permutations continue to route through `ProgramKey` and `ShaderLib`.

### IBL / PMREM direction

- `Environment` now has the same public concept as three.js: background, equirectangular environment, PMREM metadata, irradiance/prefilter slots and BRDF LUT slot.
- `PMREMGenerator::fromEquirectangular()` creates a renderer-compatible environment object.
- The current PMREM data is still placeholder/approximate. The API shape is stable so the future GPU convolution passes can replace the placeholder data without changing user code.

### Shadow map

- Directional and spot shadow map path exists.
- Shadow matrices, per-light scheduling and PCF sampling are connected to PBR/Lambert/Phong shaders.
- Point-light cubemap shadows and VSM are still future work.

### Animation / skinning

- `AnimationMixer` now applies a useful subset of three.js-style bindings:
  - `ObjectName.position`
  - `ObjectName.quaternion`
  - `ObjectName.scale`
- Linear, step and smooth interpolation are supported.
- `Skeleton`, `Bone` and `SkinnedMesh` are still present for GPU skinning.

### Helpers / line systems

- Added helper classes useful for editor-like tooling:
  - `AxesHelper`
  - `GridHelper`
  - `BoxHelper`
  - `SkeletonHelper`
- FatLine remains implemented as triangle-expanded screen-space line rendering, similar in spirit to three.js `Line2`.

### Texture loading

- Added a dependency-free `TextureLoader` for simple test assets:
  - PPM/P6 loader
  - PFM-to-tonemapped placeholder loader
- Production-grade image loading should still be replaced by stb_image/libktx/basisu or platform image codecs.

### New example

- `examples/13_v2_0_threejs_stack`
  - PBR material matrix
  - Physical transmission sphere
  - PMREM environment object
  - shadow-casting directional light
  - point light
  - helpers
  - FatLine curve
  - simple animation mixer usage

## Run

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 13_v2_0_threejs_stack
```

## Honest status versus three.js

This is not a one-to-one implementation of three.js yet. It is a staged C++ renderer with a three.js-like public architecture.

Approximate status after v2.0:

| Area | Status |
|---|---:|
| Scene/Object3D/Camera | medium |
| Basic mesh rendering | medium |
| Line/Points/FatLine | medium |
| MeshStandardMaterial | partial-medium |
| MeshPhysicalMaterial | partial |
| IBL/PMREM | API ready, render approximation |
| ShadowMap | directional/spot partial |
| Assimp/glTF import | partial |
| Skeleton/Animation | partial |
| Postprocessing | skeleton only |
| Overall three.js renderer parity | still far from 1:1 |

## Next strict work items

1. Replace PMREM placeholders with real GPU passes:
   - equirectangular to cubemap
   - diffuse irradiance convolution
   - GGX prefiltered mip chain
   - real split-sum BRDF LUT
2. Strengthen `MeshStandardMaterial` texture transforms and UV2 handling.
3. Finish glTF static PBR mapping through Assimp.
4. Add point-light cubemap shadow and shadow-camera controls.
5. Finish `LineDashedMaterial`, `SpriteMaterial` and helper coverage.
6. Add real image loading through stb_image or KTX/BasisU.
7. Add robust example assets and image comparisons against three.js.
