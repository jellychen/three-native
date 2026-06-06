# v6.0.46 Transmission / Thickness Glass Lab

This version turns `examples/46_transparent_transmissive_queue` into a dedicated glass/transmission validation scene.

## Goals

- Verify that transmissive `MeshPhysicalMaterial` objects sample an opaque-only transmission render target.
- Make transmission visible using a high-contrast striped background and opaque reference balls behind the glass.
- Test transmission amount, thickness/attenuation and roughness-driven blur independently.

## Run

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
xmake run 46_transparent_transmissive_queue
```

## Debug modes

```bash
THREECPP_DEBUG_TRANSMISSION_TARGET=1 xmake run 46_transparent_transmissive_queue
THREECPP_DEBUG_TRANSMISSION_UV=1 xmake run 46_transparent_transmissive_queue
THREECPP_DEBUG_TRANSMISSION_ATTENUATION=1 xmake run 46_transparent_transmissive_queue
THREECPP_DISABLE_TRANSMISSION_CAPTURE=1 xmake run 46_transparent_transmissive_queue
```

## Expected result

- Top row: transmission increases from opaque to glass-like.
- Middle row: thickness/attenuation increases color absorption.
- Bottom row: roughness increases background blur/softening.
- `THREECPP_DEBUG_TRANSMISSION_TARGET=1` shows the opaque-only captured background on the glass surfaces.
- `THREECPP_DISABLE_TRANSMISSION_CAPTURE=1` falls back to analytic transmission, making the difference obvious.

## Notes

This is still a safe screen-space approximation, not a bit-exact three.js transmission implementation. The path now validates the three critical stages: transmissive queue classification, opaque-only capture, and shader sampling of `transmissionSamplerMap`.
