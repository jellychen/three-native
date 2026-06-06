# v6.0.27 PBR Shadow Sampling Stability

This version restores shadow sampling in the experimental safe PBR path while keeping the renderer visible-first during the v6.0 regression phase.

## Added

- Directional / Spot shadow sampling is restored for `MeshStandardMaterial` / `MeshPhysicalMaterial` under `THREECPP_ENABLE_EXPERIMENTAL_PBR=1`.
- Shadow contribution is bounded and sanitized:
  - invalid shadow index returns `1.0`
  - NaN / Inf shadow results are clamped to `1.0`
  - final shadow factor is clamped to `[0, 1]`
  - attenuation uses `mix(1.0, shadow, 0.85)` so bad shadow maps do not make the whole material disappear.
- Added `THREECPP_DISABLE_PBR_SHADOWS=1` to disable PBR shadow sampling without disabling other PBR features.

## Test

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r

THREECPP_ENABLE_EXPERIMENTAL_PBR=1 xmake run 14_cache_dashed_texture_transform
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 xmake run 16_pbr_material_gallery
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 xmake run 36_shadow_map_parity_lab
```

Disable shadows for A/B testing:

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_DISABLE_PBR_SHADOWS=1 xmake run 36_shadow_map_parity_lab
```
