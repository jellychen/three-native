# threecpp v3.6 HDR / PMREM / IBL Pipeline

This pass moves the renderer from a procedural environment fallback toward a three.js-like PMREM flow.

## Added

- `TextureLoader::loadRGBE(path)` for Radiance `.hdr` / RGBE files.
- `PMREMGenerator::fromEquirectangular(texture)` now generates actual CPU-side cube data instead of a flat placeholder.
- Generated environment assets:
  - `Environment::irradianceMap`
  - `Environment::prefilterMap`
  - `Environment::brdfLUT`
  - `Environment::hasPMREM`
  - `Environment::pmremMipLevels`
- GL cubemap upload/caching through `GLResourceManager::getOrCreateCubeTexture()` and `bindCubeTexture()`.
- PBR shader path:
  - `USE_PMREM`
  - `samplerCube irradianceMap`
  - `samplerCube prefilteredEnvMap`
  - `sampler2D brdfLUT`
  - roughness-driven mip sampling.

## New examples

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 28_hdr_pmrem_pipeline /path/to/studio.hdr
xmake run 29_pmrem_roughness_lod_lab
```

`28_hdr_pmrem_pipeline` accepts an optional `.hdr` path. If no HDR is supplied or loading fails, it falls back to a procedural studio HDRI.

## Notes

This is still not a byte-for-byte copy of three.js `PMREMGenerator`. The current pass uses a dependency-free CPU approximation and stores filtered cube data in the current byte texture upload path. The API and renderer/shader wiring are now in place, so the next step can replace the CPU approximation with a GPU convolution pass and true half-float texture storage.
