#pragma once
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace THREE {

namespace MathUtils {

inline float clamp(float value, float min, float max) {
    return std::max(min, std::min(max, value));
}

inline float lerp(float x, float y, float t) {
    return x + (y - x) * t;
}

inline float smoothstep(float x, float edge0, float edge1) {
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

inline float smootherstep(float x, float edge0, float edge1) {
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

inline float rand(float seed) {
    return std::fmod(std::sin(seed * 12.9898f + 78.233f) * 43758.5453f, 1.0f);
}

inline float degToRad(float degrees) {
    return degrees * 0.01745329252f;
}

inline float radToDeg(float radians) {
    return radians * 57.295779513f;
}

inline float mapLinear(float x, float a1, float a2, float b1, float b2) {
    return b1 + (x - a1) * (b2 - b1) / (a2 - a1);
}

inline float inverseLerp(float x, float a, float b) {
    if (a == b) return 0.0f;
    return clamp((x - a) / (b - a), 0.0f, 1.0f);
}

inline float damp(float x, float y, float lambda, float dt) {
    return lerp(x, y, 1.0f - std::exp(-lambda * dt));
}

inline float pingpong(float x, float length) {
    float t = std::fmod(x, length * 2.0f);
    if (t > length) t = length * 2.0f - t;
    // Handle negative values
    if (t < 0) t = -t;
    return t;
}

inline float ceilPowerOfTwo(float value) {
    if (value <= 0) return 1.0f;
    return std::pow(2.0f, std::ceil(std::log2(value)));
}

inline int ceilPowerOfTwoInt(int value) {
    if (value <= 0) return 1;
    int v = value;
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v + 1;
}

inline float floorPowerOfTwo(float value) {
    if (value <= 0) return 1.0f;
    return std::pow(2.0f, std::floor(std::log2(value)));
}

inline int floorPowerOfTwoInt(int value) {
    if (value <= 0) return 1;
    int v = value;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v - (v >> 1);
}

// Euclidian modulo that works correctly with negative values
inline float euclideanModulo(float n, float m) {
    float r = std::fmod(n, m);
    return r < 0 ? r + m : r;
}

// Generate a random float in [0, 1) using integer seed
inline float seededRandom(uint32_t& seed) {
    seed = (seed * 1664525u + 1013904223u) & 0xFFFFFFFFu;
    return static_cast<float>(seed) / 4294967296.0f;
}

} // namespace MathUtils

} // namespace THREE
