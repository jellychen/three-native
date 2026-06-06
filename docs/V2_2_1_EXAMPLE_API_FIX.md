# v2.2.1 Example API Fix

Fixes compile errors reported by the macOS/xmake examples:

- Added `Window::poll()` as a compatibility alias for `Window::pollEvents()`.
- Added `Object3D::rotation` as an Euler XYZ vector in radians, matching the style used by the examples and three.js-like code.
- `Object3D::updateMatrix()` now composes `quaternion * glm::quat(rotation)` so quaternion animation/lookAt and simple Euler example rotation both work.

This is a compatibility step. A future version should replace the simple Euler field with a full `Euler` type that tracks order and synchronizes quaternion/rotation like three.js.
