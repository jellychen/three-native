# V6.0.9 Retina / framebuffer viewport fix

Fixes macOS Retina rendering only into the lower-left quarter of the window.

## Cause

On macOS, GLFW window logical size and framebuffer pixel size can differ. For example a 1280x720 window may have a 2560x1440 framebuffer. If `glViewport` is set to the logical window size, rendering only covers the lower-left quarter.

## Fix

`GLRenderer::render()` now calls `syncFramebufferSizeFromCurrentContext()` before rendering. The renderer queries `glfwGetCurrentContext()` and `glfwGetFramebufferSize()`, then calls `setSize(fbw, fbh)` when the framebuffer size changed.

This keeps `params.width/height`, viewport, transmission target, and camera aspect update paths consistent for examples that forgot to call `renderer.initialize(window)`.

## Opt-out

Set:

```cpp
RendererParameters params;
params.autoResizeToFramebuffer = false;
GLRenderer renderer(params);
```
