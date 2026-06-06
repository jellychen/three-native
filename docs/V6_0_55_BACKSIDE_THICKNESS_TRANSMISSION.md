# v6.0.55 Backside Thickness / three.js-style Transmission Approximation

This version moves GLB transmission closer to the three.js `MeshPhysicalMaterial` renderer strategy.

## Key change

Transmission is still rendered as an opaque refractive surface, not as alpha blending. In addition to the opaque transmission background capture, the renderer now builds a backface view-depth target for transmissive objects:

1. Render opaque background into `transmissionTarget`.
2. Render transmissive object backfaces into `transmissionBackfaceTarget` as normalized view depth.
3. In the physical material shader, estimate screen-space object thickness:

```glsl
screenThickness = (backDepth - frontDepth) * cameraFar;
opticalThickness = screenThickness * material.thickness * thicknessMap;
```

This approximates the front/back distance used by three.js-style transmission volume shading and gives complex assets such as `DragonDispersion.glb` a much stronger volume response.

## Debug flags

```bash
THREECPP_DEBUG_TRANSMISSION_TARGET=1
THREECPP_DEBUG_TRANSMISSION_BACKFACE_THICKNESS=1
THREECPP_DEBUG_TRANSMISSION_THICKNESS=1
THREECPP_DEBUG_TRANSMISSION_FRESNEL=1
THREECPP_DISABLE_TRANSMISSION_BACKFACE=1
```

## Test

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
xmake run 38_asset_import_parity_lab /path/to/DragonDispersion.glb
```

To compare with the old material-thickness-only approximation:

```bash
THREECPP_DISABLE_TRANSMISSION_BACKFACE=1 xmake run 38_asset_import_parity_lab /path/to/DragonDispersion.glb
```
