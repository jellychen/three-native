# v6.0.10 Retina resize stall fix

Fixes macOS Retina stalls caused by older examples calling `renderer.setSize(window.width(), window.height())` every frame.

On Retina, GLFW window size is logical points while framebuffer size is physical pixels. The previous behavior could alternate between logical size and framebuffer size every frame:

1. example calls `setSize(window.width(), window.height())`
2. `render()` auto-syncs to framebuffer size
3. transmission render target is reallocated repeatedly

`GLRenderer::setSize()` now detects this case and converts the logical window size to framebuffer size when `autoResizeToFramebuffer` is enabled. It also early-outs if the size is unchanged, avoiding redundant viewport updates and render target resizing.

This is especially important for examples using transmission materials, such as `11_material_geometry_matrix`.
