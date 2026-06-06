# v4.4 MeshPhysicalMaterial parity

This version advances the realtime renderer toward the three.js `MeshPhysicalMaterial` behavior.

## Added / improved

- Keeps the existing renderer-level transmission target path.
- Adds a dedicated physical material parity lab: `examples/37_physical_material_parity_lab`.
- Improves clearcoat by using `clearcoatNormalMap` when present and by adding an environment clearcoat lobe.
- Improves sheen by adding an environment-facing fabric contribution.
- Keeps IOR-derived dielectric F0, specular color/intensity, Beer-Lambert attenuation, rough transmission sampling, dispersion, iridescence and anisotropy hooks.
- The test combines PMREM-style environment lighting, dynamic directional/point lights, shadows, transmission, clearcoat, sheen, specular, iridescence and anisotropy.

## Run

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 37_physical_material_parity_lab
```

## Notes

This is still not bit-exact to three.js. The remaining major gaps are full LTC/anisotropic GGX, full transmission thickness/backside pass, more accurate iridescence, physical attenuation in all transparent ordering edge cases, and glTF KHR physical material extension validation.
