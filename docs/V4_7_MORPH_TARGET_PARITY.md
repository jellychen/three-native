# v4.7 Morph Target / Blend Shape Parity

This iteration focuses on making morph target behavior closer to three.js while keeping the OpenGL renderer maintainable.

## Added

- `src/morph/MorphTargetUtils.hpp`
  - `targetCount()`
  - `normalizedInfluences()`
  - `topInfluenceIndices()`
  - `evaluateVec3()`
  - `bakeActiveMorphsToBase()` CPU fallback helper
- `BufferGeometry` morph metadata:
  - `morphTargetsNeedUpdate`
  - `morphTextureFallbackPreferred`
  - `morphTextureStride`
  - `hasMorphAttribute()`
- `Mesh` morph metadata:
  - `morphTargetCount()`
  - `morphTargetsNeedUpdate`
  - `morphTargetsUseCpuFallback`
- New test:
  - `examples/40_morph_target_parity_lab`

## Behavior

The renderer still uses attribute-based GPU morphing for the first four position/normal targets. This matches the existing shader path and keeps compatibility with current examples.

For larger facial rigs, v4.7 adds a CPU evaluation/bake fallback utility. This is not as efficient as three.js's full morph texture path, but it provides a deterministic path for validating more than four active morph targets and for future shadow/depth pass integration.

## Test

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 40_morph_target_parity_lab
```

The test shows:

- Left: live GPU morphing using the first four morph targets.
- Right: CPU-baked mesh using six active morph targets.
- Morph target dictionary names similar to blend shape names: Smile, BlinkL, BlinkR, Puff, Twist, Jaw.
- PBR material, IBL, directional shadow, and helper overlay.

## Remaining work

- True GPU morph texture fallback for many targets.
- Morph color shader path.
- Morph tangent shader path.
- Full morph + skinning + shadow integration for all render passes.
- Real RobotExpressive.glb validation.
