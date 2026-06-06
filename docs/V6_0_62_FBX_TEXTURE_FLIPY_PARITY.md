# V6.0.62 - FBX texture orientation parity

## Problem

`Capoeira.fbx` could resolve its embedded textures after V6.0.60, but the rendered result looked offset/misaligned. The loader had forced every Assimp-imported texture to `flipY=false`, which is correct for glTF but not for FBX/OBJ/DAE in a three.js-like pipeline.

In three.js, `Texture.flipY` defaults to `true`, and `GLTFLoader` explicitly sets glTF textures to `flipY=false`. FBX textures loaded through the ordinary texture path keep the default orientation.

## Fix

### AssimpLoader

`AssimpLoader::loadMaterialTexture()` now chooses texture orientation by imported asset format:

- `.gltf` / `.glb`: `flipY=false`
- `.fbx`, `.obj`, `.dae`, etc.: `flipY=true`
- if `AssimpLoaderOptions::flipUVs=true`, pixels are not flipped again

This keeps glTF behavior stable while matching three.js behavior for FBX.

### GLResourceManager

`GLResourceManager::getOrCreateTexture()` now actually honors `Texture::flipY` for CPU-decoded textures by uploading a vertically-flipped copy to OpenGL.

Before this version, `Texture::flipY` existed as state but was ignored during upload, so changing it in the loader had no visible effect.

## Notes

Compressed KTX2 upload is not CPU-flipped in this patch. glTF/KTX2 assets should continue to use `flipY=false`.
