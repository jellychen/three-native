# v5.3 Transparent / Transmissive Queue Stabilization

This version tightens the three.js-style render queue behavior around alpha-tested, transparent, and transmissive materials.

## Renderer changes

- `RendererParameters::sortObjects`
  - Mirrors `WebGLRenderer.sortObjects`.
  - When enabled, opaque objects sort front-to-back by program/material/geometry and z.
  - Transparent and transmissive objects sort back-to-front with `renderOrder` priority.

- `RendererParameters::transmissionExcludesTransparent`
  - Default: `true`.
  - The transmission background capture now renders only opaque items by default.
  - Transmissive materials are excluded from their own refraction source.
  - Transparent blend objects are excluded from the source unless explicitly requested.

- `RenderItem` queue metadata
  - `receivesTransmissionBackground`
  - `writesDepth`
  - `alphaTested`

- Queue classification
  - `MeshPhysicalMaterial` with `transmission`, `transmissionMap`, `thickness`, or `thicknessMap` enters the transmissive queue.
  - `transparent=true`, `opacity<1`, or non-none blending enters the transparent queue.
  - `alphaTest>0` alone stays opaque, matching the three.js depth-writing alpha-test path.

- Transmission target clear color
  - The offscreen transmission capture now uses the resolved scene background color, not the renderer default clear color.

## Added example

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 46_transparent_transmissive_queue
```

The scene contains:

- opaque PBR row,
- alpha-tested plane row,
- transparent blended plane row,
- transmissive physical glass row,
- directional shadow,
- PMREM environment fallback.

## Remaining work

- Better order-independent transparency is not implemented.
- Transmission blur still uses the current mip-based render-target approximation.
- Transparent shadow support remains limited.
- Future work should integrate this queue model with glTF alphaMode regression tests.
