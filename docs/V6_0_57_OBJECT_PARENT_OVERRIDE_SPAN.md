# v6.0.57 Object3D weak parent + Scene overrideMaterial + span APIs

## Goal

This patch tightens three.js behavioral parity in three core places:

1. `Object3D::parent` is now non-owning weak state instead of an owning/raw parent link.
2. `Scene::overrideMaterial` is supported in the normal forward render-list projection path.
3. Public hot-path APIs avoid taking `std::vector` directly where a read-only contiguous view is sufficient; C++20 `std::span` is used for material arrays, indices, morph attributes and render item submission.

## Object3D parent ownership

`Object3D` still owns its children with `std::vector<std::shared_ptr<Object3D>> children`, matching the parent-owned scene-graph lifetime model used by the existing C++ examples.

The parent link is now:

```cpp
std::weak_ptr<Object3D> parent;
```

This prevents parent/child reference cycles when objects are heap-owned through `std::shared_ptr`. A private non-owning `parentRaw` fallback is kept only to preserve the existing stack-allocated `Scene scene;` examples. It is not an owning pointer and is cleared on `remove()` / `clear()`.

Use `parentObject()` when renderer/internal code needs a temporary raw pointer for matrix propagation.

## Scene override material

`Scene` now exposes:

```cpp
std::shared_ptr<Material> overrideMaterial;
```

During `GLRenderer::projectObject()`, the material selection follows the three.js rule:

```text
resolvedMaterial = scene.overrideMaterial ? scene.overrideMaterial : object/group material
```

Geometry groups and draw ranges are preserved. The override material replaces only the material used to classify, sort and draw each item. This means an opaque override material will place all renderables into the opaque queue, while a transparent/physical/transmissive override material will be classified accordingly.

## std::span migration

Changed APIs:

```cpp
RenderableObject::setMaterials(std::span<const std::shared_ptr<Material>>)
BufferGeometry::setIndex(std::span<const std::uint32_t>)
BufferGeometry::setMorphAttribute(std::span<const BufferAttribute>)
BufferAttribute::fromSpan(std::span<const T>)
GLRenderer::renderObjects(std::span<const RenderItem>)
GLRenderer::hasTransmissionItems(std::span<const RenderItem>)
```

Initializer-list convenience overloads are kept for ergonomic geometry construction. Existing internal call sites that used `std::move(vector)` were changed to explicit span views so ownership is not implied at the API boundary.

## Three.js parity notes

- `add()` removes the child from its previous parent before attaching it to the new parent.
- `remove()` and `clear()` clear the child's parent weak state.
- `updateMatrixWorld()` resolves the weak/non-owning parent through `parentObject()` before composing world matrices.
- `Scene::overrideMaterial` is applied before render-list queue classification, matching the observable three.js behavior where override material changes render state, shader permutation and transparent/opaque queue placement.

## Validation

The package was source-patched and statically checked for stale direct parent uses and stale `std::vector<RenderItem>` render-object signatures. A full local build was not completed in this container because the environment does not provide the required external `glm` package and network fetches are unavailable here. Build on the target machine with the normal xmake or CMake flow.

## 2026-06-07 hotfix

Fixed `GLRenderer::projectObject()` after adding `Scene::overrideMaterial`.

The previous implementation referenced `scene.overrideMaterial` from a function whose signature only accepted `Object3D& object, Camera& camera`, causing compile errors like:

```text
use of undeclared identifier 'scene'
```

The corrected signature is now:

```cpp
void projectObject(Scene& scene, Object3D& object, Camera& camera);
```

Call sites now pass the scene explicitly:

```cpp
projectObject(scene, scene, camera);
```

This keeps the three.js-compatible material resolution rule in the render-list build phase:

```cpp
Material* material = scene.overrideMaterial
    ? scene.overrideMaterial.get()
    : objectMaterial;
```
