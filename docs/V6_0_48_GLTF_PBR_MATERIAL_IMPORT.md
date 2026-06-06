# v6.0.48 glTF / GLB PBR Material Import Parity Fix

This version focuses on cases where geometry loads correctly but material parameters look wrong.

## Fixes

- Embedded GLB PNG/JPEG payloads are no longer uploaded as raw RGBA bytes.
- Optional `stb` package integration decodes embedded and external PNG/JPEG textures when available.
- Imported textures use `flipY=false`, matching glTF loader expectations.
- Base color and emissive textures are marked sRGB.
- Normal, roughness, metalness, AO, alpha, and thickness-like scalar textures are marked linear.
- glTF metallicRoughnessTexture keeps roughness in G and metalness in B.
- AO and lightMap default to uv2 when available.
- roughness/metalness factors use additional glTF raw key aliases exposed by different Assimp versions.
- MeshPhysicalMaterial copied from MeshStandardMaterial now keeps a unique material id.
- `38_asset_import_parity_lab` now prints a detailed material dump.

## Build

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
```

`stb` is optional via xmake package. If unavailable, the loader keeps safe white fallback for undecoded PNG/JPEG payloads instead of uploading compressed bytes as raw pixels.

## Test

```bash
xmake run 38_asset_import_parity_lab /path/to/model.glb
xmake run 53_regression_stability_runner --no-render /path/to/model.glb
```

Look for material dump lines:

- baseColor / roughness / metalness
- roughnessMap channel G
- metalnessMap channel B
- map/emissiveMap colorSpace=sRGB
- normal/roughness/metalness/ao colorSpace=Linear
- flipY=false
- physical transmission/thickness/clearcoat/sheena parameters
