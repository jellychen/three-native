# v0.6 Transmission Render Target

This iteration moves `MeshPhysicalMaterial::transmission` from a pure material-local approximation toward the renderer-level path used by three.js.

## Added

- `GLRenderTarget`
  - color texture
  - depth renderbuffer
  - resize support
  - optional mipmap generation
- `RendererParameters::transmission`
- `RendererParameters::transmissionResolutionScale`
- `RendererParameters::transmissionMipLevel`
- `GLRenderer::renderTransmissionBackground`
- `ProgramKey::useTransmissionRenderTarget`
- `USE_TRANSMISSION_RENDERTARGET` shader permutation
- `transmissionSamplerMap` shader uniform
- screen-space refracted framebuffer lookup for physical transmission
- new example: `07_transmission_render_target`

## Rendering flow

The frame is now staged as:

```txt
1. update scene/camera matrices
2. build render lists
3. render opaque objects to the default framebuffer
4. if transmissive transparent physical materials exist:
   4.1 render opaque objects again into transmission render target
   4.2 generate mipmaps for roughness-dependent transmission blur
5. render transparent objects, including MeshPhysicalMaterial transmission
```

This is closer to the three.js renderer design than v0.5, where transmission only blended a local ambient/env approximation.

## Shader behavior

When `USE_TRANSMISSION_RENDERTARGET` is enabled, the physical fragment shader:

1. computes view direction and approximate refracted direction
2. derives screen UV from `gl_FragCoord`
3. offsets the framebuffer sample by refracted XY distortion
4. samples `transmissionSamplerMap` with `textureLod`
5. applies Beer-Lambert attenuation using `attenuationColor`, `attenuationDistance`, and `thickness`
6. mixes the result by `transmission` and `transmissionMap`

## Current approximation

This version is intentionally still not a perfect clone of three.js. Remaining gaps:

- no backside thickness pass yet
- no per-object exclusion from the transmission target
- no sorted transmissive multi-layer refraction
- roughness blur uses mipmap level approximation
- no real PMREM environment map lookup yet
- no screen-space depth correction for refraction intersection

## Next steps

To continue toward three.js parity:

1. split transparent render list into transmissive and normal transparent buckets
2. exclude the current transmissive object from its own transmission background
3. add backside depth/thickness pass
4. add proper PMREM cubemap for IBL
5. implement `MeshPhysicalMaterial` transmission defines closer to three.js chunks
6. add renderer-level color-management flags matching three.js `outputColorSpace`
