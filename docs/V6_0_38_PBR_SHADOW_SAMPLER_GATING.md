# v6.0.38 PBR shadow sampler gating

Fixes the DirectionalLight-only and SpotLight-only shadow labs showing only the background while PointLight shadow worked.

Root cause: the experimental PBR program compiled Directional, Spot and Point shadow sampler branches together whenever `THREECPP_ENABLE_PBR_SHADOWS=1`. In single-light labs this left unused sampler uniforms active. On macOS OpenGL, sampler2D and samplerCube uniforms defaulting to the same texture unit can make draws produce no visible fragments.

Fix: `GLRenderer::getProgram()` now enables `USE_PBR_DIRECTIONAL_SHADOW`, `USE_PBR_SPOT_SHADOW`, and `USE_PBR_POINT_SHADOW` only when the current frame's `GLShadowMap` actually contains a matching shadow item.

Expected tests:

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 xmake run 54_directional_shadow_only_lab
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 xmake run 55_spot_shadow_only_lab
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 xmake run 36_shadow_map_parity_lab
```

Debug:

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 THREECPP_DEBUG_PBR_DIRECTIONAL_SHADOW=1 xmake run 54_directional_shadow_only_lab
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 THREECPP_DEBUG_PBR_SPOT_SHADOW=1 xmake run 55_spot_shadow_only_lab
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 THREECPP_DEBUG_PBR_POINT_SHADOW=1 xmake run 36_shadow_map_parity_lab
```
