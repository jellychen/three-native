# v6.0.42 Default Safe PBR / Old Example Visibility Fix

This version fixes older examples such as `10_pmrem_lut`, `11_material_geometry_matrix`, `12_shadow_map`, and `13_v2_0_threejs_stack` rendering only the blue clear background when run without `THREECPP_ENABLE_EXPERIMENTAL_PBR=1`.

## Root cause

The renderer had two material paths:

- default path: legacy fallback / old material shaders
- experimental path: repaired safe PBR shader path

After the v6.0 shader recovery, the repaired path was only enabled when `THREECPP_ENABLE_EXPERIMENTAL_PBR=1` was set. Older examples were run without that variable, so they could still select the old broken mesh material branch and render only the background.

## Fix

`MeshStandardMaterial` and `MeshPhysicalMaterial` now use the repaired safe PBR forward path by default.

Compatibility switches:

```bash
# default: safe PBR enabled
xmake run 11_material_geometry_matrix

# explicit safe PBR, still supported
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 xmake run 11_material_geometry_matrix

# force legacy fallback if needed for debugging
THREECPP_DISABLE_EXPERIMENTAL_PBR=1 xmake run 11_material_geometry_matrix
```

## Expected tests

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r

xmake run 10_pmrem_lut
xmake run 11_material_geometry_matrix
xmake run 12_shadow_map
xmake run 13_v2_0_threejs_stack
```

These examples should no longer render only the blue background.
