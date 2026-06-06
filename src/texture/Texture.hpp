#pragma once
#include "common.hpp"

namespace threecpp {

enum class TextureFormat { RGBA8, SRGB8Alpha8, RGB16F, RGBA16F, Depth24Stencil8 };
enum class TextureWrap { ClampToEdge, Repeat, MirroredRepeat };
enum class TextureFilter { Nearest, Linear, LinearMipmapLinear };
enum class TextureMapping { UV, EquirectangularReflection, EquirectangularRefraction, CubeReflection, CubeRefraction };
enum class CompressedTextureContainer { None, KTX2, BasisUniversal, DDS, Unknown };

enum class CompressedTextureGPUFormat {
    None,
    RGBA8,
    SRGB8Alpha8,
    BC1_RGB,
    BC3_RGBA,
    BC7_RGBA,
    ETC2_RGB,
    ETC2_RGBA,
    ASTC_4x4_RGBA
};

struct CompressedTextureLevel {
    std::uint32_t level = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint64_t byteOffset = 0;
    std::uint64_t byteLength = 0;
    std::uint64_t uncompressedByteLength = 0;
    std::vector<std::byte> data;
};

struct TextureTransform {
    glm::vec2 offset{0.0f};
    glm::vec2 scale{1.0f};
    glm::vec2 center{0.0f};
    float rotation = 0.0f;
    int texCoord = 0;
    bool enabled = false;

    glm::mat3 matrix() const {
        const float c = std::cos(rotation);
        const float si = std::sin(rotation);
        glm::mat3 t1(1.0f);
        t1[2] = glm::vec3(-center, 1.0f);
        glm::mat3 r(1.0f);
        r[0][0] = c;  r[0][1] = si;
        r[1][0] = -si; r[1][1] = c;
        glm::mat3 sc(1.0f);
        sc[0][0] = scale.x;
        sc[1][1] = scale.y;
        glm::mat3 t2(1.0f);
        t2[2] = glm::vec3(center + offset, 1.0f);
        return t2 * r * sc * t1;
    }
};

enum class ColorSpace { LinearSRGB, SRGB };
enum class TextureChannel { R = 0, G = 1, B = 2, A = 3 };

class Texture {
public:
    ObjectId id = next_object_id();
    std::string name;
    int width = 0;
    int height = 0;
    int channels = 4;
    TextureFormat format = TextureFormat::RGBA8;
    ColorSpace colorSpace = ColorSpace::SRGB;
    TextureWrap wrapS = TextureWrap::ClampToEdge;
    TextureWrap wrapT = TextureWrap::ClampToEdge;
    TextureFilter minFilter = TextureFilter::LinearMipmapLinear;
    TextureFilter magFilter = TextureFilter::Linear;
    TextureMapping mapping = TextureMapping::UV;
    int uvChannel = 0;
    TextureChannel scalarChannel = TextureChannel::R;
    bool generateMipmaps = true;
    bool flipY = true;
    float anisotropy = 1.0f;
    glm::vec2 offset{0.0f};
    glm::vec2 repeat{1.0f};
    glm::vec2 center{0.0f};
    float rotation = 0.0f;
    TextureTransform textureTransform;
    bool hasTextureTransform = false;
    std::vector<std::byte> pixels;

    CompressedTextureContainer compressedContainer = CompressedTextureContainer::None;
    bool compressed = false;
    bool compressedUploadReady = false;
    bool transcodedToRGBA = false;
    bool transcodeFailed = false;
    std::string transcodeMessage;
    std::string compressionScheme;
    CompressedTextureGPUFormat gpuCompressedFormat = CompressedTextureGPUFormat::None;
    std::string gpuCompressedFormatName;
    std::uint32_t vkFormat = 0;
    std::uint32_t typeSize = 1;
    std::uint32_t layerCount = 0;
    std::uint32_t faceCount = 1;
    std::uint32_t levelCount = 1;
    std::uint32_t supercompressionScheme = 0;
    std::vector<CompressedTextureLevel> compressedLevels;

    std::string sourcePath;
    std::string mimeType;
    bool embedded = false;

    std::uint64_t version = 0;
    void markNeedsUpdate() { ++version; }

    glm::mat3 uvTransform() const {
        const float c = std::cos(rotation);
        const float s = std::sin(rotation);
        glm::mat3 t1(1.0f);
        t1[2] = glm::vec3(-center, 1.0f);
        glm::mat3 r(1.0f);
        r[0][0] = c;  r[0][1] = s;
        r[1][0] = -s; r[1][1] = c;
        glm::mat3 sc(1.0f);
        sc[0][0] = repeat.x;
        sc[1][1] = repeat.y;
        glm::mat3 t2(1.0f);
        t2[2] = glm::vec3(center + offset, 1.0f);
        if (hasTextureTransform) return textureTransform.matrix();
        return t2 * r * sc * t1;
    }

    void applyTextureTransform(const TextureTransform& tr) {
        textureTransform = tr;
        hasTextureTransform = tr.enabled;
        if (tr.enabled) {
            offset = tr.offset;
            repeat = tr.scale;
            center = tr.center;
            rotation = tr.rotation;
            uvChannel = tr.texCoord;
        }
        markNeedsUpdate();
    }
};

class CubeTexture : public Texture {
public:
    int mipLevels = 1;
    std::array<std::vector<std::byte>, 6> faces;
    std::vector<std::array<std::vector<std::byte>, 6>> mipFaces;
};

} // namespace threecpp
