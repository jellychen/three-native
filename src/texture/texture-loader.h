#pragma once
#include "texture/texture.h"
#include <fstream>
#include <sstream>

namespace THREE {

class TextureLoader {
public:
    // Minimal dependency-free PPM P6 loader. It is intentionally small and is
    // meant for renderer tests; production image loading should be backed by
    // stb_image, libktx or platform image codecs.
    static std::shared_ptr<Texture> loadPPM(const std::filesystem::path& path, bool srgb = true) {
        std::ifstream f(path, std::ios::binary);
        if (!f) throw std::runtime_error("TextureLoader::loadPPM failed to open " + path.string());
        std::string magic;
        f >> magic;
        if (magic != "P6") throw std::runtime_error("TextureLoader::loadPPM expected P6 file");
        auto skipComments = [&]() {
            while (f >> std::ws && f.peek() == '#') {
                std::string line;
                std::getline(f, line);
            }
        };
        skipComments();
        int w = 0, h = 0, maxv = 255;
        f >> w; skipComments(); f >> h; skipComments(); f >> maxv;
        f.get();
        if (w <= 0 || h <= 0 || maxv <= 0) throw std::runtime_error("TextureLoader::loadPPM invalid header");
        std::vector<unsigned char> rgb(static_cast<std::size_t>(w * h * 3));
        f.read(reinterpret_cast<char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));
        auto t = std::make_shared<Texture>();
        t->name = path.filename().string();
        t->width = w;
        t->height = h;
        t->channels = 4;
        t->format = srgb ? TextureFormat::SRGB8Alpha8 : TextureFormat::RGBA8;
        t->colorSpace = srgb ? ColorSpace::SRGB : ColorSpace::LinearSRGB;
        t->pixels.resize(static_cast<std::size_t>(w * h * 4));
        for (int i = 0; i < w * h; ++i) {
            t->pixels[static_cast<std::size_t>(i * 4 + 0)] = std::byte(rgb[static_cast<std::size_t>(i * 3 + 0)]);
            t->pixels[static_cast<std::size_t>(i * 4 + 1)] = std::byte(rgb[static_cast<std::size_t>(i * 3 + 1)]);
            t->pixels[static_cast<std::size_t>(i * 4 + 2)] = std::byte(rgb[static_cast<std::size_t>(i * 3 + 2)]);
            t->pixels[static_cast<std::size_t>(i * 4 + 3)] = std::byte(255);
        }
        t->markNeedsUpdate();
        return t;
    }



    static std::shared_ptr<Texture> loadRGBE(const std::filesystem::path& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) throw std::runtime_error("TextureLoader::loadRGBE failed to open " + path.string());

        std::string line;
        std::getline(f, line);
        if (line.rfind("#?RADIANCE", 0) != 0 && line.rfind("#?RGBE", 0) != 0) {
            throw std::runtime_error("TextureLoader::loadRGBE expected Radiance HDR header");
        }
        bool formatRGBE = false;
        while (std::getline(f, line)) {
            if (line.empty()) break;
            if (line.find("FORMAT=32-bit_rle_rgbe") != std::string::npos) formatRGBE = true;
        }
        if (!formatRGBE) throw std::runtime_error("TextureLoader::loadRGBE unsupported HDR format");

        std::getline(f, line);
        int width = 0, height = 0;
        if (std::sscanf(line.c_str(), "-Y %d +X %d", &height, &width) != 2 &&
            std::sscanf(line.c_str(), "+Y %d +X %d", &height, &width) != 2) {
            throw std::runtime_error("TextureLoader::loadRGBE invalid resolution line: " + line);
        }
        if (width <= 0 || height <= 0) throw std::runtime_error("TextureLoader::loadRGBE invalid size");

        auto tex = std::make_shared<Texture>();
        tex->name = path.filename().string();
        tex->sourcePath = path.string();
        tex->width = width;
        tex->height = height;
        tex->channels = 4;
        tex->format = TextureFormat::RGBA8;
        tex->colorSpace = ColorSpace::LinearSRGB;
        tex->mapping = TextureMapping::EquirectangularReflection;
        tex->wrapS = TextureWrap::Repeat;
        tex->wrapT = TextureWrap::ClampToEdge;
        tex->generateMipmaps = true;
        tex->pixels.resize(static_cast<std::size_t>(width * height * 4));

        std::vector<std::uint8_t> scanline(static_cast<std::size_t>(width * 4));
        std::vector<std::uint8_t> rgbe(static_cast<std::size_t>(4));
        for (int y = 0; y < height; ++y) {
            f.read(reinterpret_cast<char*>(rgbe.data()), 4);
            if (!f) throw std::runtime_error("TextureLoader::loadRGBE unexpected EOF");
            if (width < 8 || width > 32767 || rgbe[0] != 2 || rgbe[1] != 2 || (rgbe[2] & 0x80)) {
                // Old non-RLE path: the first pixel has already been read.
                auto writeRGBE = [&](int x, const std::uint8_t* px) {
                    float scale = px[3] ? std::ldexp(1.0f, int(px[3]) - (128 + 8)) : 0.0f;
                    glm::vec3 c(float(px[0]) * scale, float(px[1]) * scale, float(px[2]) * scale);
                    c = c / (glm::vec3(1.0f) + c * 0.35f);
                    std::size_t i = static_cast<std::size_t>((y * width + x) * 4);
                    tex->pixels[i + 0] = std::byte(std::uint8_t(glm::clamp(c.r, 0.0f, 1.0f) * 255.0f));
                    tex->pixels[i + 1] = std::byte(std::uint8_t(glm::clamp(c.g, 0.0f, 1.0f) * 255.0f));
                    tex->pixels[i + 2] = std::byte(std::uint8_t(glm::clamp(c.b, 0.0f, 1.0f) * 255.0f));
                    tex->pixels[i + 3] = std::byte(255);
                };
                writeRGBE(0, rgbe.data());
                for (int x = 1; x < width; ++x) {
                    f.read(reinterpret_cast<char*>(rgbe.data()), 4);
                    writeRGBE(x, rgbe.data());
                }
                continue;
            }
            int scanWidth = (int(rgbe[2]) << 8) | int(rgbe[3]);
            if (scanWidth != width) throw std::runtime_error("TextureLoader::loadRGBE corrupt scanline width");
            for (int channel = 0; channel < 4; ++channel) {
                int x = 0;
                while (x < width) {
                    unsigned char code = 0;
                    f.read(reinterpret_cast<char*>(&code), 1);
                    if (code > 128) {
                        int count = code - 128;
                        unsigned char value = 0;
                        f.read(reinterpret_cast<char*>(&value), 1);
                        for (int i = 0; i < count && x < width; ++i) scanline[static_cast<std::size_t>(x++ * 4 + channel)] = value;
                    } else {
                        int count = code;
                        for (int i = 0; i < count && x < width; ++i) {
                            unsigned char value = 0;
                            f.read(reinterpret_cast<char*>(&value), 1);
                            scanline[static_cast<std::size_t>(x++ * 4 + channel)] = value;
                        }
                    }
                }
            }
            for (int x = 0; x < width; ++x) {
                const std::uint8_t* px = scanline.data() + std::size_t(x * 4);
                float scale = px[3] ? std::ldexp(1.0f, int(px[3]) - (128 + 8)) : 0.0f;
                glm::vec3 c(float(px[0]) * scale, float(px[1]) * scale, float(px[2]) * scale);
                c = c / (glm::vec3(1.0f) + c * 0.35f);
                std::size_t i = static_cast<std::size_t>((y * width + x) * 4);
                tex->pixels[i + 0] = std::byte(std::uint8_t(glm::clamp(c.r, 0.0f, 1.0f) * 255.0f));
                tex->pixels[i + 1] = std::byte(std::uint8_t(glm::clamp(c.g, 0.0f, 1.0f) * 255.0f));
                tex->pixels[i + 2] = std::byte(std::uint8_t(glm::clamp(c.b, 0.0f, 1.0f) * 255.0f));
                tex->pixels[i + 3] = std::byte(255);
            }
        }
        tex->markNeedsUpdate();
        return tex;
    }



    struct KTX2LevelIndexInfo {
        std::uint64_t byteOffset = 0;
        std::uint64_t byteLength = 0;
        std::uint64_t uncompressedByteLength = 0;
    };

    struct KTX2HeaderInfo {
        bool valid = false;
        std::uint32_t vkFormat = 0;
        std::uint32_t typeSize = 1;
        std::uint32_t pixelWidth = 0;
        std::uint32_t pixelHeight = 0;
        std::uint32_t pixelDepth = 0;
        std::uint32_t layerCount = 0;
        std::uint32_t faceCount = 1;
        std::uint32_t levelCount = 1;
        std::uint32_t supercompressionScheme = 0;
        std::uint32_t dfdByteOffset = 0;
        std::uint32_t dfdByteLength = 0;
        std::uint32_t kvdByteOffset = 0;
        std::uint32_t kvdByteLength = 0;
        std::uint64_t sgdByteOffset = 0;
        std::uint64_t sgdByteLength = 0;
        std::vector<KTX2LevelIndexInfo> levels;
    };

    static KTX2HeaderInfo readKTX2Header(const std::filesystem::path& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) throw std::runtime_error("TextureLoader::readKTX2Header failed to open " + path.string());
        std::array<unsigned char, 12> id{};
        f.read(reinterpret_cast<char*>(id.data()), static_cast<std::streamsize>(id.size()));
        static constexpr unsigned char expected[12] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
        if (std::memcmp(id.data(), expected, 12) != 0) return {};
        auto read32 = [&]() -> std::uint32_t {
            std::uint32_t v = 0;
            f.read(reinterpret_cast<char*>(&v), sizeof(v));
            return v;
        };
        auto read64 = [&]() -> std::uint64_t {
            std::uint64_t v = 0;
            f.read(reinterpret_cast<char*>(&v), sizeof(v));
            return v;
        };
        KTX2HeaderInfo h;
        h.valid = true;
        h.vkFormat = read32();
        h.typeSize = read32();
        h.pixelWidth = read32();
        h.pixelHeight = read32();
        h.pixelDepth = read32();
        h.layerCount = read32();
        h.faceCount = read32();
        h.levelCount = read32();
        h.supercompressionScheme = read32();
        h.dfdByteOffset = read32();
        h.dfdByteLength = read32();
        h.kvdByteOffset = read32();
        h.kvdByteLength = read32();
        h.sgdByteOffset = read64();
        h.sgdByteLength = read64();
        h.levels.reserve(h.levelCount ? h.levelCount : 1);
        for (std::uint32_t i = 0; i < (h.levelCount ? h.levelCount : 1); ++i) {
            KTX2LevelIndexInfo li;
            li.byteOffset = read64();
            li.byteLength = read64();
            li.uncompressedByteLength = read64();
            h.levels.push_back(li);
        }
        return h;
    }

    static std::shared_ptr<Texture> loadKTX2Metadata(const std::filesystem::path& path, bool srgb = true) {
        auto header = readKTX2Header(path);
        if (!header.valid) throw std::runtime_error("TextureLoader::loadKTX2Metadata invalid KTX2 file: " + path.string());
        std::ifstream f(path, std::ios::binary);
        std::vector<char> rawBytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        std::vector<std::byte> bytes(rawBytes.size());
        if (!rawBytes.empty()) std::memcpy(bytes.data(), rawBytes.data(), rawBytes.size());
        auto t = std::make_shared<Texture>();
        t->name = path.filename().string();
        t->sourcePath = path.string();
        t->width = static_cast<int>(header.pixelWidth);
        t->height = static_cast<int>(header.pixelHeight);
        t->channels = 0;
        t->compressed = true;
        t->compressedContainer = CompressedTextureContainer::KTX2;
        t->compressionScheme = header.supercompressionScheme == 1 ? "BasisLZ" : (header.supercompressionScheme == 2 ? "Zstandard" : "None/Unknown");
        t->vkFormat = header.vkFormat;
        t->typeSize = header.typeSize;
        t->layerCount = header.layerCount;
        t->faceCount = header.faceCount;
        t->levelCount = header.levelCount ? header.levelCount : 1;
        t->supercompressionScheme = header.supercompressionScheme;
        t->format = srgb ? TextureFormat::SRGB8Alpha8 : TextureFormat::RGBA8;
        t->colorSpace = srgb ? ColorSpace::SRGB : ColorSpace::LinearSRGB;
        t->pixels = std::move(bytes);
        t->generateMipmaps = false;
        t->compressedUploadReady = header.supercompressionScheme == 0;
        t->transcodeMessage = t->compressedUploadReady ? "KTX2 contains GPU-ready level payload; GL upload can use glCompressedTexImage2D" : "KTX2 is supercompressed; BasisU transcoder required or RGBA fallback will be used";
        for (std::uint32_t mip = 0; mip < t->levelCount; ++mip) {
            CompressedTextureLevel level;
            level.level = mip;
            level.width = std::max<std::uint32_t>(1u, header.pixelWidth >> mip);
            level.height = std::max<std::uint32_t>(1u, header.pixelHeight >> mip);
            if (mip < header.levels.size()) {
                level.byteOffset = header.levels[mip].byteOffset;
                level.byteLength = header.levels[mip].byteLength;
                level.uncompressedByteLength = header.levels[mip].uncompressedByteLength;
                const std::uint64_t end = level.byteOffset + level.byteLength;
                if (end <= t->pixels.size()) {
                    auto first = t->pixels.begin() + static_cast<std::ptrdiff_t>(level.byteOffset);
                    auto last = first + static_cast<std::ptrdiff_t>(level.byteLength);
                    level.data.assign(first, last);
                }
            }
            t->compressedLevels.push_back(std::move(level));
        }
        t->markNeedsUpdate();
        return t;
    }

    static std::shared_ptr<Texture> loadKTX2(const std::filesystem::path& path, bool srgb = true) {
        return loadKTX2Metadata(path, srgb);
    }

    static bool isKTX2File(const std::filesystem::path& path) {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec)) return false;
        try { return readKTX2Header(path).valid; } catch (...) { return false; }
    }

    // Portable float-map loader for HDR-style tests. PFM is much simpler than
    // RGBE/HDR and keeps this repository dependency-free while allowing 16F/32F
    // data paths to be exercised later.
    static std::shared_ptr<Texture> loadPFMAsRGB16FPlaceholder(const std::filesystem::path& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) throw std::runtime_error("TextureLoader::loadPFM failed to open " + path.string());
        std::string magic;
        int w = 0, h = 0;
        float scale = 1.0f;
        f >> magic >> w >> h >> scale;
        f.get();
        if (magic != "PF" || w <= 0 || h <= 0) throw std::runtime_error("TextureLoader::loadPFM expected RGB PFM");
        std::vector<float> rgb(static_cast<std::size_t>(w * h * 3));
        f.read(reinterpret_cast<char*>(rgb.data()), static_cast<std::streamsize>(rgb.size() * sizeof(float)));
        auto t = std::make_shared<Texture>();
        t->name = path.filename().string();
        t->width = w;
        t->height = h;
        t->channels = 4;
        t->format = TextureFormat::RGBA16F;
        t->colorSpace = ColorSpace::LinearSRGB;
        t->mapping = TextureMapping::EquirectangularReflection;
        // Store tone-mapped bytes until GLResourceManager grows true float upload.
        t->pixels.resize(static_cast<std::size_t>(w * h * 4));
        for (int i = 0; i < w * h; ++i) {
            glm::vec3 c(rgb[static_cast<std::size_t>(i * 3 + 0)], rgb[static_cast<std::size_t>(i * 3 + 1)], rgb[static_cast<std::size_t>(i * 3 + 2)]);
            c = c / (glm::vec3(1.0f) + c);
            t->pixels[static_cast<std::size_t>(i * 4 + 0)] = std::byte(static_cast<std::uint8_t>(glm::clamp(c.r, 0.0f, 1.0f) * 255.0f));
            t->pixels[static_cast<std::size_t>(i * 4 + 1)] = std::byte(static_cast<std::uint8_t>(glm::clamp(c.g, 0.0f, 1.0f) * 255.0f));
            t->pixels[static_cast<std::size_t>(i * 4 + 2)] = std::byte(static_cast<std::uint8_t>(glm::clamp(c.b, 0.0f, 1.0f) * 255.0f));
            t->pixels[static_cast<std::size_t>(i * 4 + 3)] = std::byte(255);
        }
        t->markNeedsUpdate();
        return t;
    }
};

} // namespace THREE
