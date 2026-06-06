# v3.7 glTF / GLB Material and Animation Validation

This iteration focuses on convergence rather than adding unrelated features. It adds a dedicated validation viewer for real assets and keeps the renderer on the realtime three.js-like path.

## New example

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
xmake run 30_gltf_material_animation_validation /path/to/model.glb
```

The example loads `glb`, `gltf`, `fbx`, or `obj` through `AssimpLoader`, then reports object, mesh, material, texture, skeleton, bone, morph target, animation, and track counts through `GltfValidationSuite`.

## Runtime controls

- Mouse left drag: orbit
- Mouse middle/right drag: pan
- Mouse wheel: dolly
- `SPACE`: pause/resume animation
- `1`, `2`, `3`: switch among the first three animation clips
- `ESC`: close

## Scene setup

The validation viewer enables:

- ACES tone mapping
- PMREM environment placeholder path
- Ambient + Hemisphere + Directional + Point light
- Directional shadow setup
- Grid and axis helpers
- Imported mesh `castShadow` and `receiveShadow`
- Skinning, morph target, embedded texture, tangent generation and animation import options

## Why this matters

The next bottleneck is no longer class coverage. It is real asset behavior. This example is intended for assets such as:

- `DamagedHelmet.glb`
- `BoomBox.glb`
- `Lantern.glb`
- `Fox.glb`
- `CesiumMan.glb`
- `RobotExpressive.glb`
- Mixamo FBX files

Each real model should expose concrete importer, material, animation, shadow, morph, and texture bugs that can be fixed against observable behavior.

## Known remaining gaps

- PMREM is still not fully equivalent to three.js `PMREMGenerator`.
- `MeshStandardMaterial` still needs stricter color-space, tangent-space, metallic-roughness channel, and alpha-mode handling.
- `MeshPhysicalMaterial` still needs a full transmission background capture path.
- Animation clip blending/action semantics are still simplified.
- Point light cubemap shadows are not yet implemented.
