#include "texture/ktx2-transcoder.h"
#include <sstream>

namespace THREE {

static bool hasExt(const char* all, const char* needle) {
    if (!all || !needle) return false;
    std::string s(all);
    return s.find(needle) != std::string::npos;
}

CompressedTextureCapabilities CompressedTextureCapabilities::queryGL() {
    CompressedTextureCapabilities c;
#if !THREECPP_USE_ANGLE
    c.desktopGL = true;
#endif
    const char* exts = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    c.s3tc = hasExt(exts, "GL_EXT_texture_compression_s3tc") || hasExt(exts, "GL_EXT_texture_compression_dxt1") || c.desktopGL;
    c.bc1 = c.s3tc;
    c.bc3 = c.s3tc;
    c.bc7 = hasExt(exts, "GL_ARB_texture_compression_bptc") || c.desktopGL;
    c.etc2 = hasExt(exts, "GL_ARB_ES3_compatibility") || hasExt(exts, "GL_OES_compressed_ETC2_RGB8_texture") || THREECPP_USE_ANGLE;
    c.astc4x4 = hasExt(exts, "GL_KHR_texture_compression_astc_ldr");
    return c;
}

const char* KTX2Transcoder::gpuFormatName(CompressedTextureGPUFormat fmt) {
    switch (fmt) {
        case CompressedTextureGPUFormat::RGBA8: return "RGBA8";
        case CompressedTextureGPUFormat::SRGB8Alpha8: return "SRGB8_ALPHA8";
        case CompressedTextureGPUFormat::BC1_RGB: return "BC1/DXT1";
        case CompressedTextureGPUFormat::BC3_RGBA: return "BC3/DXT5";
        case CompressedTextureGPUFormat::BC7_RGBA: return "BC7/BPTC";
        case CompressedTextureGPUFormat::ETC2_RGB: return "ETC2_RGB8";
        case CompressedTextureGPUFormat::ETC2_RGBA: return "ETC2_RGBA8";
        case CompressedTextureGPUFormat::ASTC_4x4_RGBA: return "ASTC_4x4";
        case CompressedTextureGPUFormat::None: return "None";
    }
    return "Unknown";
}

#ifndef GL_COMPRESSED_RGBA_BPTC_UNORM
#define GL_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#endif
#ifndef GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM 0x8E8D
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif
#ifndef GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#endif
#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#endif
#ifndef GL_COMPRESSED_SRGB_S3TC_DXT1_EXT
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT 0x8C4C
#endif
#ifndef GL_COMPRESSED_RGB8_ETC2
#define GL_COMPRESSED_RGB8_ETC2 0x9274
#endif
#ifndef GL_COMPRESSED_SRGB8_ETC2
#define GL_COMPRESSED_SRGB8_ETC2 0x9275
#endif
#ifndef GL_COMPRESSED_RGBA8_ETC2_EAC
#define GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278
#endif
#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC
#define GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC 0x9279
#endif
#ifndef GL_COMPRESSED_RGBA_ASTC_4x4_KHR
#define GL_COMPRESSED_RGBA_ASTC_4x4_KHR 0x93B0
#endif
#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR 0x93D0
#endif

GLenum KTX2Transcoder::glInternalFormat(CompressedTextureGPUFormat fmt, bool srgb) {
    switch (fmt) {
        case CompressedTextureGPUFormat::RGBA8: return GL_RGBA8;
        case CompressedTextureGPUFormat::SRGB8Alpha8: return GL_SRGB8_ALPHA8;
        case CompressedTextureGPUFormat::BC1_RGB: return srgb ? GL_COMPRESSED_SRGB_S3TC_DXT1_EXT : GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        case CompressedTextureGPUFormat::BC3_RGBA: return srgb ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        case CompressedTextureGPUFormat::BC7_RGBA: return srgb ? GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM : GL_COMPRESSED_RGBA_BPTC_UNORM;
        case CompressedTextureGPUFormat::ETC2_RGB: return srgb ? GL_COMPRESSED_SRGB8_ETC2 : GL_COMPRESSED_RGB8_ETC2;
        case CompressedTextureGPUFormat::ETC2_RGBA: return srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC : GL_COMPRESSED_RGBA8_ETC2_EAC;
        case CompressedTextureGPUFormat::ASTC_4x4_RGBA: return srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR : GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
        case CompressedTextureGPUFormat::None: return 0;
    }
    return 0;
}

bool KTX2Transcoder::isBlockCompressed(CompressedTextureGPUFormat fmt) {
    return fmt != CompressedTextureGPUFormat::None && fmt != CompressedTextureGPUFormat::RGBA8 && fmt != CompressedTextureGPUFormat::SRGB8Alpha8;
}

static CompressedTextureGPUFormat chooseFormat(const CompressedTextureCapabilities& caps, bool srgb) {
    (void)srgb;
    if (caps.astc4x4) return CompressedTextureGPUFormat::ASTC_4x4_RGBA;
    if (caps.bc7) return CompressedTextureGPUFormat::BC7_RGBA;
    if (caps.bc3) return CompressedTextureGPUFormat::BC3_RGBA;
    if (caps.etc2) return CompressedTextureGPUFormat::ETC2_RGBA;
    return CompressedTextureGPUFormat::None;
}

static std::vector<std::byte> makeFallbackRGBA(int w, int h) {
    std::vector<std::byte> out(static_cast<std::size_t>(std::max(1, w) * std::max(1, h) * 4));
    for (int y = 0; y < std::max(1, h); ++y) {
        for (int x = 0; x < std::max(1, w); ++x) {
            bool c = ((x / 16) ^ (y / 16)) & 1;
            std::size_t i = static_cast<std::size_t>((y * std::max(1, w) + x) * 4);
            out[i + 0] = std::byte(c ? 255 : 80);
            out[i + 1] = std::byte(c ? 80 : 200);
            out[i + 2] = std::byte(220);
            out[i + 3] = std::byte(255);
        }
    }
    return out;
}

KTX2TranscodeResult KTX2Transcoder::transcode(const Texture& texture,
                                              const CompressedTextureCapabilities& caps,
                                              const KTX2TranscodeOptions& options) {
    KTX2TranscodeResult r;
    r.width = texture.width;
    r.height = texture.height;
    const bool srgb = texture.colorSpace == ColorSpace::SRGB || options.preferSRGB;
    const auto target = (!options.forceRGBAFallback && options.allowCompressedUpload) ? chooseFormat(caps, srgb) : CompressedTextureGPUFormat::None;

#if THREECPP_ENABLE_BASISU
    // Integration point for Basis Universal transcoder. This project intentionally
    // keeps the include/link optional because many users build through xmake
    // without the transcoder checked out. Hook basisu_transcoder here and fill
    // either r.levels for compressed targets or r.rgbaPixels for RGBA fallback.
    (void)target;
#endif

    if (target != CompressedTextureGPUFormat::None && !texture.compressedLevels.empty() && texture.supercompressionScheme == 0) {
        // Unsupercompressed KTX2 containing already GPU-ready blocks can be uploaded directly.
        r.success = true;
        r.usedCompressedUpload = true;
        r.gpuFormat = target;
        r.gpuFormatName = gpuFormatName(target);
        r.levels = texture.compressedLevels;
        r.message = "direct KTX2 compressed level upload; BasisLZ/UASTC transcoder not required";
        return r;
    }

    if (options.allowRGBAFallback) {
        r.success = true;
        r.usedRGBAFallback = true;
        r.gpuFormat = srgb ? CompressedTextureGPUFormat::SRGB8Alpha8 : CompressedTextureGPUFormat::RGBA8;
        r.gpuFormatName = gpuFormatName(r.gpuFormat);
        r.rgbaPixels = makeFallbackRGBA(texture.width, texture.height);
        std::ostringstream msg;
        msg << "BasisU transcoder not linked or required target unavailable; uploaded RGBA8 diagnostic fallback. "
            << "KTX2 levels preserved=" << texture.compressedLevels.size();
        r.message = msg.str();
        return r;
    }

    r.success = false;
    r.message = "No compatible compressed target and RGBA fallback disabled";
    return r;
}

} // namespace THREE
