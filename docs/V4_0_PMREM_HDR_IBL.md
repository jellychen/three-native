# v4.0 PMREM / HDR IBL

This version advances the renderer's environment lighting path toward the three.js `PMREMGenerator` model.

## Added / strengthened

- RGBE `.hdr` loader path through `TextureLoader::loadRGBE`.
- PFM test loader path through `TextureLoader::loadPFMAsRGB16FPlaceholder`.
- `PMREMGenerator::fromEquirectangular()` produces an `Environment` with:
  - `equirectangularMap`
  - irradiance cube map
  - prefiltered cube map mip chain
  - BRDF LUT texture
  - roughness-to-mip mapping
- PBR shader path uses:
  - `irradianceMap` for diffuse IBL
  - `prefilteredEnvMap` with `textureLod` for specular IBL
  - `brdfLUT` for split-sum Fresnel compensation
- Environment rotation support:
  - `Environment::environmentRotation`
  - `Environment::backgroundRotation`
  - legacy `Environment::rotation` retained
- New example:
  - `examples/33_pmrem_hdr_ibl`

## Run

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 33_pmrem_hdr_ibl

# Optional external HDR/RGBE file:
xmake run 33_pmrem_hdr_ibl /path/to/studio.hdr
```

## Notes

The PMREM generation is currently CPU-side and dependency-free. It creates valid cubemap mip storage and exercises the same renderer/shader binding path required for a future GPU PMREM implementation. It is not yet a bit-exact clone of three.js cubeUV PMREM filtering.

## Remaining work toward three.js parity

- GPU equirectangular-to-cubemap render pass.
- GPU GGX prefilter with importance sampling and cubeUV layout.
- Higher precision float texture upload path.
- Separate background full-screen equirectangular pass.
- `backgroundBlurriness`, `backgroundRotation`, and `environmentRotation` parity at renderer level.
