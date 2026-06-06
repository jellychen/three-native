# v5.0 Performance Cache / Large Scene

This version focuses on the runtime systems required to move the renderer from a feature prototype toward a large-scene renderer similar in structure to three.js WebGLRenderer.

## Added

- `src/performance/PerformanceCache.hpp`
  - `RendererCacheFrameStats`
  - `LargeSceneProfile`
  - `analyze_large_scene()`
  - `DrawRangeKey`
  - `RenderListCache`
  - `scene_cache_signature()`
- `InstancedMesh` metadata class in `Renderable.hpp`
  - `instanceMatrices`
  - `instanceColors`
  - `setMatrixAt()`
  - `setColorAt()`
- `examples/43_performance_cache_large_scene`
  - 1000+ mesh stress scene
  - shared geometry cache stress
  - many material variants
  - transparent/transmissive queue stress
  - moving lights
  - render statistics logging

## Scope

This version adds the data model and validation lab for the performance-cache layer. It does not yet replace every draw call with GPU instancing; the next optimization pass should wire `InstancedMesh` to `glDrawElementsInstanced` and add per-instance matrix attributes.

## Run

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 43_performance_cache_large_scene
```

## Next optimization tasks

1. Wire `InstancedMesh` to actual instanced rendering.
2. Add material-version-aware `ProgramKey` invalidation.
3. Add `TextureUnitAllocator` and sampler binding cache.
4. Add matrix-array uniform cache where safe.
5. Add `BufferGeometry.groups` and multi-material draw support.
6. Add `drawRange` to `BufferGeometry` and `RenderItem`.
7. Add persistent `RenderList` cache keyed by scene/camera/layers signatures.
8. Add transparent/transmissive sort debug views.
