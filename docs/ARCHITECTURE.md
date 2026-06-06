# threecpp_renderer architecture v0.2

## Design target

The renderer is an Object3D render system inspired by `three.js` `WebGLRenderer`, not just a mesh renderer.

Renderable object types are first-class:

- `Mesh`
- `SkinnedMesh`
- `Line`
- `LineSegments`
- `LineLoop`
- `Points`
- `FatLine`
- future `Sprite`

## Core render flow

```text
renderer.render(scene, camera)
    scene.updateMatrixWorld()
    camera.updateMatrixWorld()
    project visible renderables into RenderList
    sort opaque front-to-back by program/material/geometry
    sort transparent back-to-front
    clear target
    render opaque
    render transparent
```

## Attribute contract

Common three.js-style attribute names are preserved:

```text
position      -> location 0
normal        -> location 1, mesh path
uv            -> location 2, mesh path
color         -> location 3
skinIndex     -> location 4
skinWeight    -> location 5
lineDistance  -> location 6
```

FatLine uses a different shader contract:

```text
position      -> location 0, control: x start/end, y side
instanceStart -> location 1
instanceEnd   -> location 2
lineDistance  -> location 6
```

This mirrors the three.js `Line2` / `LineSegments2` idea: the CPU stores line segment endpoints, and the vertex shader expands the segment in screen space.

## Current shader paths

- `mesh_basic`
- `line_basic`
- `points`
- `fat_line`
- `mesh_standard` scaffold, currently rendered with basic color until PBR phase

## FatLine implementation

v0.2 uses shader-side screen-space extrusion:

1. `FatLineGeometry::fromPolyline()` creates one quad per segment.
2. Each quad stores `instanceStart` and `instanceEnd`.
3. Vertex shader projects start/end to clip space.
4. It computes NDC direction.
5. It offsets the selected endpoint by a perpendicular vector scaled by viewport resolution and `linewidth`.

This is much closer to three.js `Line2` than the v0.1 object-space fallback.

## ANGLE boundary

`AngleContext` owns:

- `EGLDisplay`
- `EGLContext`
- `EGLSurface`

`Window` owns the GLFW window. `AngleContext` extracts platform-native handles with `glfw3native.h` when `THREECPP_USE_ANGLE=1`.

## Planned next phases

1. Texture system: image loading, GL texture cache, sRGB/linear handling.
2. PBR direct lighting: GGX, Smith, Schlick Fresnel.
3. IBL: HDR loading, cubemap, irradiance, prefilter, BRDF LUT.
4. Light packing: ambient/directional/point/spot/hemisphere.
5. Shadow map pass.
6. Assimp skeleton and animation conversion.
7. GPU skinning.
8. Sprite/helper system.
9. RenderTarget and post-processing.
