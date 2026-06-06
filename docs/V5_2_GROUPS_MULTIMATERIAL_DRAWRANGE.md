# V5.2 BufferGeometry groups / multi-material / drawRange

This version adds the core three.js-style geometry draw partitioning path.

## Implemented

- `GeometryGroup { start, count, materialIndex }`
- `DrawRange { start, count }`
- `BufferGeometry::addGroup()`
- `BufferGeometry::clearGroups()`
- `BufferGeometry::setDrawRange()`
- `RenderableObject::materials`
- `RenderableObject::setMaterials()`
- `RenderableObject::materialAt()`
- RenderList splits one renderable into one `RenderItem` per active group.
- `RenderItem` now carries `groupStart`, `groupCount`, and `materialIndex`.
- Forward draw path uses `glDrawElements` / `glDrawArrays` with the group range.
- Instanced draw path uses `glDrawElementsInstanced` / `glDrawArraysInstanced` with the group range.
- Shadow depth pass respects groups, drawRange, and per-group transparent/alphaTest skip logic.

## Why this matters

three.js-compatible asset import requires this feature. glTF primitives, OBJ/MTL material ranges, FBX material IDs, CAD exports, and editor-created meshes frequently need one geometry to be rendered with multiple materials.

## Example

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 45_groups_multimaterial_drawrange
```

The example validates:

- one box with six geometry groups and six materials;
- one box with groups clipped by drawRange;
- one InstancedMesh using geometry groups and material array;
- shadow pass compatibility.

## Remaining work

- Importer-side conversion of native Assimp material ranges into `BufferGeometry::groups` when needed.
- More precise transparent sorting for groups sharing the same object.
- Persistent RenderList cache keyed by geometry groups / material versions.
- Group-level bounding information for extremely large grouped meshes.
