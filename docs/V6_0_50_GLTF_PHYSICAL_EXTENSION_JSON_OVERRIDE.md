# v6.0.50 glTF Physical Extension JSON Override

This version fixes GLB files where Assimp loads mesh geometry correctly but does not expose glTF material extensions such as:

- `KHR_materials_transmission`
- `KHR_materials_volume`
- `KHR_materials_ior`
- `KHR_materials_dispersion`

The loader now pre-parses the GLB JSON chunk and applies a material-index based override after Assimp conversion.

## DragonDispersion.glb case

The uploaded `DragonDispersion.glb` contains a dragon material named `Dragon with Attenuation` with:

- `transmissionFactor = 1`
- `thicknessFactor = 2.27`
- `attenuationColor = vec3(0.75, 0.8, 0.82)`
- `attenuationDistance = 0.155`
- `ior = 1.75`
- `dispersion = 2.04`
- `thicknessTexture = texture[0]`

Before v6.0.50 this material was imported as `MeshStandardMaterial` with `transmission=0`. It should now be promoted to `MeshPhysicalMaterial` and preserve the embedded JPEG thickness texture.

## Test

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
xmake run 38_asset_import_parity_lab /path/to/DragonDispersion.glb
```

Expected material dump:

```txt
material name='Dragon with Attenuation' type=MeshPhysical
transmission=1
thickness≈2.27
ior≈1.75
dispersion≈2.04
attenuationColor≈(0.75,0.8,0.82)
attenuationDistance≈0.155
thicknessMap: gltf://texture/0 ... decoded image payload via stb_image: image/jpeg
```
