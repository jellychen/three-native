# v6.0.54 GLB Transmission Parity Tuning

This build improves `KHR_materials_transmission + KHR_materials_volume` rendering for GLB assets such as `DragonDispersion.glb`.

The prior build imported the physical material correctly, but strict single-pass Beer-Lambert attenuation made high-thickness / short-distance volume glass look opaque. This version keeps glTF transmission surfaces opaque in render-state terms, but softens screen-space transmittance and strengthens IOR/Fresnel environment reflection so the material reads as refractive glass rather than alpha-blended ghosting or opaque plastic.

Key changes:

- preserves `transparent=false`, `depthWrite=true` for glTF transmission surfaces
- keeps transmissive queue classification from `transmission/thickness`
- softens the screen-space transmittance term until a true backside thickness pass exists
- uses IOR-derived Fresnel reflection across the surface
- adds a small volume-scatter/body tint term from `attenuationColor`
- keeps alpha at 1.0 to match three.js transmission semantics better than alpha blending

Suggested test:

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
xmake run 38_asset_import_parity_lab /path/to/DragonDispersion.glb
```

Debug:

```bash
THREECPP_DEBUG_TRANSMISSION_TARGET=1 xmake run 38_asset_import_parity_lab /path/to/DragonDispersion.glb
THREECPP_DEBUG_TRANSMISSION_THICKNESS=1 xmake run 38_asset_import_parity_lab /path/to/DragonDispersion.glb
THREECPP_DEBUG_TRANSMISSION_FRESNEL=1 xmake run 38_asset_import_parity_lab /path/to/DragonDispersion.glb
```

Remaining gap vs three.js:

- no backside thickness/depth target yet
- no true screen-space ray marching
- rough transmission blur is approximate
- dispersion is approximate RGB UV offset
