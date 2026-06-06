# v1.1.1 Compile Fix

This patch fixes the `examples/10_pmrem_lut` API mismatch reported on macOS:

- `Window window({ ... })` was replaced with the valid constructor call `Window window(width, height, title)`.
- Added `Window::time()` backed by `glfwGetTime()`.
- Added `Object3D::lookAt()`, inherited by `PerspectiveCamera`.

Recommended build:

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 10_pmrem_lut
```
