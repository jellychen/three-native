#pragma once
#include <string>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <unordered_map>
#include <glm/glm.hpp>
#include "math/math-utils.h"

namespace THREE {

inline float srgbToLinear(float c) {
    if (c <= 0.04045f) return c / 12.92f;
    return std::pow((c + 0.055f) / 1.055f, 2.4f);
}

inline float linearToSrgb(float c) {
    if (c <= 0.0031308f) return c * 12.92f;
    return 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
}

class Color {
public:
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;

    Color() = default;
    Color(float r, float g, float b);

    explicit Color(uint32_t hex);
    explicit Color(const std::string& style);

    Color& set(float r_, float g_, float b_);
    Color& set(const Color& c);

    Color& setScalar(float s);

    Color& setHex(uint32_t hex);

    Color& setRGB(float r_, float g_, float b_);

    Color& setHSL(float h, float s, float l);

    uint32_t getHex() const;

    std::string getHexString() const;

    void getHSL(float& h, float& s, float& l) const;

    // CSS-style color name matching (X11 color names)
    static uint32_t nameToHex(const std::string& name);

    Color& setStyle(const std::string& style);

    // Operations
    Color& add(const Color& c);
    Color& addColors(const Color& a, const Color& b);
    Color& addScalar(float s);
    Color& sub(const Color& c);
    Color& multiply(const Color& c);
    Color& multiplyScalar(float s);
    Color& lerp(const Color& c, float alpha);
    Color& lerpColors(const Color& c1, const Color& c2, float alpha);

    Color& copy(const Color& c);

    // Conversion to/from glm::vec3 (for compatibility with existing codebase)
    glm::vec3 toVec3() const;
    static Color fromVec3(const glm::vec3& v);

    bool equals(const Color& c) const;

    std::string toString() const;

    Color clone() const;

    // Named color constants
    static Color white();
    static Color black();
    static Color red();
    static Color green();
    static Color blue();
    static Color yellow();
    static Color cyan();
    static Color magenta();
    static Color orange();
    static Color grey();
    static Color transparent();
};

} // namespace THREE
