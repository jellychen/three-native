# v0.3 implementation notes

The renderer now treats meshes, lines, points, fat lines and sprites as peer renderable object types under `Object3D`, rather than making the renderer mesh-only. This matches the architectural requirement for editor helpers, gizmos, point clouds, curve editors and fatline outlines.

The shader layer now uses a `ProgramKey` with primitive type, material type, vertex color, skinning, IBL and light-count flags. This keeps the renderer closer to the way three.js produces shader permutations while keeping C++ code statically typed.

The PBR shader is intentionally simplified but uses a Cook-Torrance structure with GGX distribution, Smith visibility and Schlick Fresnel. It is suitable as the base that later receives texture-map sampling, IBL prefilter sampling, shadowing and MeshPhysical extensions.

FatLine keeps the three.js `Line2` style idea: line width is expanded in clip/screen space inside the vertex shader and rendered as triangles rather than relying on `glLineWidth`, which is not portable in OpenGL ES / WebGL.

Skeleton support is represented by `Bone`, `Skeleton`, `SkinnedMesh`, `bindMatrix`, `bindMatrixInverse` and `boneMatrices[128]` shader upload. The animation mixer API exists, but actual keyframe target binding is staged for the next milestone.
