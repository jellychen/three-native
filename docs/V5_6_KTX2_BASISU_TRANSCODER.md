# v5.6 KTX2 / BasisU Transcoder and GPU Upload

This version adds the real KTX2 staging and OpenGL upload path used by the renderer:

- `TextureLoader::readKTX2Header()` now parses the KTX2 level index.
- `TextureLoader::loadKTX2()` / `loadKTX2Metadata()` preserve payload and per-mip data.
- `KTX2Transcoder` selects a target format from GPU capabilities: ASTC, BC7, BC3, ETC2, or RGBA8 fallback.
- `GLResourceManager` uploads block-compressed levels with `glCompressedTexImage2D` when the KTX2 payload is already GPU-ready.
- If BasisU transcoding is not linked or no compatible GPU compressed target exists, the renderer uploads a visible RGBA8 diagnostic fallback instead of failing silently.
- `AssimpLoader` auto-detects `.ktx2` / `KHR_texture_basisu` textures and loads KTX2 mip metadata and payload for external files.
- `ImportCompatibilityReport` reports compressed/KTX2/BasisU counts and transcode/upload state.
- `examples/49_ktx2_transcoder_viewer` can inspect standalone `.ktx2` files or glTF/GLB/FBX/OBJ models.

## Build

Standalone KTX2 texture viewer does not require Assimp:

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 49_ktx2_transcoder_viewer /path/to/texture.ktx2
```

Model viewer requires Assimp:

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
xmake run 49_ktx2_transcoder_viewer /path/to/model.glb
```

Optional BasisU hook:

```bash
xmake f -m debug --use_angle=false --enable_assimp=true --enable_basisu=true
```

`--enable_basisu=true` currently enables the integration hook. Add the Basis Universal transcoder sources/libraries in your local build to replace the diagnostic RGBA fallback with true ETC1S/UASTC transcoding.

## Current Limitations

This version implements KTX2 container parsing, mip extraction, GPU target selection, compressed-level upload for already GPU-ready KTX2 data, and RGBA fallback. It does not vendor Basis Universal source files into the package, so BasisLZ/UASTC transcoding still requires the external transcoder to be linked by the user.
