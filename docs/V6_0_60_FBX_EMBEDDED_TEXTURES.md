# v6.0.60 - FBX Embedded Texture Import Fix

## Problem

Some FBX files, especially Mixamo-style exports, store material texture references as ordinary file paths such as:

- `Ch46_1001_Diffuse.png`
- `Ch46_1001_Normal.png`
- `Ch46_1001_Specular.png`
- `Ch46_1001_Glossiness.png`

while the actual image bytes are embedded in the FBX. Assimp may expose those embedded images through `aiScene::mTextures[i]` with `aiTexture::mFilename`, not through the `*0`, `*1` numeric embedded texture syntax.

The previous loader only handled the numeric embedded path form. As a result, `GetTexture()` returned a path, external lookup failed, no pixels were decoded, and the renderer uploaded the white fallback texture. The model rendered as a white mesh even though the FBX contained texture data.

## Fix

`AssimpLoader` now resolves embedded textures using both forms:

1. Numeric embedded references: `*0`, `*1`, ...
2. FBX filename references matched against `aiTexture::mFilename` by normalized basename.

The loader now checks embedded textures before falling back to external filesystem lookup.

## Additional FBX robustness

The material importer now also searches `aiTextureType_UNKNOWN` by common filename hints when Assimp does not classify FBX texture slots cleanly:

- Base color: `diffuse`, `albedo`, `basecolor`, `base_color`, `color`
- Normal: `normal`, `nrm`, `bump`
- Roughness: `roughness`, `rough`
- Metalness: `metallic`, `metalness`, `metal`
- AO: `ao`, `ambientocclusion`, `occlusion`
- Emissive: `emissive`, `emission`, `emit`
- Alpha: `opacity`, `alpha`

This keeps behavior closer to three.js loader behavior, where texture intent is resolved from both material metadata and common asset conventions.

## stb_image is now required

FBX embedded PNG/JPEG/TGA payloads must be decoded before GL upload. The previous optional `stb` dependency could silently build `THREECPP_ENABLE_STB_IMAGE=0`, which necessarily produced white fallback textures for compressed embedded media.

`stb` is now a required xmake package and `THREECPP_ENABLE_STB_IMAGE=1` is always defined for the core target.

Build with Assimp:

```bash
xmake f --enable_assimp=true
xmake
```

## Files changed

- `src/loader/AssimpLoader.cpp`
- `xmake.lua`
- `src/xmake.lua`
