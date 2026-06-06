# v6.0.39 Shadow Lab Helper + Startup Fix

## Fixes

- `SpotLightHelper` now tracks both `SpotLight::position` and `SpotLight::target`.
- `SpotLightHelper::update(const SpotLight&)` orients the helper cone with `Object3D::lookAt()` so local `-Z` points at the actual spotlight target.
- `55_spot_shadow_only_lab` updates the helper every frame after animating the spotlight target.
- Focused shadow labs no longer build PMREM synchronously at startup:
  - `36_shadow_map_parity_lab`
  - `54_directional_shadow_only_lab`
  - `55_spot_shadow_only_lab`
- These labs now use a small analytic `Environment` fill instead of CPU PMREM, avoiding a black window pause before the first frame.

## Tests

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 xmake run 36_shadow_map_parity_lab
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 xmake run 54_directional_shadow_only_lab
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 xmake run 55_spot_shadow_only_lab
```

Spot helper direction should now match the animated spotlight target.
