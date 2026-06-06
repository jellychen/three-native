# v3.9 Renderer Architecture Closure

This version focuses on narrowing the gap between the previous prototype renderer and a three.js `WebGLRenderer`-style runtime architecture.

## Added / changed

### Render lists and queues

- `RenderList` now owns three buckets:
  - `opaque`
  - `transmissive`
  - `transparent`
- Items carry `renderOrder`, `objectId`, `geometryId`, `materialId`, `materialVersion`, `bucket`, and a stable `sortKey`.
- Opaque items use front-to-back friendly sorting by renderOrder, state key and depth.
- Transparent/transmissive items use back-to-front ordering.

### Render state stack

- Added `RenderState` and `RenderStateStack`.
- The renderer now tracks per-frame scene, camera, viewport, scissor and render-list statistics.

### Program cache

- Added `ProgramCache` wrapper around `ProgramKey -> GLProgram`.
- Shadow depth programs and forward programs now use the same cache entry point.
- Renderer info reports live program count from the cache.

### Binding state cache

- Added `GLBindingStates` to avoid redundant VAO binding.

### Uniform cache foundation

- Added `UniformCache` as a lightweight foundation for future reduction of redundant scalar/vector uniform uploads.
- Matrix and array uniforms are still uploaded directly for correctness.

### Viewport / scissor API

`GLRenderer` now exposes:

```cpp
renderer.setViewport(x, y, w, h);
renderer.setScissor(x, y, w, h);
renderer.setScissorTest(enabled);
```

### Layers

- `Object3D::layers` already existed.
- v3.9 render projection now checks `(object.layers & camera.layers) != 0` before submitting a render item.

### Material and object versioning

- `Material::version` and `Material::markNeedsUpdate()` were added.
- This prepares material/program invalidation similar to three.js.

### New example

```bash
xmake run 32_renderer_architecture_lab
```

The test creates opaque PBR objects, a transmissive physical sphere and a transparent overlay to exercise queue classification, sorting, cache reuse and renderer statistics.

## Still not complete

This is not the final three.js renderer architecture yet. Still missing or incomplete:

- Full uniform cache integration for matrices/arrays/textures.
- Complete WebGLRenderStates feature parity.
- Clipping planes.
- MRT render targets.
- Transmissive background exclusion for transmissive objects.
- Material-program invalidation beyond the current `ProgramKey`.
- Render target stack for nested render-to-texture.

## Next planned version

`v4.0 PMREM / HDR IBL` should build on this renderer structure.
