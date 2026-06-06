#pragma once
#include "common.hpp"
#include "texture/Texture.hpp"

namespace threecpp {

enum class Side { FrontSide, BackSide, DoubleSide };
enum class Blending { None, Normal, Additive, Subtractive, Multiply, Custom };
enum class MaterialType {
    MeshBasic,
    MeshLambert,
    MeshPhong,
    MeshStandard,
    MeshPhysical,
    LineBasic,
    LineDashed,
    FatLine,
    Points,
    Sprite,
    Shader,
    Depth,
    Distance
};

enum class ToneMapping { None, Linear, Reinhard, Cineon, ACESFilmic };
enum class CombineOperation { Multiply, Mix, Add };

class Material {
public:
    ObjectId id = next_object_id();
    std::string name;
    MaterialType type = MaterialType::MeshBasic;

    bool transparent = false;
    float opacity = 1.0f;
    float alphaTest = 0.0f;

    bool depthTest = true;
    bool depthWrite = true;
    bool colorWrite = true;
    bool stencilWrite = false;
    bool premultipliedAlpha = false;
    bool toneMapped = true;

    bool vertexColors = false;
    bool fog = false;
    bool flatShading = false;
    bool wireframe = false;
    bool polygonOffset = false;
    float polygonOffsetFactor = 0.0f;
    float polygonOffsetUnits = 0.0f;

    Side side = Side::FrontSide;
    Blending blending = Blending::Normal;

    bool needsUpdate = true;
    std::uint64_t version = 0;

    void markNeedsUpdate() { needsUpdate = true; ++version; }
    virtual ~Material() = default;
};

class MeshBasicMaterial : public Material {
public:
    glm::vec3 color{1.0f};
    std::shared_ptr<Texture> map;
    std::shared_ptr<Texture> alphaMap;
    std::shared_ptr<Texture> aoMap;
    std::shared_ptr<Texture> lightMap;
    std::shared_ptr<Texture> envMap;
    float reflectivity = 1.0f;
    CombineOperation combine = CombineOperation::Multiply;
    MeshBasicMaterial() { type = MaterialType::MeshBasic; }
};

class MeshLambertMaterial : public Material {
public:
    glm::vec3 color{1.0f};
    glm::vec3 emissive{0.0f};
    float emissiveIntensity = 1.0f;
    std::shared_ptr<Texture> map;
    std::shared_ptr<Texture> emissiveMap;
    std::shared_ptr<Texture> aoMap;
    std::shared_ptr<Texture> lightMap;
    float lightMapIntensity = 1.0f;
    MeshLambertMaterial() { type = MaterialType::MeshLambert; }
};

class MeshPhongMaterial : public Material {
public:
    glm::vec3 color{1.0f};
    glm::vec3 specular{0.067f};
    float shininess = 30.0f;
    glm::vec3 emissive{0.0f};
    float emissiveIntensity = 1.0f;
    std::shared_ptr<Texture> map;
    std::shared_ptr<Texture> normalMap;
    std::shared_ptr<Texture> specularMap;
    std::shared_ptr<Texture> emissiveMap;
    std::shared_ptr<Texture> lightMap;
    float lightMapIntensity = 1.0f;
    MeshPhongMaterial() { type = MaterialType::MeshPhong; }
};

class MeshStandardMaterial : public Material {
public:
    glm::vec3 color{1.0f};
    float roughness = 1.0f;
    float metalness = 0.0f;
    glm::vec3 emissive{0.0f};
    float emissiveIntensity = 1.0f;

    // three.js-compatible PBR texture slots. Color/emissive maps are sampled
    // as sRGB by convention; scalar maps stay in linear space.
    std::shared_ptr<Texture> map;
    std::shared_ptr<Texture> alphaMap;
    std::shared_ptr<Texture> normalMap;
    std::shared_ptr<Texture> bumpMap;
    std::shared_ptr<Texture> displacementMap;
    std::shared_ptr<Texture> roughnessMap;
    std::shared_ptr<Texture> metalnessMap;
    std::shared_ptr<Texture> aoMap;
    std::shared_ptr<Texture> lightMap;
    std::shared_ptr<Texture> emissiveMap;

    glm::vec2 normalScale{1.0f, 1.0f};
    float bumpScale = 1.0f;
    float displacementScale = 1.0f;
    float displacementBias = 0.0f;
    float aoMapIntensity = 1.0f;
    float lightMapIntensity = 1.0f;
    float envMapIntensity = 1.0f;

    // three.js-compatible scalar texture channel controls. Assimp/glTF can
    // set these when an extension or format does not follow the default
    // glTF metallicRoughness convention. Defaults match three.js/glTF:
    // roughness=G, metalness=B, ao=R, alpha=G/R depending on texture type.
    TextureChannel roughnessChannel = TextureChannel::G;
    TextureChannel metalnessChannel = TextureChannel::B;
    TextureChannel aoChannel = TextureChannel::R;
    TextureChannel alphaChannel = TextureChannel::G;

    MeshStandardMaterial() { type = MaterialType::MeshStandard; }
};

class MeshPhysicalMaterial : public MeshStandardMaterial {
public:
    // Mirrors the public shape of three.js MeshPhysicalMaterial as closely as
    // possible while keeping v0.5 forward-renderer friendly. More expensive
    // effects such as true transmission framebuffer sampling are staged behind
    // the same material parameters.
    float ior = 1.5f;
    float reflectivity = 0.5f;

    float transmission = 0.0f;
    std::shared_ptr<Texture> transmissionMap;
    float thickness = 0.0f;
    std::shared_ptr<Texture> thicknessMap;
    float attenuationDistance = std::numeric_limits<float>::infinity();
    glm::vec3 attenuationColor{1.0f};

    float specularIntensity = 1.0f;
    glm::vec3 specularColor{1.0f};
    std::shared_ptr<Texture> specularIntensityMap;
    std::shared_ptr<Texture> specularColorMap;
    std::shared_ptr<Texture> specularMap; // legacy convenience slot; maps to specular intensity when imported from non-glTF assets.

    float clearcoat = 0.0f;
    float clearcoatRoughness = 0.0f;
    glm::vec2 clearcoatNormalScale{1.0f, 1.0f};
    std::shared_ptr<Texture> clearcoatMap;
    std::shared_ptr<Texture> clearcoatRoughnessMap;
    std::shared_ptr<Texture> clearcoatNormalMap;

    float sheen = 0.0f;
    glm::vec3 sheenColor{0.0f};
    float sheenRoughness = 1.0f;
    std::shared_ptr<Texture> sheenColorMap;
    std::shared_ptr<Texture> sheenRoughnessMap;

    float iridescence = 0.0f;
    float iridescenceIOR = 1.3f;
    float iridescenceThicknessMinimum = 100.0f;
    float iridescenceThicknessMaximum = 400.0f;
    std::shared_ptr<Texture> iridescenceMap;
    std::shared_ptr<Texture> iridescenceThicknessMap;

    float anisotropy = 0.0f;
    float anisotropyRotation = 0.0f;
    std::shared_ptr<Texture> anisotropyMap;

    // three.js r168+ compatibility knobs. Dispersion is used by transmission
    // as a cheap chromatic refraction approximation in the forward path.
    float dispersion = 0.0f;
    bool forceSinglePass = false;

    MeshPhysicalMaterial() { type = MaterialType::MeshPhysical; }
};

class LineBasicMaterial : public Material {
public:
    glm::vec3 color{1.0f};
    float linewidth = 1.0f;
    std::shared_ptr<Texture> map;
    LineBasicMaterial() { type = MaterialType::LineBasic; }
};

class LineDashedMaterial : public LineBasicMaterial {
public:
    float scale = 1.0f;
    float dashSize = 3.0f;
    float gapSize = 1.0f;
    LineDashedMaterial() { type = MaterialType::LineDashed; }
};

class PointsMaterial : public Material {
public:
    glm::vec3 color{1.0f};
    float size = 8.0f;
    // three.js-style scale used by sizeAttenuation. Renderer still exposes
    // pointSize directly but examples can now tune the attenuation curve.
    float scale = 300.0f;
    bool sizeAttenuation = true;
    std::shared_ptr<Texture> map;
    std::shared_ptr<Texture> alphaMap;
    PointsMaterial() { type = MaterialType::Points; }
};

class FatLineMaterial : public Material {
public:
    glm::vec3 color{1.0f};
    float linewidth = 4.0f;
    bool worldUnits = false;
    // Optional Line2/LineSegments2-style controls. worldUnits keeps width in
    // world space; screen-space mode uses resolution and linewidth in pixels.
    bool trimNearPlane = true;
    glm::vec2 resolution{1.0f};
    bool dashed = false;
    float dashScale = 1.0f;
    float dashSize = 1.0f;
    float gapSize = 1.0f;
    bool alphaToCoverage = false;
    FatLineMaterial() { type = MaterialType::FatLine; side = Side::DoubleSide; }
};

class SpriteMaterial : public Material {
public:
    glm::vec3 color{1.0f};
    float rotation = 0.0f;
    glm::vec2 center{0.5f, 0.5f};
    bool sizeAttenuation = true;
    std::shared_ptr<Texture> map;
    std::shared_ptr<Texture> alphaMap;
    SpriteMaterial() { type = MaterialType::Sprite; transparent = true; }
};

class DepthMaterial : public Material {
public:
    bool skinning = false;
    DepthMaterial() { type = MaterialType::Depth; }
};

class ShaderMaterial : public Material {
public:
    std::string vertexShader;
    std::string fragmentShader;
    ShaderMaterial() { type = MaterialType::Shader; }
};

} // namespace threecpp
