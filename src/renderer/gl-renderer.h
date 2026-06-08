#pragma once
#include "core/scene.h"
#include "core/camera.h"
#include "core/renderable.h"
#include "light/light.h"
#include "renderer/gl-state.h"
#include "renderer/gl-program.h"
#include "renderer/gl-resources.h"
#include "renderer/render-list.h"
#include "renderer/render-state.h"
#include "renderer/program-cache.h"
#include "renderer/gl-binding-states.h"
#include "renderer/gl-render-target.h"
#include "renderer/gl-shadow-map.h"
#include "shader/shader-lib.h"
#include "renderer/cache/renderer-cache-diagnostics.h"
#include "renderer/cache/render-list-persistent-cache.h"
#include "renderer/cache/web-gl-state-cache.h"
#include "renderer/cache/texture-unit-allocator.h"

namespace THREE {

class Window;

struct RendererParameters {
    int width = 1280;
    int height = 720;
    bool antialias = true;
    glm::vec4 clearColor{0.02f, 0.02f, 0.025f, 1.0f};
    ToneMapping toneMapping = ToneMapping::ACESFilmic;
    float toneMappingExposure = 1.0f;
    ColorSpace outputColorSpace = ColorSpace::SRGB;
    bool transmission = true;
    // three.js-compatible object sorting toggle. true mirrors renderer.sortObjects.
    bool sortObjects = true;
    // If true, the transmission background pass captures opaque objects only
    // and explicitly excludes transmissive/transparent items from the source.
    bool transmissionExcludesTransparent = true;
    // three.js-style toggle: when enabled, point/spot lights use physically plausible inverse-power decay.
    bool physicallyCorrectLights = true;
    // Debug/robustness: allow disabling object frustum culling when diagnosing empty renders.
    bool frustumCulling = true;
    // Native GLFW/OpenGL convenience: keep viewport/renderer size in sync with
    // the current framebuffer. This fixes macOS Retina where window size and
    // framebuffer size differ, and prevents rendering only into the lower-left quarter.
    bool autoResizeToFramebuffer = true;
    float transmissionResolutionScale = 1.0f;
    float transmissionMipLevel = 4.0f;
};

struct RendererInfo {
    int calls = 0;
    int triangles = 0;
    int lines = 0;
    int points = 0;
    int programs = 0;
    int instancedCalls = 0;
    int instances = 0;
    void resetFrame() { calls = triangles = lines = points = instancedCalls = instances = 0; }
};

class GLRenderer {
    RendererParameters params;
    GLState state;
    GLResourceManager resources;
    RenderList renderList;
    LightCache lightCache;
    ProgramCache programCache;
    RenderStateStack renderStates;
    GLBindingStates bindingStates;
    TextureUnitAllocator textureUnitAllocator;
    WebGLStateCache webglStateCache;
    RenderListPersistentCache persistentRenderListCache;
    int frameIndex = 0;
    ViewportRect viewport;
    ScissorRect scissor;
    std::unique_ptr<GLRenderTarget> transmissionTarget;
    // Backside view-depth target for three.js-style transmission thickness approximation.
    std::unique_ptr<GLRenderTarget> transmissionBackfaceTarget;
    GLShadowMap shadowMap;

    // Background and sprite rendering state (lazy-init)
    struct BackgroundMesh {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ibo = 0;
        int indexCount = 0;
    };
    BackgroundMesh bgCube;
    BackgroundMesh bgQuad;
    GLuint bgEquirectProgram = 0;
    GLuint bgCubeProgram = 0;
    GLuint bg2DProgram = 0;
    GLuint spriteProgram = 0;
    GLuint spriteVao = 0;
    GLuint spriteVbo = 0;

    void ensureBackgroundResources();
    void renderBackground(Scene& scene, Camera& camera);

    bool frustumCullingOverride = false;

public:
    RendererInfo info;

    explicit GLRenderer(RendererParameters p = {});
    void setSize(int w, int h);
    // Compatibility shim for older examples that explicitly initialize a renderer with a Window.
    // Native OpenGL context creation is owned by Window; renderer initialization only syncs size.
    void initialize(Window& window);
    void setClearColor(const glm::vec4& color);
    void setClearColor(const glm::vec3& color, float alpha = 1.0f);
    void setViewport(int x, int y, int w, int h);
   void setScissor(int x, int y, int w, int h);
   void setScissorTest(bool enabled);
   void setToneMapping(ToneMapping mode, float exposure = 1.0f);
    void setRenderTarget(GLRenderTarget* target);
    void renderFullScreenQuad();
    void setOutputColorSpace(ColorSpace colorSpace);
    void clear();
    void render(Scene& scene, Camera& camera);
    RendererCacheDiagnostics diagnostics() const;

private:
    void projectObject(Scene& scene, Object3D& object, Camera& camera);
    void setupLights(Scene& scene);
    void renderObjects(std::span<const RenderItem> items, Scene& scene, Camera& camera);
    bool hasTransmissionItems(std::span<const RenderItem> items) const;
    glm::vec4 resolveClearColor(Scene& scene) const;
    void ensureTransmissionTarget();
    void renderTransmissionBackground(Scene& scene, Camera& camera);
    void renderTransmissionBackfaces(Scene& scene, Camera& camera);
    void syncFramebufferSizeFromCurrentContext();
    GLProgram& getProgram(const RenderItem& item, Scene& scene);
    void setCommonUniforms(GLProgram& program, const RenderItem& item, Scene& scene, Camera& camera);
   GLenum drawMode(PrimitiveMode mode) const;

    GLRenderTarget* activeTarget = nullptr;
    GLuint fsQuadVAO = 0;
    GLuint fsQuadVBO = 0;
    void ensureFSQuad();
};

} // namespace THREE
