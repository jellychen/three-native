# v3.4 Stability / Shader Fix

This package is the first stability pass after v3.3.

## Fixed

- Fixed `18_morph_targets_test` runtime shader compile failure on macOS OpenGL 3.3 core.
- The generic mesh vertex shader now declares morph target attributes and the `morphTargetInfluences[4]` uniform whenever `USE_MORPHTARGETS` / `USE_MORPHNORMALS` is enabled.
- Skinning now consumes the already morphed `transformed` vertex position instead of the original `position`, matching the expected morph-before-skinning order.

## Related error

```txt
Use of undeclared identifier 'morphTarget0'
Use of undeclared identifier 'morphTargetInfluences'
```

## Test

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 18_morph_targets_test
```

