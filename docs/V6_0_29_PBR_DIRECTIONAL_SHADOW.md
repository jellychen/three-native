# v6.0.29 PBR Directional Shadow Safe Restore

This build restores experimental PBR shadows through a minimal, isolated directional-light path.

## Why

The old `USE_SHADOWMAP` branch mixed directional, spot, and point/cubemap shadow samplers in one shader path. On macOS it made otherwise valid experimental PBR draws disappear. v6.0.29 does not use that legacy branch for experimental PBR.

## What changed

- Added `ProgramKey::usePBRDirectionalShadow`.
- Added shader define `USE_PBR_DIRECTIONAL_SHADOW`.
- Added isolated uniforms:
  - `pbrDirectionalShadowEnabled`
  - `pbrDirectionalShadowMap`
  - `pbrDirectionalShadowMatrix`
  - `pbrDirectionalShadowMapSize`
  - `pbrDirectionalShadowBias`
  - `pbrDirectionalShadowRadius`
- Experimental PBR with `THREECPP_ENABLE_PBR_SHADOWS=1` now samples only the first valid DirectionalLight shadow map.
- Spot and Point shadow sampling remain disabled in the experimental PBR path until they are restored safely.

## Test

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r

THREECPP_ENABLE_EXPERIMENTAL_PBR=1 xmake run 36_shadow_map_parity_lab
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 xmake run 36_shadow_map_parity_lab
```

Expected: both commands should display. The second should include the first directional shadow only.
