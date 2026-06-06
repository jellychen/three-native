# v6.0.37 Directional / Spot shadow visibility labs

This version adjusts the standalone DirectionalLight and SpotLight shadow labs so the scene remains visible while shadow compare math is being tuned.

Changes:
- Directional and Spot PBR shadow debug output now uses a small visual floor instead of pure black.
- Directional and Spot direct-light shadow application keeps a small minimum light contribution for debug visibility.
- Standalone Directional / Spot labs use a slightly stronger IBL fill while still keeping only one shadow-casting light.

Run:

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 xmake run 54_directional_shadow_only_lab
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 xmake run 55_spot_shadow_only_lab
```

Debug:

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 THREECPP_DEBUG_PBR_DIRECTIONAL_SHADOW=1 xmake run 54_directional_shadow_only_lab
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 THREECPP_DEBUG_PBR_SPOT_SHADOW=1 xmake run 55_spot_shadow_only_lab
```
