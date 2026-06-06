# v6.0.40 PBR PMREM specular IBL restore

This version restores PMREM/equirectangular specular IBL in the experimental PBR path after the v6.0 visibility stabilization work.

## Main goals

- Keep `THREECPP_ENABLE_EXPERIMENTAL_PBR=1` visible by default.
- Restore PMREM diffuse/specular environment contribution with safety clamps.
- Add debug modes for diffuse IBL, specular IBL, and roughness-to-mip visualization.
- Keep independent kill switches for IBL and PMREM when diagnosing driver/sampler issues.

## Runtime switches

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1
THREECPP_DISABLE_PBR_IBL=1
THREECPP_DISABLE_PBR_PMREM=1
THREECPP_ENABLE_PBR_PMREM=0
THREECPP_DEBUG_PBR_IBL_DIFFUSE=1
THREECPP_DEBUG_PBR_IBL_SPECULAR=1
THREECPP_DEBUG_PBR_PMREM_LOD=1
THREECPP_PBR_PMREM_SPECULAR_STRENGTH=1.0
```

## Test

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 xmake run 56_pbr_pmrem_specular_restore
```

Optional HDR/PFM input:

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 xmake run 56_pbr_pmrem_specular_restore /path/to/studio.hdr
```

If the scene disappears, compare:

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_DISABLE_PBR_PMREM=1 xmake run 56_pbr_pmrem_specular_restore
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_DISABLE_PBR_IBL=1 xmake run 56_pbr_pmrem_specular_restore
```
