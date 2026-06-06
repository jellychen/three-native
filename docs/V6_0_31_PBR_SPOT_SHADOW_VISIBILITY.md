# V6.0.31 PBR Spot Shadow Visibility

This version keeps the isolated PBR SpotLight shadow path from v6.0.30, but makes it testable and visible.

Changes:

- Adds `LightShadow::intensity`.
- Carries shadow intensity into `ShadowRenderItem`.
- Adds `pbrSpotShadowStrength` uniform.
- Adds `THREECPP_DEBUG_PBR_SPOT_SHADOW=1` to render the Spot shadow factor as grayscale.
- Adjusts `36_shadow_map_parity_lab` so the Spot shadow is not hidden by strong Directional/Ambient lighting.

Test:

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 xmake run 36_shadow_map_parity_lab
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 THREECPP_DEBUG_PBR_SPOT_SHADOW=1 xmake run 36_shadow_map_parity_lab
```

If the debug mode shows a grayscale projected shape, the Spot shadow map and matrix are working; the remaining difference is visual weighting.
