# v6.0.49 - stb_image Link Fix

## Problem

When building with `stb` available, `AssimpLoader.cpp` included `stb_image.h` and called:

- `stbi_load_from_memory`
- `stbi_image_free`

but no translation unit defined `STB_IMAGE_IMPLEMENTATION`. This caused arm64 linker errors:

```txt
Undefined symbols for architecture arm64:
  "_stbi_image_free"
  "_stbi_load_from_memory"
```

## Fix

Added:

```txt
src/thirdparty/StbImage.cpp
```

This file defines `STB_IMAGE_IMPLEMENTATION` exactly once when `THREECPP_ENABLE_STB_IMAGE=1`.

## Notes

- `AssimpLoader.cpp` remains a user of the stb API only.
- The stb implementation is isolated in one translation unit.
- If the `stb` xmake package is unavailable, `THREECPP_ENABLE_STB_IMAGE=0` and the fallback path is used.
