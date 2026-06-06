# v5.4 Regression Scene Runner

This version adds a fixed regression entry point for validating real assets after renderer changes. It is intended to catch regressions in glTF/GLB/FBX/OBJ loading, material mapping, animation, morph targets, shadows, transparency and renderer caches.

## New files

- `src/regression/RegressionSceneRunner.hpp`
- `examples/47_regression_scene_runner/main.cpp`

## Build

The runner depends on Assimp:

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
```

## Run with explicit model paths

```bash
xmake run 47_regression_scene_runner --frames=60 --size=1280x720 /path/to/DamagedHelmet.glb /path/to/Fox.glb
```

Headless/inspection-only mode:

```bash
xmake run 47_regression_scene_runner --no-render /path/to/model.glb
```

## Run with manifest

```json
{
  "models": [
    "DamagedHelmet.glb",
    "BoomBox.glb",
    "Lantern.glb",
    "Fox.glb",
    "RobotExpressive.glb"
  ],
  "frames": 60,
  "width": 1280,
  "height": 720,
  "render": true,
  "checkExternalFiles": true,
  "enableAnimation": true,
  "enableSkinning": true,
  "enableMorphTargets": true,
  "enableTextures": true,
  "enableShadows": true
}
```

```bash
xmake run 47_regression_scene_runner /path/to/regression.json
```

Relative model paths in the manifest are resolved relative to the manifest file.

## Reported data

For each model the runner prints:

- loader format
- load time
- inspect time
- first frame time
- average frame time
- draw calls / instanced calls / instances
- triangles / lines / points / program count
- object / mesh / material / texture counts
- skeleton / bone counts
- morph target counts
- animation / track counts
- import compatibility diagnostics
- texture color space and missing-file warnings
- scene cache signature

## Recommended fixed model set

- `DamagedHelmet.glb`: metallic/roughness PBR baseline
- `BoomBox.glb`: packed metallicRoughness texture
- `Lantern.glb`: transparency and emissive
- `Fox.glb`: skeletal animation
- `CesiumMan.glb`: skinning validation
- `RobotExpressive.glb`: skinning + morph target validation
- `Sponza.gltf`: large multi-material scene
- Mixamo FBX: FBX transform/skeleton validation
- OBJ + MTL: legacy material fallback validation

## Notes

This is not a screenshot-diff tool yet. It is a deterministic structural and runtime smoke test. A later version can add image capture and perceptual diff against reference renders.
