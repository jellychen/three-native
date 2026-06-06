# v2.2 Skinning and PBR Material Test Scenes

This drop adds two focused validation scenes for the three.js-like realtime renderer.

## 15_skinning_animation_test

Purpose:

- Exercises `SkinnedMesh` render path.
- Generates a procedural three-bone skinned cylinder.
- Uploads `skinIndex` and `skinWeight` attributes.
- Uses `AnimationMixer` to animate `Bone_Mid.quaternion` and `Bone_Tip.quaternion`.
- Renders with `MeshStandardMaterial`, directional/point lights, environment lighting, shadow casting/receiving, and `SkeletonHelper`.

Run:

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 15_skinning_animation_test
```

Expected result:

- A vertical skinned tube bends continuously.
- Skeleton helper lines follow the bones.
- The skinned mesh casts and receives shadows.
- Camera orbits the object.

## 16_pbr_material_gallery

Purpose:

- Exercises multiple `MeshStandardMaterial` and `MeshPhysicalMaterial` parameter combinations.
- Shows roughness sweep, metalness sweep, texture slots, normal maps, roughness/metalness maps, and several physical-material parameters.
- Uses procedural textures so the example does not require external assets.
- Uses HDR-like procedural equirectangular environment through the current PMREM facade.

Run:

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 16_pbr_material_gallery
```

Expected result:

- Row 1: dielectric roughness gradient.
- Row 2: metalness gradient.
- Row 3: textured standard materials with stripe map, normal map, roughness map, and optional metalness map.
- Row 4: physical-material parameter tests for transmission, clearcoat, sheen, iridescence and anisotropy parameter paths.

## Notes

These tests are intended as renderer regression tests, not final art demos. They help validate that geometry attributes, material program keys, texture binding, lights, shadows, environment sampling, and animation updates are connected in one place.
