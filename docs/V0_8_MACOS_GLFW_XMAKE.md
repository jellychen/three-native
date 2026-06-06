# v0.8 macOS GLFW Native OpenGL Test

This version adds a no-ANGLE macOS test path. The default backend is now native OpenGL through GLFW, intended for quick renderer validation on macOS before re-enabling ANGLE.

## Backend

- GLFW creates a native OpenGL 3.3 Core profile context.
- ANGLE/EGL is disabled by default.
- Runtime shader generation now emits:
  - `#version 330 core` for native OpenGL
  - `#version 300 es` for ANGLE/OpenGL ES
- `GLFW_INCLUDE_NONE` is used to avoid GLFW pulling legacy GL headers before `OpenGL/gl3.h`.

## Build with xmake on macOS

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake
xmake run 08_macos_glfw_native_test
```

Release:

```bash
xmake f -m release --use_angle=false --enable_assimp=false
xmake
xmake run 08_macos_glfw_native_test
```

## Test Scene

`examples/08_macos_glfw_native_test` creates:

- GLFW window
- Perspective camera orbiting the scene
- `MeshBasicMaterial` cube
- `MeshStandardMaterial` PBR spheres
- `MeshPhysicalMaterial` transmission sphere
- Ambient, directional, and point lights
- `LineSegments` grid
- `Points` point cloud
- `FatLine` curve

Press `ESC` to exit.

## Notes

Assimp is disabled by default in this version so that the window/rendering test is not blocked by model-loading dependencies. Enable it explicitly when testing model import:

```bash
xmake f --enable_assimp=true
```

ANGLE can still be tested explicitly:

```bash
xmake f --use_angle=true --angle_dir=/path/to/angle
xmake run 08_glfw_angle_renderer_test
```
