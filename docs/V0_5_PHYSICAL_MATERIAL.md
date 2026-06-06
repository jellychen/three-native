# threecpp renderer v0.5 physical material notes

v0.5 extends the v0.4 PBR path toward three.js `MeshPhysicalMaterial`.

## Implemented material API

`MeshPhysicalMaterial` now extends `MeshStandardMaterial` with the main three.js-style physical controls:

- `ior`
- `reflectivity`
- `transmission`
- `transmissionMap`
- `thickness`
- `thicknessMap`
- `attenuationDistance`
- `attenuationColor`
- `specularIntensity`
- `specularColor`
- `specularIntensityMap`
- `specularColorMap`
- `clearcoat`
- `clearcoatRoughness`
- `clearcoatNormalScale`
- `clearcoatMap`
- `clearcoatRoughnessMap`
- `clearcoatNormalMap`
- `sheen`
- `sheenColor`
- `sheenRoughness`
- `sheenColorMap`
- `sheenRoughnessMap`
- `iridescence`
- `iridescenceIOR`
- `iridescenceThicknessMinimum`
- `iridescenceThicknessMaximum`
- `iridescenceMap`
- `iridescenceThicknessMap`
- `anisotropy`
- `anisotropyRotation`
- `anisotropyMap`

## Shader implementation

The shader generator now emits physical-material permutations through `ProgramKey` flags:

- `USE_PHYSICAL`
- `USE_TRANSMISSION`
- `USE_TRANSMISSIONMAP`
- `USE_THICKNESSMAP`
- `USE_SPECULARMAP`
- `USE_CLEARCOAT`
- `USE_CLEARCOATMAP`
- `USE_CLEARCOAT_ROUGHNESSMAP`
- `USE_CLEARCOAT_NORMALMAP`
- `USE_SHEEN`
- `USE_SHEEN_COLORMAP`
- `USE_SHEEN_ROUGHNESSMAP`
- `USE_IRIDESCENCE`
- `USE_IRIDESCENCEMAP`
- `USE_IRIDESCENCE_THICKNESSMAP`
- `USE_ANISOTROPY`
- `USE_ANISOTROPYMAP`

The v0.5 physical shader adds:

- IOR-derived dielectric F0
- specular color/intensity modulation
- clearcoat second specular lobe
- sheen grazing lobe approximation
- iridescence tint approximation
- Beer-Lambert attenuation helper
- forward-rendering transmission approximation

## Important limitation

The transmission implementation is intentionally not claimed to be identical to three.js yet. three.js `MeshPhysicalMaterial.transmission` relies on a more involved renderer path, including framebuffer sampling and renderer-side transmission render targets. v0.5 does not yet do this.

Current v0.5 behavior:

```text
transmission ≈ mix(lit-surface, attenuated environment/ambient approximation)
```

This is useful for API compatibility, shader-permutation design, material import and early visual tuning, but it is not real refractive background lookup.

## Remaining parity checklist against three.js

To approach three.js more closely, the renderer still needs:

1. Transmission render target pass.
2. Backside transmission thickness pass.
3. Proper refracted background lookup using view-space thickness.
4. Chromatic aberration / dispersion path.
5. Correct clearcoat normal-map lobe.
6. Full sheen BRDF model matching three.js chunks.
7. Full iridescence Fresnel model.
8. Anisotropic GGX tangent-frame implementation.
9. PMREM prefiltered cubemap sampling with roughness mip selection.
10. BRDF LUT integration.
11. Shadow-map integration into direct-light PBR.
12. Render target / EffectComposer parity.
13. Color-management parity with modern three.js output color spaces.

## New example

`examples/06_physical_transmission` creates three spheres:

- transmission glass approximation
- clearcoat/sheen/iridescence material
- physical metal with specular controls

This example is designed to verify that `MeshPhysicalMaterial` parameters are wired from C++ material objects into program keys, GLSL defines and uniforms.
