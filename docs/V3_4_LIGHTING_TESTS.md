# v3.4 Lighting stabilization and tests

This iteration focuses on the three real-time light classes that matter most for the renderer:

- `DirectionalLight`
- `PointLight`
- `SpotLight`

## Renderer fixes

### DirectionalLight

- Normalizes degenerate target vectors safely.
- Keeps `directionCone.xyz` as the light travel direction.
- Shader uses `-directionCone.xyz` as the surface-to-light vector.
- Directional shadow projection now uses `LightShadow::cameraLeft/right/top/bottom/near/far` instead of fixed constants.

### PointLight

- Uses three.js-style cutoff behavior: `distance == 0` means infinite range.
- Uses inverse-power falloff controlled by `decay`.
- Uses smooth fourth-power range cutoff when `distance > 0`.
- PointLight direct lighting is covered by the new tests.

PointLight cubemap shadows are still a later milestone. The current shadow pass covers DirectionalLight and SpotLight.

### SpotLight

- Treats `angle` as the outer cone half-angle.
- Computes inner cone as `cos(angle * (1 - penumbra))`.
- Uses hard-edge behavior when `penumbra == 0` and smoothstep when penumbra is non-zero.
- Spot shadow projection now uses shadow near/far parameters and the spot cone angle.

### Shadow sampling

- PCF now uses the actual per-shadow-map size instead of the previous hardcoded 1024 texel size.
- `shadowMapSize[4]` is uploaded with the shadow matrices, bias and radius.

## New examples

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 22_multi_light_shadow_test
xmake run 23_moving_lights_test
xmake run 24_light_falloff_spot_test
```

### 22_multi_light_shadow_test

Static multi-light validation scene:

- AmbientLight
- HemisphereLight
- DirectionalLight with shadow
- SpotLight with shadow
- PointLight direct contribution
- PBR material grid
- PCF shadow stability check

### 23_moving_lights_test

Animated light validation scene:

- Moving red/blue PointLights
- Moving SpotLight with shadow
- Moving DirectionalLight shadow direction
- Rotating metallic torus
- Camera orbit

### 24_light_falloff_spot_test

Falloff and spot-cone focused test:

- PointLight decay and cutoff row
- SpotLight cone/penumbra animation
- Spot shadow projected on obstacle row
- Marker meshes for light positions

## Remaining work

- PointLight cubemap shadow maps.
- PCFSoft/VSM variants.
- Better shadow normal bias in shader or depth pass.
- Shadow frustum helper visualization.
- Light helpers matching three.js helpers.
