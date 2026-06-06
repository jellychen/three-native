#pragma once
#include "core/Scene.hpp"
#include "core/Camera.hpp"
#include "core/Renderable.hpp"
#include "light/Light.hpp"
#include "renderer/GLProgram.hpp"
#include "renderer/GLResources.hpp"
#include "renderer/ProgramCache.hpp"
#include "platform/gl_headers.hpp"

namespace threecpp {

enum class ShadowMapType { Basic, PCF, PCFSoft, VSM };

struct ShadowRenderItem {
    Light* light = nullptr;
    glm::mat4 lightViewProjection{1.0f};
    glm::mat4 shadowMatrix{1.0f};
    int mapSizeX = 1024;
    int mapSizeY = 1024;
    GLuint framebuffer = 0;
    GLuint depthTexture = 0;
    GLuint depthCubeTexture = 0;
    bool isPointShadow = false;
    float cameraNear = 0.1f;
    float cameraFar = 200.0f;
    glm::vec3 lightPosition{0.0f};
    std::array<glm::mat4, 6> pointFaceViewProjection{};
    float bias = 0.0005f;
    float normalBias = 0.0f;
    float radius = 1.0f;
    float intensity = 1.0f;
};

class GLShadowMap {
public:
    bool enabled = true;
    ShadowMapType type = ShadowMapType::PCF;
    std::vector<ShadowRenderItem> items;

    GLShadowMap() = default;
    GLShadowMap(const GLShadowMap&) = delete;
    GLShadowMap& operator=(const GLShadowMap&) = delete;
    ~GLShadowMap();

    void buildShadowItems(Scene& scene);
    void render(Scene& scene, GLResourceManager& resources, ProgramCache& programs);

private:
    struct ShadowTarget { GLuint framebuffer = 0; GLuint depthTexture = 0; int width = 0; int height = 0; };
    struct CubeShadowTarget { GLuint framebuffer = 0; GLuint depthCubeTexture = 0; int size = 0; };
    std::unordered_map<ObjectId, ShadowTarget> targets;
    std::unordered_map<ObjectId, CubeShadowTarget> cubeTargets;

    ShadowTarget& ensureTarget(ObjectId lightId, int w, int h);
    CubeShadowTarget& ensureCubeTarget(ObjectId lightId, int size);
    GLProgram& depthProgram(ProgramCache& programs, const RenderableObject& object);
    void renderDepthObject(RenderableObject& object, const glm::mat4& lightVP, GLResourceManager& resources, GLProgram& program, int drawStart = 0, int drawCount = -1);

    static glm::mat4 biasMatrix();
    static glm::mat4 computeLightViewProjection(const Light& light);
    static std::array<glm::mat4, 6> computePointLightViewProjections(const PointLight& light, float nearPlane, float farPlane);
};

} // namespace threecpp
