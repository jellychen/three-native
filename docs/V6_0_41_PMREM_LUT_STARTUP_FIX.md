# v6.0.41 PMREM LUT / Startup Fix

Fixes two PMREM/IBL issues found in `56_pbr_pmrem_specular_restore`:

1. BRDF LUT texture completeness on macOS OpenGL.
   - `TextureFactory::makeBRDFLUT()` sets `generateMipmaps=false`, but previously inherited `LinearMipmapLinear` as the min filter.
   - A non-mipmapped texture with a mipmapped min filter is incomplete/unloadable.
   - The BRDF LUT now uses `TextureFilter::Linear` for both min/mag filtering.

2. Slow startup.
   - The PMREM lab now defaults to fast interactive PMREM settings: cubeSize=64, irradianceSamples=32, prefilterSamples=32.
   - Override with environment variables:
     - `THREECPP_PMREM_CUBE_SIZE`
     - `THREECPP_PMREM_IRRADIANCE_SAMPLES`
     - `THREECPP_PMREM_PREFILTER_SAMPLES`

Run:

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 xmake run 56_pbr_pmrem_specular_restore
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_DEBUG_PBR_IBL_SPECULAR=1 xmake run 56_pbr_pmrem_specular_restore
```
