# v1.2.1 No Ray Tracing Branch

This package removes the CPU ray tracer and the ray-tracing example from v1.2.

The renderer roadmap is now focused on three.js-style real-time rendering only:

- MeshBasic / Lambert / Phong / Standard / Physical materials
- PBR direct lighting
- IBL / PMREM
- Shadow maps
- Assimp/glTF import
- SkinnedMesh and animation
- Line / Points / FatLine
- Geometry helpers and editor helpers

The next high-value target should be real shadow-map rendering or a real PMREM GPU pipeline.
