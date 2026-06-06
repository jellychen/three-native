# v2.4 Morph Targets / Blend Shapes

This version adds a three.js-style morph target path for realtime rendering.

## Added

- `BufferGeometry::morphAttributes`
- `BufferGeometry::morphTargetsRelative`
- `Mesh::morphTargetInfluences`
- `Mesh::morphTargetDictionary`
- `Mesh::setMorphTargetInfluence(index, value)`
- `Mesh::setMorphTargetInfluence(name, value)`
- shader defines:
  - `USE_MORPHTARGETS`
  - `USE_MORPHNORMALS`
  - `MORPHTARGETS_RELATIVE`
- GPU upload of the first 4 position morph targets and first 4 normal morph targets
- animation track support for:
  - `MeshName.morphTargetInfluences[0]`
  - `MeshName.morphTargetInfluences[Smile]`
- Assimp import of `aiAnimMesh` blend shape position/normal targets
- Assimp import of `aiMeshMorphAnim` scalar morph weight tracks

## Test

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 18_morph_targets_test
```

For imported glTF / GLB / FBX morph targets:

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
xmake run 17_assimp_multi_format_viewer /path/to/model.glb
```

## Current limits

- Renderer uploads 4 morph position targets and 4 morph normal targets per draw.
- Morph color targets are stored in the data model but not yet consumed by shaders.
- Targets are handled as absolute values by default for Assimp imports, matching typical glTF/FBX blend shape payloads.
- For complete three.js parity, the next steps are morph target texture fallback for many targets, morph color support, morph + tangent-space normal refinement, and broader glTF animation conformance tests.
