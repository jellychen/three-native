# v6.0.28 PBR Shadow Gate

This stability build keeps the repaired experimental PBR path visible by default.

Observed regression:

- `THREECPP_ENABLE_EXPERIMENTAL_PBR=1` works without PBR shadows.
- Enabling the `USE_SHADOWMAP` branch made all mesh fragments disappear while line shaders still rendered.
- `THREECPP_DISABLE_PBR_SHADOWS=1` restored visibility.

Fix:

- `MeshStandardMaterial` and `MeshPhysicalMaterial` no longer compile the PBR shadow sampling branch by default when `THREECPP_ENABLE_EXPERIMENTAL_PBR=1` is set.
- To explicitly test the broken/experimental shadow sampling branch, set:

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 xmake run 36_shadow_map_parity_lab
```

Recommended tests:

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 xmake run 14_cache_dashed_texture_transform
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 xmake run 16_pbr_material_gallery
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 xmake run 36_shadow_map_parity_lab
```

Next step:

- Rebuild PBR shadow sampling as a dedicated safe branch with a minimal single directional shadow first, then Spot and Point shadows.
