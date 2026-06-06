# v1.3 ShadowMap / PCF gap closure

This version removes the ray tracing direction and continues the realtime three.js-style renderer path.

## Implemented in this iteration

- `GLShadowMap` is now a real renderer subsystem instead of a scheduling-only placeholder.
- Directional and spot lights can render depth maps.
- Per-light depth framebuffer and depth texture allocation.
- Shadow item cache keyed by light object id.
- Shadow matrix generation using `biasMatrix * lightViewProjection`.
- Forward material shaders can consume shadow maps.
- Basic 3x3 PCF sampling in shader.
- `GpuLight::shadowIndex` connects light contribution with its shadow map.
- Shadow pass supports `Mesh::castShadow` and skips non-mesh primitives by default.
- Receivers are shaded through Lambert / Phong / Standard / Physical material paths.
- New example: `examples/12_shadow_map`.

## Current limitations versus three.js

- Directional and spot shadows only. Point-light cubemap shadows are not implemented yet.
- Shadow camera settings are still fixed. They should be exposed like `DirectionalLightShadow.camera` in three.js.
- Alpha-tested shadow casters are not texture-clipped yet.
- VSM / PCFSoft / normalBias are still approximations.
- Cascaded shadow maps are not implemented.
- Transparent and transmission-specific shadow behavior is not matched to three.js yet.
- Skinned depth pass plumbing exists, but full animated skinning import still needs more work.

## Test

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 12_shadow_map
```

