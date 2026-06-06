#include "common.hpp"

#if THREECPP_ENABLE_STB_IMAGE
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#ifndef STBI_NO_STDIO
// Keep stdio enabled so future file loaders may use stb directly if needed.
#endif
#include <stb_image.h>
#endif
