# v6.0.56 Transmission parity tuning + FBX texture resolution

## Transmission

This version keeps the three.js strategy for glTF `KHR_materials_transmission`:

- transmissive surfaces remain opaque refractive surfaces (`transparent=false`, `opacity=1`)
- the renderer captures the opaque background into a transmission target
- the material samples the captured background using screen-space refraction
- Fresnel reflection and volume attenuation shape the final color

The previous build over-applied material thickness and a diffuse/baseColor floor, which made the dragon read as milky opaque plastic. v6.0.56 softens the single-pass optical thickness approximation and removes the diffuse floor for transmissive materials.

## FBX textures

FBX files often store texture paths as absolute Windows/macOS paths or paths relative to a texture subfolder. The loader now tries:

1. the raw path
2. path relative to the model directory
3. basename in the model directory
4. `textures/`, `Textures/`, `texture/`, `Texture/`
5. recursive basename search under the model directory, capped for safety

Texture diagnostics now keep a message when a texture is missing or decoded.
