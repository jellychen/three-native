#pragma once
#include "texture/Texture.hpp"

namespace threecpp::TextureFactory {

inline std::shared_ptr<Texture> makeCheckerboard(int width = 256, int height = 256, int cells = 8, glm::vec3 a = {1.0f, 1.0f, 1.0f}, glm::vec3 b = {0.05f, 0.05f, 0.05f}) {
    auto t = std::make_shared<Texture>();
    t->width = width;
    t->height = height;
    t->channels = 4;
    t->format = TextureFormat::SRGB8Alpha8;
    t->colorSpace = ColorSpace::SRGB;
    t->wrapS = TextureWrap::Repeat;
    t->wrapT = TextureWrap::Repeat;
    t->pixels.resize(static_cast<std::size_t>(width * height * 4));
    int cellW = std::max(1, width / std::max(1, cells));
    int cellH = std::max(1, height / std::max(1, cells));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            bool odd = ((x / cellW) + (y / cellH)) & 1;
            glm::vec3 c = odd ? a : b;
            std::size_t i = static_cast<std::size_t>((y * width + x) * 4);
            t->pixels[i + 0] = std::byte(std::uint8_t(glm::clamp(c.r, 0.0f, 1.0f) * 255.0f));
            t->pixels[i + 1] = std::byte(std::uint8_t(glm::clamp(c.g, 0.0f, 1.0f) * 255.0f));
            t->pixels[i + 2] = std::byte(std::uint8_t(glm::clamp(c.b, 0.0f, 1.0f) * 255.0f));
            t->pixels[i + 3] = std::byte(255);
        }
    }
    t->markNeedsUpdate();
    return t;
}

inline std::shared_ptr<Texture> makeScalarRoughness(int width = 128, int height = 128, float value = 0.75f) {
    auto t = std::make_shared<Texture>();
    t->width = width;
    t->height = height;
    t->channels = 4;
    t->format = TextureFormat::RGBA8;
    t->colorSpace = ColorSpace::LinearSRGB;
    t->pixels.resize(static_cast<std::size_t>(width * height * 4));
    std::uint8_t v = std::uint8_t(glm::clamp(value, 0.0f, 1.0f) * 255.0f);
    for (int i = 0; i < width * height; ++i) {
        t->pixels[std::size_t(i * 4 + 0)] = std::byte(v);
        t->pixels[std::size_t(i * 4 + 1)] = std::byte(v);
        t->pixels[std::size_t(i * 4 + 2)] = std::byte(v);
        t->pixels[std::size_t(i * 4 + 3)] = std::byte(255);
    }
    t->markNeedsUpdate();
    return t;
}


inline std::shared_ptr<Texture> makeEquirectangularGradient(int width = 512, int height = 256, glm::vec3 sky = {0.55f, 0.72f, 1.0f}, glm::vec3 horizon = {1.0f, 0.86f, 0.62f}, glm::vec3 ground = {0.14f, 0.12f, 0.10f}) {
    auto t = std::make_shared<Texture>();
    t->width = width;
    t->height = height;
    t->channels = 4;
    t->format = TextureFormat::SRGB8Alpha8;
    t->colorSpace = ColorSpace::SRGB;
    t->mapping = TextureMapping::EquirectangularReflection;
    t->wrapS = TextureWrap::Repeat;
    t->wrapT = TextureWrap::ClampToEdge;
    t->pixels.resize(static_cast<std::size_t>(width * height * 4));
    for (int y = 0; y < height; ++y) {
        float v = float(y) / float(std::max(1, height - 1));
        glm::vec3 c = v < 0.5f ? glm::mix(ground, horizon, v * 2.0f) : glm::mix(horizon, sky, (v - 0.5f) * 2.0f);
        for (int x = 0; x < width; ++x) {
            float u = float(x) / float(std::max(1, width - 1));
            float sun = std::pow(std::max(0.0f, 1.0f - glm::length(glm::vec2(u - 0.08f, v - 0.72f)) * 11.0f), 12.0f);
            glm::vec3 out = glm::clamp(c + glm::vec3(1.0f, 0.72f, 0.36f) * sun, glm::vec3(0.0f), glm::vec3(1.0f));
            std::size_t i = static_cast<std::size_t>((y * width + x) * 4);
            t->pixels[i + 0] = std::byte(std::uint8_t(out.r * 255.0f));
            t->pixels[i + 1] = std::byte(std::uint8_t(out.g * 255.0f));
            t->pixels[i + 2] = std::byte(std::uint8_t(out.b * 255.0f));
            t->pixels[i + 3] = std::byte(255);
        }
    }
    t->markNeedsUpdate();
    return t;
}

inline std::shared_ptr<Texture> makeBRDFLUT(int size = 256) {
    auto t = std::make_shared<Texture>();
    t->width = size;
    t->height = size;
    t->channels = 4;
    t->format = TextureFormat::RGBA8;
    t->colorSpace = ColorSpace::LinearSRGB;
    t->generateMipmaps = false;
    // BRDF LUT has no mip chain. Using a mipmapped min filter makes the texture
    // incomplete on macOS OpenGL, which then causes the sampler to read as an
    // unloadable/zero texture. Keep it strictly non-mipmapped.
    t->minFilter = TextureFilter::Linear;
    t->magFilter = TextureFilter::Linear;
    t->wrapS = TextureWrap::ClampToEdge;
    t->wrapT = TextureWrap::ClampToEdge;
    t->pixels.resize(static_cast<std::size_t>(size * size * 4));

    auto radicalInverseVdC = [](std::uint32_t bits) {
        bits = (bits << 16u) | (bits >> 16u);
        bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
        bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
        bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
        bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
        return float(bits) * 2.3283064365386963e-10f;
    };
    auto geometrySchlickGGX = [](float NoV, float roughness) {
        float a = roughness;
        float k = (a * a) / 2.0f;
        return NoV / std::max(NoV * (1.0f - k) + k, 1e-6f);
    };
    auto geometrySmith = [&](float NoV, float NoL, float roughness) {
        return geometrySchlickGGX(NoV, roughness) * geometrySchlickGGX(NoL, roughness);
    };
    auto importanceSampleGGX = [](glm::vec2 xi, float roughness) {
        float a = roughness * roughness;
        float phi = glm::two_pi<float>() * xi.x;
        float cosTheta = std::sqrt((1.0f - xi.y) / std::max(1.0f + (a * a - 1.0f) * xi.y, 1e-6f));
        float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
        return glm::vec3(std::cos(phi) * sinTheta, std::sin(phi) * sinTheta, cosTheta);
    };

    constexpr int sampleCount = 128;
    for (int y = 0; y < size; ++y) {
        float roughness = glm::clamp((float(y) + 0.5f) / float(size), 0.0f, 1.0f);
        for (int x = 0; x < size; ++x) {
            float NoV = glm::clamp((float(x) + 0.5f) / float(size), 0.001f, 1.0f);
            glm::vec3 V(std::sqrt(std::max(0.0f, 1.0f - NoV * NoV)), 0.0f, NoV);
            float A = 0.0f;
            float B = 0.0f;
            for (int i = 0; i < sampleCount; ++i) {
                glm::vec2 xi(float(i) / float(sampleCount), radicalInverseVdC(static_cast<std::uint32_t>(i)));
                glm::vec3 H = importanceSampleGGX(xi, roughness);
                glm::vec3 L = glm::normalize(2.0f * glm::dot(V, H) * H - V);
                float NoL = std::max(L.z, 0.0f);
                float NoH = std::max(H.z, 0.0f);
                float VoH = std::max(glm::dot(V, H), 0.0f);
                if (NoL > 0.0f) {
                    float G = geometrySmith(NoV, NoL, roughness);
                    float GVis = (G * VoH) / std::max(NoH * NoV, 1e-6f);
                    float Fc = std::pow(1.0f - VoH, 5.0f);
                    A += (1.0f - Fc) * GVis;
                    B += Fc * GVis;
                }
            }
            A /= float(sampleCount);
            B /= float(sampleCount);
            std::size_t i = static_cast<std::size_t>((y * size + x) * 4);
            t->pixels[i + 0] = std::byte(std::uint8_t(glm::clamp(A, 0.0f, 1.0f) * 255.0f));
            t->pixels[i + 1] = std::byte(std::uint8_t(glm::clamp(B, 0.0f, 1.0f) * 255.0f));
            t->pixels[i + 2] = std::byte(0);
            t->pixels[i + 3] = std::byte(255);
        }
    }
    t->markNeedsUpdate();
    return t;
}


inline std::shared_ptr<Texture> makeStudioHDRI(int width = 1024, int height = 512) {
    auto t = std::make_shared<Texture>();
    t->width = width;
    t->height = height;
    t->channels = 4;
    t->format = TextureFormat::SRGB8Alpha8;
    t->colorSpace = ColorSpace::SRGB;
    t->mapping = TextureMapping::EquirectangularReflection;
    t->wrapS = TextureWrap::Repeat;
    t->wrapT = TextureWrap::ClampToEdge;
    // High-contrast procedural HDRI-style environment encoded as regular sRGB
    // pixels so the current lightweight GL upload path can run on OpenGL 3.3.
    t->pixels.resize(static_cast<std::size_t>(width * height * 4));
    for (int y = 0; y < height; ++y) {
        float v = float(y) / float(std::max(1, height - 1));
        for (int x = 0; x < width; ++x) {
            float u = float(x) / float(std::max(1, width - 1));
            glm::vec3 base = glm::mix(glm::vec3(0.04f,0.045f,0.055f), glm::vec3(0.45f,0.58f,0.85f), glm::smoothstep(0.28f, 0.95f, v));
            auto spot = [&](float su, float sv, float power, glm::vec3 color) {
                glm::vec2 d(u - su, v - sv);
                d.x = std::min(std::abs(d.x), 1.0f - std::abs(d.x));
                float g = std::exp(-glm::dot(d, d) * power);
                return color * g;
            };
            glm::vec3 c = base;
            c += spot(0.08f, 0.70f, 520.0f, {3.0f, 2.25f, 1.35f});
            c += spot(0.62f, 0.58f, 360.0f, {0.8f, 1.15f, 2.0f});
            c += spot(0.88f, 0.38f, 450.0f, {1.7f, 0.75f, 0.35f});
            c = glm::clamp(c / (glm::vec3(1.0f) + c * 0.35f), glm::vec3(0.0f), glm::vec3(1.0f));
            std::size_t i = static_cast<std::size_t>((y * width + x) * 4);
            t->pixels[i + 0] = std::byte(std::uint8_t(c.r * 255.0f));
            t->pixels[i + 1] = std::byte(std::uint8_t(c.g * 255.0f));
            t->pixels[i + 2] = std::byte(std::uint8_t(c.b * 255.0f));
            t->pixels[i + 3] = std::byte(255);
        }
    }
    t->markNeedsUpdate();
    return t;
}

inline std::shared_ptr<Texture> makeAlphaCircle(int width = 256, int height = 256) {
    auto t = std::make_shared<Texture>();
    t->width = width;
    t->height = height;
    t->channels = 4;
    t->format = TextureFormat::RGBA8;
    t->colorSpace = ColorSpace::LinearSRGB;
    t->pixels.resize(static_cast<std::size_t>(width * height * 4));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            glm::vec2 p((float(x) + 0.5f) / width * 2.0f - 1.0f, (float(y) + 0.5f) / height * 2.0f - 1.0f);
            float a = glm::smoothstep(0.62f, 0.55f, glm::length(p));
            std::uint8_t q = std::uint8_t(glm::clamp(a, 0.0f, 1.0f) * 255.0f);
            std::size_t i = static_cast<std::size_t>((y * width + x) * 4);
            t->pixels[i + 0] = std::byte(255);
            t->pixels[i + 1] = std::byte(255);
            t->pixels[i + 2] = std::byte(255);
            t->pixels[i + 3] = std::byte(q);
        }
    }
    t->markNeedsUpdate();
    return t;
}

} // namespace threecpp::TextureFactory
