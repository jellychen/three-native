# v6.0.30 PBR SpotLight Shadow Stability

This version restores SpotLight shadow sampling for the experimental PBR path using an isolated, single-sampler branch.

Environment flags:

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 xmake run 36_shadow_map_parity_lab
```

Directional and Spot shadows are enabled through safe standalone samplers. Legacy shadow-array and PointLight cubemap shadow paths remain gated off for experimental PBR.

Optional disable for Spot shadow only:

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 THREECPP_ENABLE_PBR_SPOT_SHADOWS=0 xmake run 36_shadow_map_parity_lab
```
