# v2.3 Assimp multi-format import

This version expands the importer layer for the realtime three.js-like renderer.

## Goals

Supported through Assimp:

- `.glb`
- `.gltf`
- `.fbx`
- `.obj`
- plus common fallback formats: `.dae`, `.3ds`, `.stl`, `.ply`

Primary validation target should still be glTF/GLB because it has the closest material semantics to three.js PBR.

## New API

```cpp
AssimpLoaderOptions opts;
opts.loadTextures = true;
opts.loadEmbeddedTextures = true;
opts.loadSkinning = true;
opts.loadAnimations = true;
opts.generateTangents = true;

AssimpLoader loader(opts);
AssimpLoadResult result = loader.loadResult("model.glb");
scene.add(result.root);
```

`AssimpLoadResult` contains:

```cpp
std::shared_ptr<Object3D> root;
std::vector<AnimationClip> animations;
std::string format;
bool hasMeshes;
bool hasSkins;
bool hasAnimations;
```

## Import features

### Geometry

- positions
- normals
- tangents
- uv
- uv2
- vertex colors
- indices
- bounding sphere

### PBR material mapping

Maps Assimp material properties to renderer materials:

- baseColor / diffuse -> `MeshStandardMaterial::color`
- opacity / glTF alphaMode -> transparency / alphaTest
- roughness factor
- metallic factor
- emissive color
- twoSided -> `Side::DoubleSide`
- baseColorTexture / diffuse texture -> `map`
- normal texture -> `normalMap`
- roughness texture -> `roughnessMap`
- metallic texture -> `metalnessMap`
- occlusion/lightmap -> `aoMap` / `lightMap`
- emissive texture -> `emissiveMap`
- opacity texture -> `alphaMap`

### Physical material extension hooks

The importer detects common glTF KHR material keys when Assimp exposes them:

- transmission factor
- ior
- volume thickness
- clearcoat factor
- clearcoat roughness
- sheen roughness

When detected, it upgrades the material to `MeshPhysicalMaterial`.

Texture enum names vary across Assimp versions, so v2.3 uses conservative numeric fallback slots for some KHR texture imports. Missing slots simply return null.

### Embedded textures

For GLB/FBX embedded textures:

- compressed embedded payload is stored in `Texture::pixels`
- uncompressed embedded RGBA payload is copied into `Texture::pixels`
- `Texture::sourcePath` uses `embedded://N`
- `Texture::embedded = true`
- `Texture::mimeType` stores the Assimp format hint

The actual PNG/JPEG decode path is still left to app integration / future stb_image wiring.

### Skinning

For meshes with bones:

- imports `skinIndex`
- imports `skinWeight`
- normalizes up to 4 weights per vertex
- creates `SkinnedMesh`
- resolves `Bone` nodes from the imported node hierarchy
- attaches orphan fallback bones if the source format does not expose bone nodes in the visible hierarchy
- assigns inverse bind matrices from Assimp bone offset matrices

### Animation

Imports Assimp node animation channels into renderer `AnimationClip` tracks:

- `NodeName.position`
- `NodeName.quaternion`
- `NodeName.scale`

The existing `AnimationMixer` can play the first clip directly.

## New example

Only compiled when Assimp is enabled:

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
xmake run 17_assimp_multi_format_viewer /path/to/model.glb
xmake run 17_assimp_multi_format_viewer /path/to/model.gltf
xmake run 17_assimp_multi_format_viewer /path/to/model.fbx
xmake run 17_assimp_multi_format_viewer /path/to/model.obj
```

## Notes

- OBJ has no standard PBR or skeleton semantics; it will load as static mesh material data only.
- FBX material semantics are inconsistent between exporters; glTF/GLB should be used as the PBR reference path.
- Full KHR extension fidelity still requires more format-specific testing against real assets.
