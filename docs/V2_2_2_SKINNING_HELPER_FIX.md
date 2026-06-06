# v2.2.2 Skinning Helper Compile Fix

Fixes the skinning animation example compile errors reported on macOS/Xcode:

- `SkeletonHelper` now has a constructor accepting `Object3D*`/`SkinnedMesh*` usage through implicit base pointer conversion.
- `SkeletonHelper::update()` is added as the three.js-style refresh API.
- `SkeletonHelper::setRoot(Object3D*)` is added for rebinding helpers after construction.
- `SkeletonHelper::updateFromRoot()` now forces a matrix-world update before rebuilding helper line geometry.

Run:

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 15_skinning_animation_test
xmake run 16_pbr_material_gallery
```
