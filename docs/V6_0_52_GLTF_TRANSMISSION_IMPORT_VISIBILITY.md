# v6.0.52 glTF Transmission Import Visibility

This patch fixes GLB/glTF physical transmission materials that imported with
`transmission=1` but rendered visually opaque.

Changes:

- Imported glTF transmission/volume materials are promoted to transmissive render state:
  - `transparent = true`
  - `depthWrite = false`
  - conservative opacity cap for fallback blending.
- Physical transmission shader no longer keeps alpha at 1.0 via `max(alpha, ...)`.
- Glass alpha now decreases with transmission and increases with optical thickness/Fresnel.

This keeps `KHR_materials_transmission + KHR_materials_volume` assets such as
`DragonDispersion.glb` visibly glass-like while preserving the existing screen-space
transmission capture path.

Known limitation: this is still an approximation; a true three.js-level result needs
backside thickness/depth pass and more accurate volume integration.
