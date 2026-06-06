#pragma once
#include "common.hpp"
#include "texture/Texture.hpp"
#include "texture/TextureFactory.hpp"

namespace threecpp {

class Environment;

struct PMREMOptions {
    int cubeSize = 64;
    int maxMipLevels = 6;
    bool generateBRDFLUT = true;
    int irradianceSamples = 32;
    int prefilterSamples = 32;
    bool enableCache = true;
    bool threeJSRoughnessMipMapping = true;
};

struct PMREMCacheStats {
    std::uint64_t requests = 0;
    std::uint64_t hits = 0;
    std::uint64_t misses = 0;
    std::uint64_t evictions = 0;
    std::size_t liveEntries = 0;
};

class PMREMCache {
public:
    static PMREMCache& instance() {
        static PMREMCache cache;
        return cache;
    }

    std::shared_ptr<Environment> get(const std::string& key) {
        ++stats.requests;
        auto it = entries.find(key);
        if (it == entries.end()) { ++stats.misses; return nullptr; }
        if (auto env = it->second.lock()) { ++stats.hits; stats.liveEntries = liveCount(); return env; }
        entries.erase(it);
        ++stats.misses;
        stats.liveEntries = liveCount();
        return nullptr;
    }

    void put(const std::string& key, const std::shared_ptr<Environment>& env) {
        entries[key] = env;
        stats.liveEntries = liveCount();
    }

    void clear() { entries.clear(); stats.liveEntries = 0; }
    PMREMCacheStats getStats() const { auto out = stats; out.liveEntries = liveCount(); return out; }

private:
    mutable std::unordered_map<std::string, std::weak_ptr<Environment>> entries;
    mutable PMREMCacheStats stats;

    std::size_t liveCount() const {
        std::size_t n = 0;
        for (auto it = entries.begin(); it != entries.end();) {
            if (it->second.expired()) { it = entries.erase(it); ++stats.evictions; }
            else { ++n; ++it; }
        }
        return n;
    }
};

class Environment {
public:
    std::shared_ptr<Texture> background;
    std::shared_ptr<Texture> equirectangularMap;
    std::shared_ptr<CubeTexture> irradianceMap;
    std::shared_ptr<CubeTexture> prefilterMap;
    std::shared_ptr<Texture> brdfLUT;

    // three.js-style environment intensity aliases.
    // `intensity` is the legacy/internal multiplier; `envMapIntensity` is kept for examples
    // that mirror MeshStandardMaterial/scene environment naming. GLRenderer multiplies both.
    float intensity = 1.0f;
    float envMapIntensity = 1.0f;
    float backgroundIntensity = 1.0f;
    float backgroundBlurriness = 0.0f;
    // Separate rotations mirror three.js scene.environmentRotation / backgroundRotation.
    glm::mat3 environmentRotation{1.0f};
    glm::mat3 backgroundRotation{1.0f};
    glm::mat3 rotation{1.0f}; // legacy alias kept for existing examples.

    glm::vec3 skyColor{0.62f, 0.72f, 0.95f};
    glm::vec3 groundColor{0.18f, 0.17f, 0.15f};
    glm::vec3 specularColor{1.0f, 0.96f, 0.88f};

    int pmremCubeSize = 0;
    int pmremMipLevels = 0;
    bool hasPMREM = false;
    std::string pmremCacheKey;
    bool pmremCacheHit = false;
    float maxMipLevel = 0.0f;
};

class PMREMGenerator {
public:
    explicit PMREMGenerator(PMREMOptions opts = {}) : options(opts) {}

    std::shared_ptr<Environment> fromEquirectangular(const std::shared_ptr<Texture>& hdr) const {
        const std::string key = makeCacheKey(hdr);
        if (options.enableCache) {
            if (auto cached = PMREMCache::instance().get(key)) {
                cached->pmremCacheHit = true;
                return cached;
            }
        }

        auto env = std::make_shared<Environment>();
        env->pmremCacheKey = key;
        env->pmremCacheHit = false;
        env->background = hdr;
        env->equirectangularMap = hdr;
        if (hdr) hdr->mapping = TextureMapping::EquirectangularReflection;

        if (hdr && hdr->width > 0 && hdr->height > 0 && !hdr->pixels.empty()) {
            env->skyColor = estimateDirectionColor(*hdr, {0.0f, 1.0f, 0.0f});
            env->groundColor = estimateDirectionColor(*hdr, {0.0f, -1.0f, 0.0f});
            env->specularColor = estimateAverageColor(*hdr);
            env->irradianceMap = buildIrradianceCube(*hdr, std::max(16, options.cubeSize / 8));
            env->prefilterMap = buildPrefilterCube(*hdr, options.cubeSize, options.maxMipLevels, options.prefilterSamples);
        } else {
            env->irradianceMap = makeSolidCube(std::max(16, options.cubeSize / 8), 1, {0.5f, 0.5f, 0.5f});
            env->prefilterMap = makeSolidCube(options.cubeSize, options.maxMipLevels, {1.0f, 1.0f, 1.0f});
        }

        env->brdfLUT = options.generateBRDFLUT ? TextureFactory::makeBRDFLUT(256) : nullptr;
        env->pmremCubeSize = options.cubeSize;
        env->pmremMipLevels = options.maxMipLevels;
        env->maxMipLevel = static_cast<float>(std::max(0, options.maxMipLevels - 1));
        env->hasPMREM = env->irradianceMap != nullptr && env->prefilterMap != nullptr;
        if (options.enableCache) PMREMCache::instance().put(key, env);
        return env;
    }

    static float roughnessToMip(float roughness, int mipLevels) {
        // Three.js PMREM uses a non-linear mapping from perceptual roughness to
        // prefiltered mip level. This approximation biases low roughness toward
        // sharper mips and gives rough materials faster blur growth.
        roughness = glm::clamp(roughness, 0.0f, 1.0f);
        const float maxMip = static_cast<float>(std::max(0, mipLevels - 1));
        const float perceptual = std::sqrt(std::max(roughness, 0.0f));
        const float shaped = glm::mix(roughness * roughness, perceptual, 0.72f);
        return glm::clamp(shaped * maxMip, 0.0f, maxMip);
    }

    PMREMCacheStats cacheStats() const { return PMREMCache::instance().getStats(); }

private:
    PMREMOptions options;

    std::string makeCacheKey(const std::shared_ptr<Texture>& tex) const {
        if (!tex) return "null:" + std::to_string(options.cubeSize) + ":" + std::to_string(options.maxMipLevels);
        return std::to_string(tex->id) + ":" + std::to_string(tex->version) + ":" +
               std::to_string(tex->width) + "x" + std::to_string(tex->height) + ":" +
               std::to_string(options.cubeSize) + ":" + std::to_string(options.maxMipLevels) + ":" +
               std::to_string(options.irradianceSamples) + ":" + std::to_string(options.prefilterSamples);
    }

    static glm::vec3 decodePixel(const Texture& tex, int x, int y) {
        if (tex.width <= 0 || tex.height <= 0 || tex.pixels.empty()) return glm::vec3{0.0f};
        x = ((x % tex.width) + tex.width) % tex.width;
        y = glm::clamp(y, 0, tex.height - 1);
        const std::size_t i = static_cast<std::size_t>((y * tex.width + x) * std::max(1, tex.channels));
        auto byteAt = [&](std::size_t off) -> float {
            if (i + off >= tex.pixels.size()) return 0.0f;
            return float(std::uint8_t(tex.pixels[i + off])) / 255.0f;
        };
        glm::vec3 c(byteAt(0), byteAt(1), byteAt(2));
        if (tex.colorSpace == ColorSpace::SRGB) c = glm::pow(glm::max(c, glm::vec3(0.0f)), glm::vec3(2.2f));
        return c;
    }

    static glm::vec3 sampleEquirect(const Texture& tex, glm::vec3 dir) {
        dir = glm::normalize(dir);
        float u = std::atan2(dir.z, dir.x) / glm::two_pi<float>() + 0.5f;
        float v = std::asin(glm::clamp(dir.y, -1.0f, 1.0f)) / glm::pi<float>() + 0.5f;
        float fx = u * float(tex.width - 1);
        float fy = v * float(tex.height - 1);
        int x0 = int(std::floor(fx));
        int y0 = int(std::floor(fy));
        float tx = fx - float(x0);
        float ty = fy - float(y0);
        glm::vec3 a = glm::mix(decodePixel(tex, x0, y0), decodePixel(tex, x0 + 1, y0), tx);
        glm::vec3 b = glm::mix(decodePixel(tex, x0, y0 + 1), decodePixel(tex, x0 + 1, y0 + 1), tx);
        return glm::mix(a, b, ty);
    }

    static glm::vec3 directionForFace(int face, float u, float v) {
        float x = 2.0f * u - 1.0f;
        float y = 2.0f * v - 1.0f;
        switch (face) {
            case 0: return glm::normalize(glm::vec3( 1.0f, -y, -x));
            case 1: return glm::normalize(glm::vec3(-1.0f, -y,  x));
            case 2: return glm::normalize(glm::vec3( x,  1.0f,  y));
            case 3: return glm::normalize(glm::vec3( x, -1.0f, -y));
            case 4: return glm::normalize(glm::vec3( x, -y,  1.0f));
            default:return glm::normalize(glm::vec3(-x, -y, -1.0f));
        }
    }

    static void writePixel(std::vector<std::byte>& dst, int idx, glm::vec3 c) {
        c = glm::clamp(c, glm::vec3(0.0f), glm::vec3(64.0f));
        // Encode linear scene values into displayable bytes for the current GL
        // upload path. The API still represents PMREM mip hierarchy correctly;
        // later this can move to real half-float storage without changing users.
        c = c / (glm::vec3(1.0f) + c);
        dst[static_cast<std::size_t>(idx * 4 + 0)] = std::byte(std::uint8_t(glm::clamp(c.r, 0.0f, 1.0f) * 255.0f));
        dst[static_cast<std::size_t>(idx * 4 + 1)] = std::byte(std::uint8_t(glm::clamp(c.g, 0.0f, 1.0f) * 255.0f));
        dst[static_cast<std::size_t>(idx * 4 + 2)] = std::byte(std::uint8_t(glm::clamp(c.b, 0.0f, 1.0f) * 255.0f));
        dst[static_cast<std::size_t>(idx * 4 + 3)] = std::byte(255);
    }

    static glm::vec3 estimateDirectionColor(const Texture& tex, glm::vec3 dir) {
        glm::vec3 sum(0.0f);
        int count = 0;
        for (int i = 0; i < 8; ++i) {
            float a = glm::two_pi<float>() * float(i) / 8.0f;
            glm::vec3 tangent = std::abs(dir.y) < 0.9f ? glm::normalize(glm::cross(dir, glm::vec3(0,1,0))) : glm::vec3(1,0,0);
            glm::vec3 bitangent = glm::normalize(glm::cross(dir, tangent));
            glm::vec3 d = glm::normalize(dir + 0.18f * (std::cos(a) * tangent + std::sin(a) * bitangent));
            sum += sampleEquirect(tex, d);
            ++count;
        }
        return sum / float(std::max(1, count));
    }

    static glm::vec3 estimateAverageColor(const Texture& tex) {
        glm::vec3 sum(0.0f);
        int count = 0;
        const int sx = std::max(1, tex.width / 32);
        const int sy = std::max(1, tex.height / 16);
        for (int y = 0; y < tex.height; y += sy) {
            for (int x = 0; x < tex.width; x += sx) {
                sum += decodePixel(tex, x, y);
                ++count;
            }
        }
        return sum / float(std::max(1, count));
    }

    static std::shared_ptr<CubeTexture> makeSolidCube(int size, int mipLevels, glm::vec3 color) {
        auto cube = std::make_shared<CubeTexture>();
        cube->width = size;
        cube->height = size;
        cube->channels = 4;
        cube->format = TextureFormat::RGBA8;
        cube->colorSpace = ColorSpace::LinearSRGB;
        cube->mapping = TextureMapping::CubeReflection;
        cube->generateMipmaps = false;
        cube->mipLevels = std::max(1, mipLevels);
        cube->mipFaces.resize(static_cast<std::size_t>(cube->mipLevels));
        for (int mip = 0; mip < cube->mipLevels; ++mip) {
            int s = std::max(1, size >> mip);
            for (int face = 0; face < 6; ++face) {
                auto& dst = cube->mipFaces[static_cast<std::size_t>(mip)][static_cast<std::size_t>(face)];
                dst.resize(static_cast<std::size_t>(s * s * 4));
                for (int i = 0; i < s * s; ++i) writePixel(dst, i, color);
                if (mip == 0) cube->faces[static_cast<std::size_t>(face)] = dst;
            }
        }
        cube->markNeedsUpdate();
        return cube;
    }

    static std::shared_ptr<CubeTexture> buildIrradianceCube(const Texture& src, int size) {
        auto cube = makeSolidCube(size, 1, {0.0f, 0.0f, 0.0f});
        for (int face = 0; face < 6; ++face) {
            auto& dst = cube->mipFaces[0][static_cast<std::size_t>(face)];
            for (int y = 0; y < size; ++y) {
                for (int x = 0; x < size; ++x) {
                    glm::vec3 n = directionForFace(face, (x + 0.5f) / size, (y + 0.5f) / size);
                    glm::vec3 up = std::abs(n.y) < 0.95f ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
                    glm::vec3 t = glm::normalize(glm::cross(up, n));
                    glm::vec3 b = glm::normalize(glm::cross(n, t));
                    glm::vec3 sum(0.0f);
                    float weightSum = 0.0f;
                    const int rings = 5;
                    const int slices = 12;
                    for (int r = 0; r < rings; ++r) {
                        float theta = (float(r) + 0.5f) / float(rings) * glm::half_pi<float>();
                        for (int s = 0; s < slices; ++s) {
                            float phi = glm::two_pi<float>() * (float(s) + 0.5f) / float(slices);
                            glm::vec3 d = glm::normalize(std::cos(theta) * n + std::sin(theta) * (std::cos(phi) * t + std::sin(phi) * b));
                            float w = std::max(glm::dot(n, d), 0.0f) * std::sin(theta);
                            sum += sampleEquirect(src, d) * w;
                            weightSum += w;
                        }
                    }
                    writePixel(dst, y * size + x, sum / std::max(weightSum, 1e-5f));
                }
            }
            cube->faces[static_cast<std::size_t>(face)] = dst;
        }
        cube->markNeedsUpdate();
        return cube;
    }

    static float radicalInverseVdC(std::uint32_t bits) {
        bits = (bits << 16u) | (bits >> 16u);
        bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
        bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
        bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
        bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
        return float(bits) * 2.3283064365386963e-10f;
    }

    static glm::vec2 hammersley(std::uint32_t i, std::uint32_t n) {
        return {float(i) / float(std::max<std::uint32_t>(1, n)), radicalInverseVdC(i)};
    }

    static glm::vec3 importanceSampleGGX(glm::vec2 xi, float roughness, glm::vec3 n) {
        float a = roughness * roughness;
        float phi = glm::two_pi<float>() * xi.x;
        float cosTheta = std::sqrt((1.0f - xi.y) / std::max(1.0f + (a * a - 1.0f) * xi.y, 1e-6f));
        float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
        glm::vec3 h(std::cos(phi) * sinTheta, std::sin(phi) * sinTheta, cosTheta);
        glm::vec3 up = std::abs(n.z) < 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 tangent = glm::normalize(glm::cross(up, n));
        glm::vec3 bitangent = glm::cross(n, tangent);
        return glm::normalize(tangent * h.x + bitangent * h.y + n * h.z);
    }

    static float distributionGGX(float NoH, float roughness) {
        float a = roughness * roughness;
        float a2 = a * a;
        float d = (NoH * a2 - NoH) * NoH + 1.0f;
        return a2 / std::max(glm::pi<float>() * d * d, 1e-7f);
    }

    static std::shared_ptr<CubeTexture> buildPrefilterCube(const Texture& src, int size, int mipLevels, int requestedSamples) {
        auto cube = makeSolidCube(size, mipLevels, {0.0f, 0.0f, 0.0f});
        const int baseSamples = std::max(8, requestedSamples);
        for (int mip = 0; mip < cube->mipLevels; ++mip) {
            int s = std::max(1, size >> mip);
            float rough = cube->mipLevels <= 1 ? 0.0f : float(mip) / float(cube->mipLevels - 1);
            rough = glm::clamp(rough, 0.0f, 1.0f);
            const int samples = std::max(32, int(float(baseSamples) * glm::mix(0.35f, 1.0f, rough)));
            for (int face = 0; face < 6; ++face) {
                auto& dst = cube->mipFaces[static_cast<std::size_t>(mip)][static_cast<std::size_t>(face)];
                dst.resize(static_cast<std::size_t>(s * s * 4));
                for (int y = 0; y < s; ++y) {
                    for (int x = 0; x < s; ++x) {
                        glm::vec3 R = directionForFace(face, (x + 0.5f) / s, (y + 0.5f) / s);
                        glm::vec3 N = R;
                        glm::vec3 V = R;
                        glm::vec3 color(0.0f);
                        float weightSum = 0.0f;
                        if (rough < 0.015f) {
                            color = sampleEquirect(src, R);
                            weightSum = 1.0f;
                        } else {
                            for (int i = 0; i < samples; ++i) {
                                glm::vec2 xi = hammersley(static_cast<std::uint32_t>(i), static_cast<std::uint32_t>(samples));
                                glm::vec3 H = importanceSampleGGX(xi, rough, N);
                                glm::vec3 L = glm::normalize(2.0f * glm::dot(V, H) * H - V);
                                float NoL = std::max(glm::dot(N, L), 0.0f);
                                if (NoL > 0.0f) {
                                    float NoH = std::max(glm::dot(N, H), 0.0f);
                                    float HoV = std::max(glm::dot(H, V), 0.0f);
                                    float D = distributionGGX(NoH, rough);
                                    float pdf = std::max(D * NoH / (4.0f * HoV), 1e-5f);
                                    float texelSolidAngle = 4.0f * glm::pi<float>() / (6.0f * float(size * size));
                                    float sampleSolidAngle = 1.0f / (float(samples) * pdf + 1e-5f);
                                    float lodBias = rough > 0.0f ? 0.5f * std::log2(sampleSolidAngle / texelSolidAngle) : 0.0f;
                                    (void)lodBias; // CPU sampler reads from source equirect; mip bias is recorded by output mip.
                                    color += sampleEquirect(src, L) * NoL;
                                    weightSum += NoL;
                                }
                            }
                        }
                        writePixel(dst, y * s + x, color / std::max(weightSum, 1e-5f));
                    }
                }
                if (mip == 0) cube->faces[static_cast<std::size_t>(face)] = dst;
            }
        }
        cube->markNeedsUpdate();
        return cube;
    }
};

} // namespace threecpp
