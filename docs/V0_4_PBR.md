# threecpp renderer v0.4 PBR implementation notes

v0.4 focuses on making `MeshStandardMaterial` materially useful instead of remaining a placeholder.

## Implemented in v0.4

- `MeshStandardMaterial` parameters now include:
  - `color`
  - `roughness`
  - `metalness`
  - `emissive` / `emissiveIntensity`
  - `map`
  - `normalMap`
  - `roughnessMap`
  - `metalnessMap`
  - `aoMap`
  - `emissiveMap`
  - `normalScale`
  - `aoMapIntensity`
  - `envMapIntensity`
- GLSL PBR shader path now includes:
  - Cook-Torrance BRDF
  - GGX / Trowbridge-Reitz normal distribution
  - Smith correlated visibility
  - Schlick Fresnel
  - Lambert diffuse energy split by metalness/Fresnel
  - ACES filmic tone mapping
  - sRGB-to-linear sampling for color/emissive maps
  - linear sampling for roughness/metalness/AO maps
  - normal mapping via screen-space TBN fallback
- GL texture resource cache added:
  - 2D texture upload
  - sampler binding
  - white fallback texture
  - normal fallback texture
  - mipmap generation
  - wrap/filter mapping
- PBR demo upgraded:
  - UV spheres instead of cube placeholders
  - roughness/metalness material grid
  - procedural checkerboard base color texture example
- Geometry helpers upgraded:
  - UV sphere factory
  - cube UVs
- `Environment` header fixed to remove duplicate `CubeTexture` class declaration from v0.3.

## Current limitations

- PMREM convolution is still API-level; shader uses an ambient/specular IBL approximation until real cubemap irradiance/prefilter sampling lands.
- No tangent attribute pipeline yet. Normal mapping uses a derivative-based TBN path, which is acceptable for many cases but not equivalent to glTF tangent-space parity.
- No shadow integration in the PBR direct-light path yet.
- `MeshPhysicalMaterial` still inherits the standard path; clearcoat/transmission/sheen/iridescence parameters are stored but not shaded yet.

## Next PBR tasks

1. Add tangent attribute generation/import and pass tangent to shader.
2. Implement cubemap texture upload and PMREM prefilter sampling.
3. Add BRDF LUT support for specular IBL.
4. Add shadow map sampling into the PBR direct-light path.
5. Expand `MeshPhysicalMaterial` with clearcoat, sheen, transmission, thickness and attenuation.
