# v2.1 Renderer cache, dashed line and texture transform evolution

This iteration keeps the project focused on three.js-like realtime rendering and removes no public realtime APIs.

## What changed

- `BufferGeometry::version` is now honored by `GLResourceManager`.
  - VAO/VBO/IBO resources are reused while the geometry version is unchanged.
  - If CPU-side geometry mutates through `setAttribute()`, `setIndex()`, `clear()` or `markNeedsUpdate()`, the GL resource is disposed and rebuilt.
- `Texture::version` and `Texture::markNeedsUpdate()` were added.
  - Texture pixel/sampler changes can now invalidate cached GL textures.
  - `TextureFactory` and `TextureLoader` mark generated/loaded textures as updated.
- `Texture::uvTransform()` was added.
  - `offset`, `repeat`, `center` and `rotation` are passed to shaders through a `uvTransform` uniform.
  - This starts aligning the texture transform path with three.js texture matrix behavior.
- `BufferGeometry::computeLineDistances()` was added.
  - Supports continuous line distance or per-segment distances.
  - Used by `LineDashedMaterial`.
- `LineDashedMaterial` now has shader-level dash support for regular `Line` / `LineSegments` and `FatLine`.
  - `dashScale`, `dashSize`, and `gapSize` are uploaded as uniforms.
- Added `examples/14_cache_dashed_texture_transform`.

## Why this matters

three.js relies heavily on resource versioning (`needsUpdate`, attribute versions, texture versions) to keep renderer caches correct. Before this iteration the C++ renderer had a geometry version field, but the GL backend did not actually use it. v2.1 closes that gap and makes dynamic helpers, dashed lines and texture transform tests safer.

## Run

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 14_cache_dashed_texture_transform
```

## Still incomplete versus three.js

- Texture transform is currently a single shared `uvTransform` per material draw, not one independent matrix per texture slot.
- Line joins/caps are still simple. FatLine still needs full three.js `Line2` parity.
- PMREM is still a placeholder/approximation, not the complete three.js GPU convolution path.
- ShadowMap has Directional/Spot foundation but still needs PointLight cubemap shadow, VSM and normalBias refinement.
