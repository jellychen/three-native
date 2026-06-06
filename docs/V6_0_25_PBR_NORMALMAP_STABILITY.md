# V6.0.25 PBR normalMap / TBN stability

This iteration restores normal-map support inside the safe experimental PBR path.

## Changes

- `THREECPP_ENABLE_EXPERIMENTAL_PBR=1` now supports `USE_NORMALMAP` in the Standard/Physical forward shader.
- Normal perturbation uses derivative-based TBN fallback via `perturbNormal()`.
- Meshes without explicit tangent attributes can still render normal maps correctly enough for glTF/FBX validation.
- `USE_BUMPMAP` is restored for the safe PBR path when no normal map is present.
- Safe fallback still avoids the unstable legacy full PBR/shadow/transmission branch.

## Test commands

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r

THREECPP_ENABLE_EXPERIMENTAL_PBR=1 xmake run 14_cache_dashed_texture_transform
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 xmake run 16_pbr_material_gallery
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_FORCE_NORMAL_VIEW=1 xmake run 16_pbr_material_gallery
```

## Next incremental restores

1. Re-enable safe shadow sampling in PBR.
2. Re-enable PMREM specular precision path.
3. Re-enable Physical transmission framebuffer branch.
