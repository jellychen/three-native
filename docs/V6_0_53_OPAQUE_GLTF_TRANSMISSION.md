# v6.0.53 Opaque glTF Transmission

Fixes GLB transmission assets such as DragonDispersion.glb.

- glTF KHR_materials_transmission/volume now stays `transparent=false`, `opacity=1`, `depthWrite=true`.
- Transmissive materials still enter the transmissive render queue because RenderList detects MeshPhysicalMaterial::transmission/thickness.
- Shader outputs opaque refractive glass color instead of alpha-blended ghost transparency.
- Adds stronger screen-space lensing and simple chromatic dispersion offsets from KHR_materials_dispersion.

This is still a screen-space approximation, not a full backside-thickness pass, but it should read much closer to three.js MeshPhysicalMaterial transmission.
