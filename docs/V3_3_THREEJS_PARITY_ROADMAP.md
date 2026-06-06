# threecpp renderer v3.3 — realtime three.js parity stack

v3.3 is a consolidation milestone. It does **not** claim source-level or feature-complete equality with three.js, but it organizes the renderer around the same major runtime responsibilities as `WebGLRenderer`:

- asset validation for glTF / GLB / FBX / OBJ through Assimp
- PBR material gallery and physical material parameter coverage
- morph target / blend shape data path
- skeleton and animation test paths
- shadow map path
- environment / PMREM API surface
- Line / Points / FatLine / helpers
- render queue and cache diagnostics
- postprocessing API surface

## New modules

### `src/validation/GltfValidationSuite.hpp`

A lightweight validation utility that walks an imported scene and counts:

- objects
- meshes
- skinned meshes
- unique materials
- unique textures
- skeletons / bones
- morph target objects / morph target count
- animations / tracks
- lights

This is meant to mirror the kind of checklist used when comparing imported results with three.js `GLTFLoader`.

### `src/renderer/RenderQueues.hpp`

Classifies render items into:

- opaque
- transmissive
- transparent

This is a small but important step toward the real three.js-style render queue split used by PBR, transparent sorting and transmission rendering.

### `src/debug/DebugViews.hpp`

Adds debug-view state and a material override guard for wireframe / double-side diagnostics. This is intended for editor workflows such as material override, normal debugging and selection preview.

### `src/postprocessing/Passes.hpp`

Adds API placeholders for:

- `ShaderPass`
- `ToneMappingPass`
- `BloomPass`
- `OutlinePass`
- `FXAAPass`

The current implementation preserves the composer chain and API. Full full-screen triangle shader execution is the next implementation step.

## New examples

### `19_gltf_validation_suite`

Requires Assimp:

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
xmake run 19_gltf_validation_suite /path/to/model.glb
```

It loads an asset, prints scene/material/texture/animation/morph/skeleton stats, then displays the model with OrbitControls, PBR environment, grid, axes and shadows.

### `20_postprocessing_stack`

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 20_postprocessing_stack
```

Exercises `EffectComposer`, `RenderPass`, `ToneMappingPass`, `BloomPass`, and `FXAAPass` with a PBR material gallery scene.

### `21_v3_3_runtime_cache`

```bash
xmake run 21_v3_3_runtime_cache
```

Builds a shared-geometry/shared-material stress scene to exercise geometry/material/program reuse and renderer info reporting.

## Remaining gaps to real three.js parity

The following are still not one-to-one with three.js and remain high-priority after v3.3:

1. Real GPU PMREM convolution: equirectangular -> cubemap -> irradiance -> prefiltered GGX mip chain -> BRDF LUT.
2. Full `MeshStandardMaterial` shader parity: TBN reconstruction, all map channels, flatShading, clipping, fog, wireframe and exact color management.
3. Full `MeshPhysicalMaterial`: clearcoat, sheen, transmission framebuffer, volume attenuation, iridescence and anisotropy shader parity.
4. glTF extension coverage and validation against public Khronos sample models.
5. AnimationMixer parity: crossfade, additive blending, time warping, event hooks and property binding cache.
6. FatLine parity with three.js examples: near-plane trim, world units, dashes and alpha-to-coverage.
7. Full-screen postprocessing implementation: RenderTarget ping-pong, depth texture, bloom, FXAA/SMAA and outline.
8. Shadow parity: point-light cubemap shadows, PCFSoft/VSM, normalBias, transparent/skinned/morphed shadow paths.
9. Performance parity: stable ProgramKey hashing, uniform caching, VAO/material/texture lifetime control, render list reuse, instancing and multi-material groups.

