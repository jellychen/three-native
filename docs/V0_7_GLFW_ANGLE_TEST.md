# v0.7 GLFW + ANGLE renderer test

This version adds a concrete desktop test harness around the renderer:

- GLFW creates the native window with `GLFW_NO_API` when ANGLE is enabled.
- `AngleContext` creates an EGL display/context/surface and binds OpenGL ES 3.
- `08_glfw_angle_renderer_test` runs the renderer in a real window.
- The test scene exercises MeshBasicMaterial, MeshStandardMaterial, MeshPhysicalMaterial transmission, LineSegments, Points and FatLine.
- The CMake build can disable Assimp so the window/renderer test can build before the model-loader dependency is installed.

## Build with CMake on Windows

Example layout:

```text
C:/deps/angle
  include/EGL/...
  include/GLES3/...
  lib/EGL.lib
  lib/GLESv2.lib
  bin/libEGL.dll
  bin/libGLESv2.dll
```

Configure:

```bash
cmake -S . -B build ^
  -DTHREECPP_USE_ANGLE=ON ^
  -DTHREECPP_ANGLE_DIR=C:/deps/angle ^
  -DTHREECPP_ANGLE_BIN_DIR=C:/deps/angle/bin ^
  -DTHREECPP_ENABLE_ASSIMP=OFF
cmake --build build --config Debug
```

Run:

```bash
build/Debug/08_glfw_angle_renderer_test.exe
```

Press `ESC` to exit.

## Build with CMake on Linux

ANGLE must expose EGL and GLESv2 libraries. If your ANGLE install is not in a standard path, pass `THREECPP_ANGLE_DIR` and set `LD_LIBRARY_PATH` to the runtime library directory.

```bash
cmake -S . -B build \
  -DTHREECPP_USE_ANGLE=ON \
  -DTHREECPP_ANGLE_DIR=$HOME/deps/angle \
  -DTHREECPP_ENABLE_ASSIMP=OFF
cmake --build build -j
LD_LIBRARY_PATH=$HOME/deps/angle/lib ./build/08_glfw_angle_renderer_test
```

## Build with xmake

```bash
xmake f -m debug --use_angle=true --angle_dir=C:/deps/angle --enable_assimp=false
xmake
xmake run 08_glfw_angle_renderer_test
```

## Notes

The current execution environment used to generate this package does not have GLFW, ANGLE, Assimp, or an interactive display server installed, so the test harness was added and packaged but could not be fully executed here. The source is structured so a local machine with ANGLE and GLFW can build the test first with Assimp disabled, then enable Assimp later for model loading.
