# v6.0.36 Separate Shadow Labs

Adds isolated shadow tests so each light type can be validated without other light sources washing out the result.

## Examples

### PointLight cubemap shadow only

Existing point-light-only lab:

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 xmake run 36_shadow_map_parity_lab
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 THREECPP_DEBUG_PBR_POINT_SHADOW=1 xmake run 36_shadow_map_parity_lab
```

### DirectionalLight shadow only

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 xmake run 54_directional_shadow_only_lab
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 THREECPP_DEBUG_PBR_DIRECTIONAL_SHADOW=1 xmake run 54_directional_shadow_only_lab
```

### SpotLight shadow only

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 xmake run 55_spot_shadow_only_lab
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 THREECPP_DEBUG_PBR_SPOT_SHADOW=1 xmake run 55_spot_shadow_only_lab
```

## Notes

Each scene keeps environment intensity very low and removes all other lights. This makes the active shadow source easy to inspect.

- `36_shadow_map_parity_lab`: PointLight only.
- `54_directional_shadow_only_lab`: DirectionalLight only.
- `55_spot_shadow_only_lab`: SpotLight only.

Directional shadow debug was added in this version through `THREECPP_DEBUG_PBR_DIRECTIONAL_SHADOW=1`.
