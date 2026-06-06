# v6.0 Regression Stability Runner

This version turns the previous regression example into a real stability runner for true model testing. It is designed to catch practical renderer/importer regressions while comparing behavior against the subset of three.js WebGLRenderer that this project targets.

## New capabilities

- Manifest-driven batch testing for `.glb`, `.gltf`, `.fbx`, and `.obj`.
- Per-model PASS / PARTIAL / FAIL classification.
- JSON report output.
- Markdown report output.
- Optional PPM screenshots through `glReadPixels`.
- Import compatibility diagnostics:
  - missing external textures
  - suspicious texture color spaces
  - KTX2 / BasisU compressed texture status
  - texture transform counts
  - alpha / transparent / physical material counts
- Runtime renderer statistics:
  - draw calls
  - instanced calls
  - instances
  - triangle / line / point counts
  - program count
  - first-frame and average frame timing
- Scene signature output for cache/regression comparison.

## Build

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
```

## Run with explicit models

```bash
xmake run 53_regression_stability_runner \
  --frames=60 \
  --size=1280x720 \
  --screenshots \
  --out=regression_out \
  --report=regression_out/report.json \
  --markdown=regression_out/report.md \
  /path/to/DamagedHelmet.glb \
  /path/to/Fox.glb \
  /path/to/RobotExpressive.glb
```

## Run with manifest

```bash
xmake run 53_regression_stability_runner assets/regression/models.example.json
```

Relative paths inside a manifest are resolved relative to the manifest file.

## Headless-style import check

This skips window creation and rendering, but still loads/imports and inspects model structure.

```bash
xmake run 53_regression_stability_runner --no-render /path/to/model.glb
```

## Status policy

- `PASS`: loaded, rendered if requested, and no critical compatibility warning was detected.
- `PARTIAL`: loaded/rendered, but issues were detected, such as missing external textures, suspicious texture color spaces, incomplete compressed texture transcoding, or missing animation/bone data.
- `FAIL`: loading failed, root object was empty, or the render probe threw an exception.

## Recommended model set

- `DamagedHelmet.glb` for PBR / normal / metallic / roughness.
- `BoomBox.glb` for metallicRoughnessTexture and emissive tests.
- `Lantern.glb` for alpha / transparent / emissive tests.
- `Fox.glb` for skeleton animation.
- `CesiumMan.glb` for skinned mesh validation.
- `RobotExpressive.glb` for morph target + skeleton animation.
- `Sponza.gltf` for large-scene, multi-material and multi-texture pressure.
- Mixamo FBX for FBX skeleton/unit/axis validation.
- OBJ + MTL for legacy material import validation.
