# v4.8 Line / Points / FatLine / Helpers parity

This pass focuses on the real-time editor/helper rendering side of the three.js parity work.

## Added / improved

- `Line2` and `LineSegments2` compatibility aliases backed by the existing FatLine triangle path.
- `FatLineMaterial` compatibility fields: `worldUnits`, `trimNearPlane`, `dashed`, `dashScale`, `dashSize`, `gapSize`, `alphaToCoverage`.
- `PointsMaterial::scale` to tune size attenuation similarly to three.js examples.
- `SpriteMaterial::center` and `alphaMap` metadata.
- Renderable `Sprite` quad object with `SpriteMaterial` default.
- New helpers:
  - `CameraHelper`
  - `DirectionalLightHelper`
  - `PointLightHelper`
  - `SpotLightHelper`
  - `VertexNormalsHelper`
  - `VertexTangentsHelper`
- New validation scene: `examples/41_line_points_helpers_parity_lab`.

## Notes

The FatLine path is still not bit-exact with three.js `Line2`: near-plane trimming, alpha-to-coverage, and true `worldUnits` width still need deeper shader work. The public C++ API now matches the expected concepts so the renderer can continue converging without changing user-facing code.

## Run

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 41_line_points_helpers_parity_lab
```
