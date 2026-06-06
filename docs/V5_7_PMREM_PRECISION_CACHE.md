# v5.7 PMREM Precision and Cache

This version focuses on making IBL behavior closer to three.js PMREM-driven PBR while keeping the current OpenGL 3.3 / WebGL2-compatible architecture.

## Added

- PMREM cache keyed by source texture id/version and PMREM options.
- `PMREMCacheStats` with request/hit/miss/live-entry diagnostics.
- Non-linear roughness-to-mip mapping shared by CPU PMREM generation and GLSL sampling.
- Higher quality CPU PMREM prefilter path using Hammersley + GGX importance sampling.
- Improved BRDF LUT generation using split-sum integration instead of the previous analytic placeholder.
- Cube texture upload now sets `GL_TEXTURE_BASE_LEVEL` / `GL_TEXTURE_MAX_LEVEL` for PMREM mip correctness.
- Background fallback respects `backgroundBlurriness` by blending toward prefiltered/specular environment color.
- New `examples/50_pmrem_precision_lab` for roughness/metalness, clearcoat, transmission and PMREM cache testing.

## Run

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 50_pmrem_precision_lab
```

Optional HDR/PFM input:

```bash
xmake run 50_pmrem_precision_lab /path/to/studio.hdr
```

## What this improves

- Metal reflections become sharper at low roughness and blur more naturally at high roughness.
- Roughness-to-mip behavior is less linear and closer to perceptual PMREM lookup.
- Reusing the same environment no longer rebuilds PMREM repeatedly.
- Clearcoat and transmission tests now share the same improved PMREM chain.

## Still not bit-exact with three.js

- This is not the exact three.js cubeUV atlas layout.
- The PMREM convolution is still CPU-generated and encoded into byte textures for broad compatibility.
- HDR half-float cube targets and GPU PMREM convolution are future work.
- `backgroundBlurriness` is approximated in the fallback clear/background path.

## Next recommended step

`v5.8 shader chunk system` so Standard, Physical, Shadow, Skinning, Morph, PMREM and Transmission logic can be maintained like three.js shader chunks instead of one large shader string.
