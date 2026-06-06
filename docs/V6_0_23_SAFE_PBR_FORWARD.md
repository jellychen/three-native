# v6.0.23 Safe PBR Forward

`THREECPP_ENABLE_EXPERIMENTAL_PBR=1` now uses a robust Standard/Physical forward shader path instead of the previously over-complex experimental fragment branch that produced invisible meshes on macOS Core GL.

The new path keeps visible PBR semantics first:
- baseColor / map / vertexColors
- roughness / metalness and glTF channel selection
- alpha / alphaMap
- direct light loop for Directional / Point / Spot / RectAreaLight approximation
- approximate environment diffuse/specular
- PMREM/equirect hooks where available
- clearcoat/sheen/transmission approximations for PhysicalMaterial

It intentionally avoids fragile shadow/transmission framebuffer branches while the full three.js parity shader chunks are being rebuilt.
