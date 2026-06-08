#pragma once
#include "texture/texture.h"
#include "platform/gl-headers.h"

namespace THREE {

struct CompressedTextureCapabilities {
    bool bc1 = false;
    bool bc3 = false;
    bool bc7 = false;
    bool etc2 = false;
    bool astc4x4 = false;
    bool s3tc = false;
    bool desktopGL = false;

    static CompressedTextureCapabilities queryGL();
};

struct KTX2TranscodeOptions {
    bool preferSRGB = true;
    bool allowCompressedUpload = true;
    bool allowRGBAFallback = true;
    bool forceRGBAFallback = false;
};

struct KTX2TranscodeResult {
    bool success = false;
    bool usedCompressedUpload = false;
    bool usedRGBAFallback = false;
    CompressedTextureGPUFormat gpuFormat = CompressedTextureGPUFormat::None;
    std::string gpuFormatName;
    std::string message;
    std::vector<CompressedTextureLevel> levels;
    std::vector<std::byte> rgbaPixels;
    int width = 0;
    int height = 0;
};

class KTX2Transcoder {
public:
    // Selects the best upload target for this GPU. UASTC/ETC1S transcoding via
    // BasisU is optional; without THREECPP_ENABLE_BASISU this function returns
    // a deterministic RGBA checker fallback while preserving all KTX2 metadata.
    static KTX2TranscodeResult transcode(const Texture& texture,
                                         const CompressedTextureCapabilities& caps,
                                         const KTX2TranscodeOptions& options = {});

    static const char* gpuFormatName(CompressedTextureGPUFormat fmt);
    static GLenum glInternalFormat(CompressedTextureGPUFormat fmt, bool srgb);
    static bool isBlockCompressed(CompressedTextureGPUFormat fmt);
};

} // namespace THREE
