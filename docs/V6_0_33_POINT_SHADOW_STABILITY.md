# V6.0.33 PointLight Shadow Stability

This build stabilizes the safe PBR PointLight cubemap shadow path.

## Changes

- Point shadow bias is treated as world-space distance and normalized by `cameraFar` in shader.
- Point shadow radius is now interpreted as cubemap texels instead of world/far distance.
- Added `pbrPointShadowMapSize` uniform.
- Replaced the 6-axis sample pattern with a stable 13-tap tangent/bitangent cubemap PCF kernel.
- Updated `36_shadow_map_parity_lab` point shadow defaults:
  - mapSize: 2048
  - bias: 0.018
  - radius: 3.0

## Test

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 xmake run 36_shadow_map_parity_lab
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 THREECPP_DEBUG_PBR_POINT_SHADOW=1 xmake run 36_shadow_map_parity_lab
```

The debug view should now be wider and more stable, not a thin flickering line.
