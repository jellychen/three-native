# v2.3.2 OrbitControls / Mouse Wheel Zoom

This version adds a small three.js-style `OrbitControls` implementation for the GLFW test viewer.

## Added

- `src/controls/OrbitControls.hpp`
- `src/controls/OrbitControls.cpp`
- `Window::cursorPosition()`
- `Window::mouseButtonPressed()`
- `Window::keyPressed()`
- `Window::consumeScrollDelta()`
- GLFW scroll callback wiring inside `Window`

## Controls

In `examples/17_assimp_multi_format_viewer`:

- Left mouse drag: orbit around the model target.
- Right or middle mouse drag: pan the target.
- Mouse wheel: zoom in / zoom out by changing camera distance.

## Run

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
xmake run 17_assimp_multi_format_viewer /path/to/model.glb
```

Supported importer extensions are still routed through Assimp:

- `.glb`
- `.gltf`
- `.fbx`
- `.obj`

## Notes

`OrbitControls` is intentionally kept outside `GLRenderer`, matching the three.js architecture where controls mutate the camera but are not part of renderer state.
