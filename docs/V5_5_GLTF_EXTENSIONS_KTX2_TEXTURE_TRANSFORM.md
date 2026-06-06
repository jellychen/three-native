# v5.5 glTF extensions / KTX2 / BasisU / texture transform

This release focuses on asset import parity areas that are required before the renderer can be validated against real glTF scenes at scale.

## Added

- `TextureTransform` metadata, matching `KHR_texture_transform` fields:
  - `offset`
  - `scale`
  - `center`
  - `rotation`
  - `texCoord`
- `Texture::hasTextureTransform` and `Texture::applyTextureTransform()`.
- `Texture::uvTransform()` now returns the extension transform when present.
- KTX2/BasisU compressed texture staging metadata:
  - `compressed`
  - `compressedContainer`
  - `compressionScheme`
  - `vkFormat`
  - `levelCount`
  - `supercompressionScheme`
- `TextureLoader::readKTX2Header()` and `TextureLoader::loadKTX2Metadata()`.
- `GltfExtensionsReport` for importer diagnostics.
- `AssimpLoader` extension support report for common glTF extensions:
  - `KHR_texture_transform`
  - `KHR_texture_basisu`
  - `KHR_materials_emissive_strength`
  - `KHR_materials_ior`
  - `KHR_materials_volume`
  - `KHR_materials_transmission`
  - `KHR_materials_clearcoat`
  - `KHR_materials_sheen`
  - `KHR_materials_specular`
  - `KHR_materials_iridescence`
  - `KHR_materials_anisotropy`
- New validation example:
  - `examples/48_gltf_extensions_texture_transform`

## Important boundary

KTX2/BasisU decoding is staged, not fully transcoded yet. The renderer now preserves and reports compressed payload metadata, but a true GPU upload path still needs a BasisU/KTX2 transcoder and GPU format selection.

This is intentional. A correct implementation should choose runtime-supported output formats such as BC7/BC3/ETC2/ASTC/PVRTC or fallback RGBA before upload.

## Run

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
xmake run 48_gltf_extensions_texture_transform /path/to/model.glb
```

Headless import-only check:

```bash
xmake run 48_gltf_extensions_texture_transform /path/to/model.glb --no-render
```

## Next work

- Hook a real KTX2/BasisU transcoder.
- Upload transcoded GPU compressed textures when supported.
- Parse `KHR_texture_transform` directly from glTF JSON when Assimp does not expose enough metadata.
- Add extension-specific regression models.
