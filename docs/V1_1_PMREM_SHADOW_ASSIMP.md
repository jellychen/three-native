# v1.1 PMREM / shadow / Assimp evolution

This version continues closing the gap with three.js WebGLRenderer. It is still not a full clone, but it adds stronger API and data-flow foundations for the next renderer-level passes.

## Added

- `TextureMapping` metadata: UV, equirectangular reflection/refraction, cube reflection/refraction.
- Texture transform metadata: offset, repeat, center, rotation, flipY, anisotropy.
- `TextureFactory::makeEquirectangularGradient()` for deterministic env tests.
- `TextureFactory::makeBRDFLUT()` as a lightweight split-sum LUT placeholder.
- `PMREMOptions` and richer `PMREMGenerator` state.
- `Environment::hasPMREM`, `pmremCubeSize`, `pmremMipLevels`.
- Placeholder irradiance/prefilter cubemaps with mip-face storage.
- `PMREMGenerator::roughnessToMip()` matching the roughness-to-mip concept in three.js PMREM.
- `GLShadowMap` now builds per-light shadow render items and shadow matrices for directional/spot/point lights.
- Assimp material conversion now records common glTF PBR texture slots and color-space metadata.
- New example: `10_pmrem_lut`.

## What this fixes in the roadmap

- Moves IBL from pure shader fallback toward a PMREM-owned environment asset.
- Creates the correct renderer-side shadow scheduling shape before adding depth rendering.
- Starts mapping Assimp/glTF PBR textures into the material slots expected by the PBR shaders.

## Still missing before three.js parity

- GPU equirectangular-to-cubemap conversion.
- Irradiance convolution.
- GGX prefiltered mip generation.
- Full BRDF LUT integration in the shader.
- Depth render target shadow pass and PCF sampling in PBR shaders.
- Actual image decode for Assimp texture paths.
- KHR_materials_* extension extraction.

## Run

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 10_pmrem_lut
```
