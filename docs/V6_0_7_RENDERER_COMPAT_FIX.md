# v6.0.7 Renderer API compatibility fix

Fixes older examples that still use:

```cpp
renderer.initialize(window);
renderer.setClearColor(glm::vec3{...}, 1.0f);
```

Changes:

- Added `GLRenderer::initialize(Window&)` compatibility shim. The native OpenGL context is still owned by `Window`; this function synchronizes renderer size from framebuffer size.
- Added `GLRenderer::setClearColor(const glm::vec3&, float alpha)` overload.
- Kept existing `setClearColor(const glm::vec4&)` API.
