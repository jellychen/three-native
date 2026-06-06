# Roadmap to approach three.js WebGLRenderer capability

## Completed/staged through v0.5

- Object3D render system with mesh, line, points and fatline paths.
- Basic materials: mesh, line, points and fatline.
- `MeshStandardMaterial` metallic/roughness PBR scaffold.
- Texture-map hooks for base color, normal, roughness, metalness, AO and emissive maps.
- `MeshPhysicalMaterial` API and approximate shader path for transmission, clearcoat, sheen, specular and iridescence.
- Skeleton/skinned mesh shader scaffold.
- Assimp loader bridge scaffold.

## v0.6: Real transmission path

- renderer-level transmission render target
- opaque background prepass for transmissive objects
- backside thickness pass
- screen-space refracted lookup
- attenuation through thickness map
- material sorting rules for transmission

## v0.7: PMREM / IBL parity

- HDR loader
- equirectangular-to-cubemap pass
- irradiance convolution
- prefiltered mip chain
- BRDF LUT generation/loading
- roughness-to-mip mapping matching three.js

## v0.8: Shadow system

- depth material variants
- directional and spot shadow maps
- point-light cube shadow maps
- PCF and bias/normalBias
- shadow receiver chunks in PBR shader

## v0.9: Assimp glTF/FBX parity

- robust glTF PBR material import
- `KHR_materials_transmission` import
- `KHR_materials_ior`, `specular`, `clearcoat`, `sheen` and `iridescence` import
- skeletal hierarchy import
- animation clip import
- texture path resolver

## v1.0: Renderer hardening

- frustum culling
- instancing
- VAO/material/program/state metrics
- render target and EffectComposer passes
- screenshot and conformance scenes compared against three.js
