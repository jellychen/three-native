# v6.0.15 Render Diagnostics / Empty Frame Fallback

This build adds a renderer-side diagnostic path for the “background clears but no objects appear” issue.

## What changed

- Added `RendererParameters::frustumCulling`.
- Added environment variable `THREECPP_DEBUG_RENDER=1`.
- Added environment variable `THREECPP_DISABLE_FRUSTUM_CULLING=1`.
- If the render list is empty while visible renderables exist, the renderer rebuilds the list once with frustum culling disabled for that frame.
- Debug logs now print visible renderable count, queue sizes, viewport/framebuffer size, draw calls, triangles, lines, points, and program count.

## Debug usage

```bash
THREECPP_DEBUG_RENDER=1 xmake run 14_cache_dashed_texture_transform
THREECPP_DEBUG_RENDER=1 xmake run 15_skinning_animation_test
THREECPP_DEBUG_RENDER=1 xmake run 16_pbr_material_gallery
```

If the log shows renderables but zero queued items, try:

```bash
THREECPP_DEBUG_RENDER=1 THREECPP_DISABLE_FRUSTUM_CULLING=1 xmake run 14_cache_dashed_texture_transform
```

This helps distinguish:

- frustum culling/bounding sphere issue
- render list issue
- shader/program issue
- draw call/VAO issue
- viewport/framebuffer issue

