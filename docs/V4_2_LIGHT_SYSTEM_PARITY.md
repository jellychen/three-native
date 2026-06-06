# v4.2 Light System Parity

This version focuses on bringing the realtime light system closer to the three.js WebGLRenderer path.

## Added / refined

- `RendererParameters::physicallyCorrectLights`
- `RectAreaLight` public API, using a forward-renderer finite-area approximation
- `LightProbe` placeholder API that preserves imported probe intent
- Directional / Point / Spot light packing kept compatible with the existing `GpuLight` uniform array
- Point and Spot attenuation now respect `physicallyCorrectLights`; when disabled, legacy constant attenuation can be used for debugging
- Spot cone / penumbra path remains compatible with three.js `angle` and `penumbra` semantics
- Rect area light path uses area and facing approximation in shader; full LTC is staged for a later shader pass
- New dynamic multi-light validation scene

## New example

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 35_light_system_parity_lab
```

The scene includes:

- AmbientLight
- HemisphereLight
- moving DirectionalLight + shadow
- moving PointLight with distance/decay falloff
- moving SpotLight + shadow
- RectAreaLight approximation
- PBR material grid for visual comparison

## Still not 100% three.js

The following are still planned:

- true RectAreaLight LTC tables
- PointLight cubemap shadows
- LightProbe full spherical harmonics evaluation
- physically correct unit conversion matching recent three.js behavior in more edge cases
- light helpers for all light types
