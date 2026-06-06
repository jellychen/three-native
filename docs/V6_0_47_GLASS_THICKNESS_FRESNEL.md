# v6.0.47 Glass Thickness / Fresnel / Rough Transmission

This version upgrades the `46_transparent_transmissive_queue` glass lab after v6.0.46 proved that the transmission capture path works but did not yet read like thick glass.

## Changes

- View-angle thickness approximation: grazing view rays travel farther through the volume.
- Stronger Beer-Lambert attenuation using optical thickness.
- Fresnel edge reflection mixed with PMREM/equirect/specular fallback.
- Rough transmission blur using mip LOD plus a stable 5-tap screen-space blur.
- Stronger normal-based refraction offset for curved surfaces.
- New debug modes:
  - `THREECPP_DEBUG_TRANSMISSION_THICKNESS=1`
  - `THREECPP_DEBUG_TRANSMISSION_FRESNEL=1`

## Run

```bash
xmake run 46_transparent_transmissive_queue
THREECPP_DEBUG_TRANSMISSION_TARGET=1 xmake run 46_transparent_transmissive_queue
THREECPP_DEBUG_TRANSMISSION_THICKNESS=1 xmake run 46_transparent_transmissive_queue
THREECPP_DEBUG_TRANSMISSION_FRESNEL=1 xmake run 46_transparent_transmissive_queue
```

## Expected result

- Higher thickness should absorb more color.
- Edges of spheres should show stronger reflection.
- Rough glass should show blurrier background.
- Colored attenuation should tint the transmitted background.
