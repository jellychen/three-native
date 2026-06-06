# v6.0.32 PBR PointLight cubemap shadow safe restore

This version restores a minimal, isolated PointLight cubemap shadow path for the experimental PBR renderer.

It deliberately avoids the old legacy shadow array / cube branch and uses one dedicated sampler cube:

- `USE_PBR_POINT_SHADOW`
- `pbrPointShadowMap`
- `pbrPointShadowPosition`
- `pbrPointShadowNear / Far`
- `pbrPointShadowBias / Radius / Strength`

## Test

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 xmake run 36_shadow_map_parity_lab
```

Point shadow debug view:

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 THREECPP_DEBUG_PBR_POINT_SHADOW=1 xmake run 36_shadow_map_parity_lab
```

Disable point shadow only:

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 THREECPP_ENABLE_PBR_POINT_SHADOWS=0 xmake run 36_shadow_map_parity_lab
```

Expected state:

- Directional shadow remains visible.
- Spot shadow remains visible.
- Point cubemap shadow is restored through the safe isolated path.
- PBR mesh remains visible.
