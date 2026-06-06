# v1.2 Three.js Gap Closure Notes

This v1.2.1 version removes the ray-tracing/picking module and continues the renderer evolution toward the three.js WebGLRenderer feature set. It is still not a one-to-one port of three.js; the goal of this iteration is to close concrete gaps while keeping the C++20 codebase maintainable.

## Implemented in this iteration

### PBR / material system

- Extended `MeshStandardMaterial` with additional three.js-compatible texture slots:
  - `alphaMap`
  - `bumpMap`
  - `displacementMap`
  - `lightMap`
- Added scalar controls:
  - `bumpScale`
  - `displacementScale`
  - `displacementBias`
  - `lightMapIntensity`
- Added shader permutation bits:
  - `USE_ALPHAMAP`
  - `USE_BUMPMAP`
  - `USE_DISPLACEMENTMAP`
  - `USE_LIGHTMAP`
- Added vertex-stage displacement hook for standard/physical materials.
- Added light-map contribution in Lambert/Phong/PBR style shader paths.
- Improved physical material specular map handling so legacy `specularMap` can be used as a convenience slot while still supporting glTF-style `specularIntensityMap` and `specularColorMap`.

### Geometry

Added more three.js-style primitive geometry helpers:

- `GeometryFactory::makePlaneSegments()`
- `GeometryFactory::makeCylinder()`
- `GeometryFactory::makeTorus()`

These complement the existing cube, plane, UV sphere, grid and points helpers and make it easier to build PBR material test matrices.

### Examples

Added:

- `examples/11_material_geometry_matrix`
  - PBR metalness / roughness / geometry matrix.
  - Uses sphere, torus, cylinder and plane segment geometry.
  - Includes physical transmission/clearcoat sample.

## Still missing versus three.js

### Renderer core

- Complete `WebGLPrograms` equivalent.
- Complete `WebGLTextures` equivalent.
- Complete `WebGLBindingStates` / VAO cache parity.
- Full render state stack.
- Stencil, clipping planes, scissor, viewport arrays.
- Multi-material groups and draw ranges.
- Instancing and morph targets.

### PBR

- True PMREM cubemap convolution.
- BRDF LUT shader sampling integration.
- Proper bump mapping derivative path.
- Complete clearcoat normal lobe.
- Complete sheen BRDF parity.
- Complete iridescence thin-film math.
- Complete anisotropy BRDF.
- Transmission backside thickness pass.
- Depth-aware refraction.
- Transparent/transmissive queue parity.

### IBL / environment

- Real GPU equirectangular-to-cubemap pass.
- Real irradiance convolution.
- Real GGX prefiltered mip chain.
- Environment rotation parity.
- Background blur parity.

### Shadows

- Directional / spot / point shadow depth pass.
- PCF / PCFSoft shader sampling.
- Normal bias and radius parity.
- Skinned shadow pass.
- VSM shadow path.

### Animation / skinning

- Full Assimp/glTF skeleton import.
- AnimationMixer feature parity.
- Blending, cross-fade, loop modes, additive blending.
- Bone texture fallback for large skeletons.

## Recommended next iteration

v1.3 should focus on **real shadows** or **real PMREM**, not both at once:

1. Directional shadow map depth pass + PCF sampling.
2. Or equirectangular-to-cubemap + irradiance + prefilter GPU pass.

Either one would produce a visible jump toward three.js parity.


## v1.2.1 note

Ray tracing is intentionally out of scope for this renderer branch. The project focus is real-time three.js-style rendering: PBR, IBL/PMREM, shadows, materials, geometry, animation, helpers, lines and points.
