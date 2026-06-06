#include "renderer/GLRenderer.hpp"
#include "platform/Window.hpp"
#include <iostream>

namespace threecpp {

namespace {
struct FrustumPlane { glm::vec3 n{0.0f}; float d = 0.0f; };

static FrustumPlane normalize_plane(const glm::vec4& p) {
    glm::vec3 n(p.x, p.y, p.z);
    float invLen = 1.0f / std::max(glm::length(n), 1e-6f);
    return {n * invLen, p.w * invLen};
}

static std::array<FrustumPlane, 6> extract_frustum(const glm::mat4& m) {
    // GLM is column-major. These row combinations mirror the standard
    // clip-space frustum extraction used by three.js-style frustum culling.
    glm::vec4 r0(m[0][0], m[1][0], m[2][0], m[3][0]);
    glm::vec4 r1(m[0][1], m[1][1], m[2][1], m[3][1]);
    glm::vec4 r2(m[0][2], m[1][2], m[2][2], m[3][2]);
    glm::vec4 r3(m[0][3], m[1][3], m[2][3], m[3][3]);
    return {normalize_plane(r3 + r0), normalize_plane(r3 - r0), normalize_plane(r3 + r1), normalize_plane(r3 - r1), normalize_plane(r3 + r2), normalize_plane(r3 - r2)};
}


static bool env_enabled(const char* name) {
    const char* v = std::getenv(name);
    if (!v) return false;
    return std::strcmp(v, "0") != 0 && std::strcmp(v, "false") != 0 && std::strcmp(v, "FALSE") != 0;
}

static bool env_enabled(const char* name, bool defaultValue) {
    const char* v = std::getenv(name);
    if (!v) return defaultValue;
    return std::strcmp(v, "0") != 0 && std::strcmp(v, "false") != 0 && std::strcmp(v, "FALSE") != 0;
}

static bool experimental_pbr_enabled() {
    // v6.0.42: the repaired safe PBR forward path is now the default for
    // MeshStandardMaterial / MeshPhysicalMaterial.  The old MeshBasic fallback
    // was useful during diagnostics, but it made several older examples (10-13)
    // render only the clear background because they relied on Standard/Physical
    // uniforms while the program was downgraded to MeshBasic.
    //
    // Use THREECPP_DISABLE_EXPERIMENTAL_PBR=1 to force the legacy fallback, or
    // keep THREECPP_ENABLE_EXPERIMENTAL_PBR for backwards-compatible scripts.
    if (env_enabled("THREECPP_DISABLE_EXPERIMENTAL_PBR")) return false;
    return env_enabled("THREECPP_ENABLE_EXPERIMENTAL_PBR", true);
}

static const char* gl_error_name(GLenum e) {
    switch (e) {
        case GL_NO_ERROR: return "GL_NO_ERROR";
        case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
#ifdef GL_INVALID_FRAMEBUFFER_OPERATION
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
#endif
        default: return "GL_UNKNOWN_ERROR";
    }
}

static void clear_gl_errors() {
    while (glGetError() != GL_NO_ERROR) {}
}

static void log_gl_errors(const char* tag) {
    GLenum e = GL_NO_ERROR;
    bool any = false;
    while ((e = glGetError()) != GL_NO_ERROR) {
        any = true;
        std::cerr << "[threecpp][gl] " << tag << ": " << gl_error_name(e)
                  << " (0x" << std::hex << e << std::dec << ")\n";
    }
    if (!any && env_enabled("THREECPP_DEBUG_GL_VERBOSE")) {
        std::cerr << "[threecpp][gl] " << tag << ": GL_NO_ERROR\n";
    }
}

static int count_renderables(Object3D& root) {
    int count = 0;
    root.traverseVisible([&](Object3D& o) {
        auto* r = dynamic_cast<RenderableObject*>(&o);
        if (r && r->geometry && r->materialAt(0)) ++count;
    });
    return count;
}

static int total_render_items(const RenderListStats& s) { return s.opaque + s.transmissive + s.transparent; }

static bool sphere_in_frustum(const std::array<FrustumPlane, 6>& frustum, const glm::vec3& center, float radius) {
    for (const auto& p : frustum) {
        if (glm::dot(p.n, center) + p.d < -radius) return false;
    }
    return true;
}

static float max_world_scale(const glm::mat4& m) {
    float sx = glm::length(glm::vec3(m[0]));
    float sy = glm::length(glm::vec3(m[1]));
    float sz = glm::length(glm::vec3(m[2]));
    return std::max(sx, std::max(sy, sz));
}

static bool intersect_draw_range(const BufferGeometry& geometry, int start, int count, int total, int& outStart, int& outCount) {
    const int drawStart = std::max(0, geometry.drawRange.start);
    const int drawLimit = geometry.drawRange.count == std::numeric_limits<int>::max()
        ? total
        : std::max(0, geometry.drawRange.count);
    const long long a0 = std::max(0, start);
    const long long a1 = count < 0 ? total : (a0 + std::max(0, count));
    const long long b0 = drawStart;
    const long long b1 = static_cast<long long>(drawStart) + drawLimit;
    const long long lo = std::max(a0, b0);
    const long long hi = std::min<long long>(std::min<long long>(a1, b1), total);
    if (hi <= lo) return false;
    outStart = static_cast<int>(lo);
    outCount = static_cast<int>(hi - lo);
    return true;
}
}

GLRenderer::GLRenderer(RendererParameters p) : params(p) {
    state.reset();
    viewport = {0, 0, params.width, params.height};
    state.setViewport(viewport.x, viewport.y, viewport.width, viewport.height);
    glEnable(GL_DEPTH_TEST);
}

void GLRenderer::setSize(int w, int h) {
#if !THREECPP_USE_ANGLE
    // macOS Retina compatibility:
    // Many examples historically call renderer.setSize(window.width(), window.height())
    // every frame. On Retina displays the GLFW window size is in logical points while
    // the framebuffer is in physical pixels. If we accepted the logical size here,
    // render() would immediately auto-resize back to the framebuffer size, causing
    // expensive render-target reallocations every frame, especially transmissionTarget.
    if (params.autoResizeToFramebuffer) {
        if (GLFWwindow* current = glfwGetCurrentContext()) {
            int winW = 0, winH = 0, fbW = 0, fbH = 0;
            glfwGetWindowSize(current, &winW, &winH);
            glfwGetFramebufferSize(current, &fbW, &fbH);
            if (w == winW && h == winH && (fbW != winW || fbH != winH)) {
                w = fbW;
                h = fbH;
            }
        }
    }
#endif
    w = std::max(1, w);
    h = std::max(1, h);
    if (params.width == w && params.height == h && viewport.width == w && viewport.height == h) {
        return;
    }
    params.width = w;
    params.height = h;
    setViewport(0, 0, w, h);
    if (transmissionTarget) {
        const int tw = std::max(1, int(w * params.transmissionResolutionScale));
        const int th = std::max(1, int(h * params.transmissionResolutionScale));
        transmissionTarget->resize(tw, th);
    }
    if (transmissionBackfaceTarget) {
        const int tw = std::max(1, int(w * params.transmissionResolutionScale));
        const int th = std::max(1, int(h * params.transmissionResolutionScale));
        transmissionBackfaceTarget->resize(tw, th);
    }
}
void GLRenderer::initialize(Window& window) {
    auto fb = window.framebufferSize();
    setSize(fb.x, fb.y);
}


void GLRenderer::syncFramebufferSizeFromCurrentContext() {
#if !THREECPP_USE_ANGLE
    if (!params.autoResizeToFramebuffer) return;
    GLFWwindow* current = glfwGetCurrentContext();
    if (!current) return;
    int fbw = 0;
    int fbh = 0;
    glfwGetFramebufferSize(current, &fbw, &fbh);
    fbw = std::max(fbw, 1);
    fbh = std::max(fbh, 1);
    if (fbw != params.width || fbh != params.height || viewport.width != fbw || viewport.height != fbh) {
        setSize(fbw, fbh);
    }
#endif
}

void GLRenderer::setClearColor(const glm::vec4& c) { params.clearColor = c; }

void GLRenderer::setClearColor(const glm::vec3& color, float alpha) {
    params.clearColor = glm::vec4(color, alpha);
}

void GLRenderer::setViewport(int x, int y, int w, int h) {
    viewport = {x, y, w, h};
    state.setViewport(x, y, w, h);
}

void GLRenderer::setScissor(int x, int y, int w, int h) {
    scissor.x = x; scissor.y = y; scissor.width = w; scissor.height = h;
    state.setScissor(x, y, w, h);
}

void GLRenderer::setScissorTest(bool enabled) {
    scissor.enabled = enabled;
    state.setScissorTest(enabled);
}
void GLRenderer::setToneMapping(ToneMapping mode, float exposure) { params.toneMapping = mode; params.toneMappingExposure = exposure; }
void GLRenderer::setOutputColorSpace(ColorSpace colorSpace) { params.outputColorSpace = colorSpace; }

void GLRenderer::clear() {
    glClearColor(params.clearColor.r, params.clearColor.g, params.clearColor.b, params.clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void GLRenderer::render(Scene& scene, Camera& camera) {
    syncFramebufferSizeFromCurrentContext();
    info.resetFrame();
    textureUnitAllocator.beginFrame(frameIndex++);
    scene.updateMatrixWorld();
    camera.updateMatrixWorld();

    setupLights(scene);
    shadowMap.render(scene, resources, programCache);

    // ShadowMap uses raw glBindVertexArray() inside GLShadowMap::renderDepthObject().
    // That bypasses GLBindingStates, so its cached currentVAO can become stale.
    // If the cache still thinks the forward VAO is bound, bindVertexArray() may
    // early-return while OpenGL actually has VAO 0 or a shadow-pass VAO bound;
    // glDrawElements then silently fails on macOS Core or draws nothing.
    // Reset the VAO cache after every shadow pass before the forward pass.
    bindingStates.reset();

    // ShadowMap and transmission passes intentionally use raw GL calls for
    // framebuffer, draw-buffer, cull, color-mask and program state. The cached
    // GLState object does not see those calls, so force the default forward
    // framebuffer into a known-good state before building/rendering the normal
    // queues. This fixes the "background only, no meshes" failure seen in
    // examples 14+ on macOS after shadow passes.
#if !THREECPP_USE_ANGLE
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK);
    glReadBuffer(GL_BACK);
#endif
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glDisable(GL_POLYGON_OFFSET_FILL);
    state.reset();
    state.setViewport(viewport.x, viewport.y, viewport.width, viewport.height);
    if (scissor.enabled) state.setScissorTest(true);

    setupLights(scene);
    RenderState& frameState = renderStates.push(scene, camera);
    frameState.viewport = viewport;
    frameState.scissor = scissor;
    const bool debugRender = env_enabled("THREECPP_DEBUG_RENDER");
    const int visibleRenderableCount = debugRender ? count_renderables(scene) : 0;
    renderList.clear();
    frustumCullingOverride = env_enabled("THREECPP_DISABLE_FRUSTUM_CULLING");
    projectObject(scene, scene, camera);
    renderList.sort(params.sortObjects);

    RenderListStats builtStats = renderList.stats();
    if (params.frustumCulling && !frustumCullingOverride && total_render_items(builtStats) == 0) {
        const int fallbackVisibleCount = debugRender ? visibleRenderableCount : count_renderables(scene);
        if (fallbackVisibleCount > 0) {
            // Robustness fallback: if the scene contains renderable objects but
            // the render list is empty, the most likely cause is stale/invalid
            // bounding spheres or a camera/frustum mismatch in an example.
            // Rebuild once without culling so the frame is visible and print a
            // diagnostic when enabled.
            frustumCullingOverride = true;
            renderList.clear();
            projectObject(scene, scene, camera);
            renderList.sort(params.sortObjects);
            builtStats = renderList.stats();
            if (debugRender) {
                std::cerr << "[threecpp] render list was empty with " << fallbackVisibleCount
                          << " visible renderables; rebuilt with frustum culling disabled for this frame.\n";
            }
        }
    }
    frustumCullingOverride = false;

    if (debugRender) {
        std::cerr << "[threecpp] renderables=" << visibleRenderableCount
                  << " queues opaque=" << builtStats.opaque
                  << " transmissive=" << builtStats.transmissive
                  << " transparent=" << builtStats.transparent
                  << " viewport=" << viewport.width << "x" << viewport.height
                  << " fbSize=" << params.width << "x" << params.height
                  << "\n";
    }

    // Match three.js-style scene background fallback: when no explicit
    // background texture pass exists yet, use Scene::backgroundColor for clear.
    const glm::vec4 previousClear = params.clearColor;
    params.clearColor = resolveClearColor(scene);
    clear();
    params.clearColor = previousClear;
    frameState.listStats = renderList.stats();
    renderObjects(renderList.opaque, scene, camera);
    if (params.transmission && !env_enabled("THREECPP_DISABLE_TRANSMISSION_CAPTURE") && (!renderList.transmissive.empty() || hasTransmissionItems(renderList.transparent))) {
        renderTransmissionBackground(scene, camera);
        if (!env_enabled("THREECPP_DISABLE_TRANSMISSION_BACKFACE")) {
            renderTransmissionBackfaces(scene, camera);
        }
    }
    renderObjects(renderList.transmissive, scene, camera);
    renderObjects(renderList.transparent, scene, camera);
    info.programs = programCache.size();
    if (debugRender) {
        std::cerr << "[threecpp] frame drawCalls=" << info.calls
                  << " instancedCalls=" << info.instancedCalls
                  << " triangles=" << info.triangles
                  << " lines=" << info.lines
                  << " points=" << info.points
                  << " programs=" << info.programs
                  << "\n";
    }
    renderStates.pop();
}


RendererCacheDiagnostics GLRenderer::diagnostics() const {
    RendererCacheDiagnostics d;
    d.programCache = programCache.getStats();
    d.textureUnits = textureUnitAllocator.getStats();
    d.stateCache = webglStateCache.getStats();
    d.renderListCache = persistentRenderListCache.getStats();
    d.geometryCacheEntries = resources.geometryCacheSize();
    return d;
}

bool GLRenderer::hasTransmissionItems(std::span<const RenderItem> items) const {
    return std::any_of(items.begin(), items.end(), [](const RenderItem& item) {
        return item.receivesTransmissionBackground;
    });
}

glm::vec4 GLRenderer::resolveClearColor(Scene& scene) const {
    if (!scene.background && !scene.environment) {
        return glm::vec4(glm::vec3(scene.backgroundColor), scene.backgroundColor.alpha);
    }
    if (!scene.background && scene.environment) {
        const float blur = glm::clamp(scene.environment->backgroundBlurriness, 0.0f, 1.0f);
        const glm::vec3 bg = glm::mix(scene.environment->skyColor, scene.environment->specularColor, blur * 0.35f) * scene.environment->backgroundIntensity;
        return glm::vec4(bg, params.clearColor.a);
    }
    return params.clearColor;
}

void GLRenderer::ensureTransmissionTarget() {
    int w = std::max(1, int(params.width * params.transmissionResolutionScale));
    int h = std::max(1, int(params.height * params.transmissionResolutionScale));
    if (!transmissionTarget) {
        transmissionTarget = std::make_unique<GLRenderTarget>(GLRenderTargetOptions{w, h, true, false, true});
    } else {
        transmissionTarget->resize(w, h);
    }
    if (!transmissionBackfaceTarget) {
        transmissionBackfaceTarget = std::make_unique<GLRenderTarget>(GLRenderTargetOptions{w, h, true, false, false});
    } else {
        transmissionBackfaceTarget->resize(w, h);
    }
}

void GLRenderer::renderTransmissionBackground(Scene& scene, Camera& camera) {
    ensureTransmissionTarget();
    transmissionTarget->bind();
    glViewport(0, 0, transmissionTarget->options.width, transmissionTarget->options.height);
    const glm::vec4 clearColor = resolveClearColor(scene);
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    // Capture exactly the opaque source scene for transmissive materials.
    // This matches the key WebGLRenderer behavior: transmissive objects do not
    // appear in their own refraction source and transparent blend objects are
    // excluded unless the renderer is explicitly configured otherwise.
    renderObjects(renderList.opaque, scene, camera);
    if (!params.transmissionExcludesTransparent) {
        renderObjects(renderList.transparent, scene, camera);
    }
    transmissionTarget->generateMipmaps();
    transmissionTarget->unbind();
    state.setViewport(viewport.x, viewport.y, viewport.width, viewport.height);
}


void GLRenderer::renderTransmissionBackfaces(Scene& scene, Camera& camera) {
    if (renderList.transmissive.empty()) return;
    ensureTransmissionTarget();
    if (!transmissionBackfaceTarget) return;

    // three.js strategy approximation: render transmissive backfaces into a
    // screen-space view-depth texture. The forward physical shader samples this
    // target and estimates object thickness as backDepth - frontDepth. This is
    // the missing piece that makes complex GLB transmission read as a solid
    // refractive volume rather than a flat transparent shell.
    GLint oldFbo = 0;
    GLint oldViewport[4] = {0,0,0,0};
    GLboolean oldDepthMask = GL_TRUE;
    GLboolean oldCullEnabled = glIsEnabled(GL_CULL_FACE);
    GLint oldCullFace = GL_BACK;
    GLint oldDepthFunc = GL_LESS;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFbo);
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &oldDepthMask);
    glGetIntegerv(GL_CULL_FACE_MODE, &oldCullFace);
    glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunc);

    transmissionBackfaceTarget->bind();
    glViewport(0, 0, transmissionBackfaceTarget->options.width, transmissionBackfaceTarget->options.height);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT); // draw only backfaces
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    float cameraFar = 2000.0f;
    if (auto* pcam = dynamic_cast<PerspectiveCamera*>(&camera)) cameraFar = std::max(0.001f, pcam->farPlane);
    else if (auto* ocam = dynamic_cast<OrthographicCamera*>(&camera)) cameraFar = std::max(0.001f, ocam->farPlane);

    ProgramKey key;
    key.materialType = MaterialType::Distance;
    key.primitiveMode = PrimitiveMode::Triangles;
    for (const auto& item : renderList.transmissive) {
        if (!item.geometry || !item.object) continue;
        key.useInstancing = dynamic_cast<InstancedMesh*>(item.object) != nullptr;
        key.useSkinning = item.object->kind == ObjectKind::SkinnedMesh && item.geometry->hasAttribute("skinIndex") && item.geometry->hasAttribute("skinWeight");
        key.useMorphTargets = item.geometry->morphTargetCount("position") > 0;
        key.morphTargetsRelative = item.geometry->morphTargetsRelative;
        key.morphTargetCount = std::min(4, item.geometry->morphTargetCount("position"));

        GLProgram& program = programCache.get(key);
        state.useProgram(program.id);
        glUniformMatrix4fv(program.uniform("modelMatrix"), 1, GL_FALSE, &item.object->matrixWorld[0][0]);
        glUniformMatrix4fv(program.uniform("viewMatrix"), 1, GL_FALSE, &camera.matrixWorldInverse[0][0]);
        glUniformMatrix4fv(program.uniform("projectionMatrix"), 1, GL_FALSE, &camera.projectionMatrix[0][0]);
        glUniform1f(program.uniform("cameraFar"), cameraFar);
        auto& glg = resources.getOrCreateGeometry(*item.geometry);
        bindingStates.bindVertexArray(glg.vao);
        int drawInstances = 1;
        if (auto* inst = dynamic_cast<InstancedMesh*>(item.object)) {
            drawInstances = std::max(0, inst->count);
            resources.updateInstancedAttributes(*inst);
        }
        const int totalCount = glg.hasIndex ? glg.indexCount : glg.vertexCount;
        const int drawStart = glm::clamp(item.groupStart, 0, std::max(0, totalCount));
        const int requestedCount = item.groupCount < 0 ? (totalCount - drawStart) : item.groupCount;
        const int drawCount = glm::clamp(requestedCount, 0, std::max(0, totalCount - drawStart));
        if (drawCount <= 0) continue;
        const void* indexOffset = reinterpret_cast<const void*>(static_cast<std::uintptr_t>(drawStart) * sizeof(std::uint32_t));
        if (drawInstances > 1) {
            if (glg.hasIndex) glDrawElementsInstanced(GL_TRIANGLES, drawCount, GL_UNSIGNED_INT, indexOffset, drawInstances);
            else glDrawArraysInstanced(GL_TRIANGLES, drawStart, drawCount, drawInstances);
        } else {
            if (glg.hasIndex) glDrawElements(GL_TRIANGLES, drawCount, GL_UNSIGNED_INT, indexOffset);
            else glDrawArrays(GL_TRIANGLES, drawStart, drawCount);
        }
    }
    bindingStates.reset();

    glBindFramebuffer(GL_FRAMEBUFFER, oldFbo);
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    glDepthMask(oldDepthMask);
    glDepthFunc(oldDepthFunc);
    if (oldCullEnabled) { glEnable(GL_CULL_FACE); glCullFace(oldCullFace); } else { glDisable(GL_CULL_FACE); }
    state.setViewport(viewport.x, viewport.y, viewport.width, viewport.height);
}

void GLRenderer::setupLights(Scene& scene) {
    lightCache.reset();
    scene.traverseVisible([&](Object3D& o) {
        auto* light = dynamic_cast<Light*>(&o);
        if (!light) return;
        switch (light->lightType) {
            case LightType::Ambient:
                lightCache.ambient += light->color * light->intensity;
                break;
            case LightType::Hemisphere: {
                auto* hemi = dynamic_cast<HemisphereLight*>(light);
                lightCache.hemisphereSky += (hemi ? hemi->skyColor : light->color) * light->intensity;
                lightCache.hemisphereGround += (hemi ? hemi->groundColor : glm::vec3(0.5f)) * light->intensity;
                break;
            }
            case LightType::Directional: {
                GpuLight g; g.type = 1; g.colorIntensity = glm::vec4(light->color, light->intensity);
                glm::vec3 pos = glm::vec3(light->matrixWorld[3]);
                glm::vec3 target{0.0f};
                if (auto* d = dynamic_cast<DirectionalLight*>(light)) target = d->target;
                glm::vec3 dir = target - pos;
                if (glm::dot(dir, dir) < 1e-8f) dir = glm::vec3(0.0f, -1.0f, 0.0f);
                // directionCone.xyz is the direction in which the light travels.
                // Shaders use -direction as the surface-to-light vector.
                g.directionCone = glm::vec4(glm::normalize(dir), 0.0f);
                if (light->castShadow && light->shadow.enabled) {
                    for (std::size_t si = 0; si < shadowMap.items.size(); ++si) { if (shadowMap.items[si].light == light) { g.shadowIndex = static_cast<int>(si); break; } }
                }
                lightCache.gpuLights.push_back(g);
                break;
            }
            case LightType::Point: {
                auto* p = dynamic_cast<PointLight*>(light);
                GpuLight g; g.type = 2; g.colorIntensity = glm::vec4(light->color, light->intensity);
                glm::vec3 pos = glm::vec3(light->matrixWorld[3]);
                g.positionRange = glm::vec4(pos, p ? std::max(0.0f, p->distance) : 0.0f);
                g.params.x = params.physicallyCorrectLights ? (p ? std::max(0.0f, p->decay) : 2.0f) : 0.0f;
                lightCache.gpuLights.push_back(g);
                break;
            }
            case LightType::Spot: {
                auto* s = dynamic_cast<SpotLight*>(light);
                GpuLight g; g.type = 3; g.colorIntensity = glm::vec4(light->color, light->intensity);
                glm::vec3 pos = glm::vec3(light->matrixWorld[3]);
                glm::vec3 target = s ? s->target : glm::vec3(0.0f);
                glm::vec3 dir = target - pos;
                if (glm::dot(dir, dir) < 1e-8f) dir = glm::vec3(0.0f, -1.0f, 0.0f);
                const float outerAngle = s ? glm::clamp(s->angle, 0.001f, glm::half_pi<float>() - 0.001f) : glm::radians(30.0f);
                const float penumbra = s ? glm::clamp(s->penumbra, 0.0f, 1.0f) : 0.0f;
                const float outerCos = std::cos(outerAngle);
                const float innerCos = std::cos(outerAngle * (1.0f - penumbra));
                g.positionRange = glm::vec4(pos, s ? std::max(0.0f, s->distance) : 0.0f);
                g.directionCone = glm::vec4(glm::normalize(dir), outerCos);
                g.params.x = params.physicallyCorrectLights ? (s ? std::max(0.0f, s->decay) : 2.0f) : 0.0f;
                g.params.y = innerCos;
                g.params.z = penumbra;
                if (light->castShadow && light->shadow.enabled) {
                    for (std::size_t si = 0; si < shadowMap.items.size(); ++si) { if (shadowMap.items[si].light == light) { g.shadowIndex = static_cast<int>(si); break; } }
                }
                lightCache.gpuLights.push_back(g);
                break;
            }
            case LightType::RectArea: {
                auto* r = dynamic_cast<RectAreaLight*>(light);
                GpuLight g; g.type = 4; g.colorIntensity = glm::vec4(light->color, light->intensity);
                glm::vec3 pos = glm::vec3(light->matrixWorld[3]);
                glm::vec3 target = r ? r->target : glm::vec3(0.0f);
                glm::vec3 dir = target - pos;
                if (glm::dot(dir, dir) < 1e-8f) dir = glm::vec3(0.0f, -1.0f, 0.0f);
                g.positionRange = glm::vec4(pos, 0.0f);
                g.directionCone = glm::vec4(glm::normalize(dir), 0.0f);
                g.params.x = r ? std::max(0.001f, r->width) : 1.0f;
                g.params.y = r ? std::max(0.001f, r->height) : 1.0f;
                lightCache.gpuLights.push_back(g);
                break;
            }
            default: break;
        }
    });
}

void GLRenderer::projectObject(Scene& scene, Object3D& object, Camera& camera) {
    const auto frustum = extract_frustum(camera.projectionMatrix * camera.matrixWorldInverse);
    object.traverseVisible([&](Object3D& o) {
        auto* r = dynamic_cast<RenderableObject*>(&o);
        if (!r || !r->geometry || (!r->material && !scene.overrideMaterial)) return;
        if (!o.testLayers(camera)) return;
        if (params.frustumCulling && !frustumCullingOverride && o.frustumCulled && r->geometry->boundingSphere.valid) {
            glm::vec3 worldCenter = glm::vec3(r->matrixWorld * glm::vec4(r->geometry->boundingSphere.center, 1.0f));
            float worldRadius = r->geometry->boundingSphere.radius * max_world_scale(r->matrixWorld);
            if (!sphere_in_frustum(frustum, worldCenter, worldRadius)) return;
        }
        glm::vec4 viewPos = camera.matrixWorldInverse * r->matrixWorld * glm::vec4(0, 0, 0, 1);
        const PrimitiveMode mode = primitive_for_object(o);
        const int total = !r->geometry->indices.empty() ? r->geometry->indexCount() : r->geometry->vertexCount();
        if (!r->geometry->groups.empty()) {
            for (const auto& group : r->geometry->groups) {
                Material* groupMaterial = scene.overrideMaterial ? scene.overrideMaterial.get() : r->materialAt(group.materialIndex);
                if (!groupMaterial) continue;
                int start = 0, count = 0;
                if (!intersect_draw_range(*r->geometry, group.start, group.count, total, start, count)) continue;
                renderList.push(o, *r->geometry, *groupMaterial, mode, viewPos.z, start, count, group.materialIndex);
            }
        } else {
            Material* primary = scene.overrideMaterial ? scene.overrideMaterial.get() : r->materialAt(0);
            if (!primary) return;
            int start = 0, count = 0;
            if (!intersect_draw_range(*r->geometry, 0, total, total, start, count)) return;
            renderList.push(o, *r->geometry, *primary, mode, viewPos.z, start, count, 0);
        }
    });
}

void GLRenderer::renderObjects(std::span<const RenderItem> items, Scene& scene, Camera& camera) {
    const bool forceUnlit = env_enabled("THREECPP_FORCE_UNLIT");
    const bool forceDepthOff = env_enabled("THREECPP_FORCE_DEPTH_OFF") || forceUnlit;
    const bool forceDoubleSide = env_enabled("THREECPP_FORCE_DOUBLE_SIDE") || forceUnlit;
    const bool debugDraw = env_enabled("THREECPP_DEBUG_DRAW");
    int drawIndex = 0;

    if (forceUnlit) {
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
#if !THREECPP_USE_ANGLE
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
    }

    for (const auto& item : items) {
        if (!item.geometry || !item.material || !item.object) continue;
        if (debugDraw) clear_gl_errors();
        GLProgram& program = getProgram(item, scene);
        state.useProgram(program.id);
        if (forceUnlit) {
            glDisable(GL_BLEND);
            glDisable(GL_CULL_FACE);
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        } else {
            state.applyMaterial(*item.material);
            // Robust forward-state guard. Several auxiliary passes and old examples
            // still use raw OpenGL calls, bypassing GLState's cache. If actual GL
            // state and cached state diverge, draw calls can execute while producing
            // no visible fragments. Re-assert the critical states before each draw.
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDepthRange(0.0, 1.0);
            glDepthFunc(GL_LEQUAL);
            if (item.material->depthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
            glDepthMask(item.material->depthWrite ? GL_TRUE : GL_FALSE);
            const bool blendEnabled = item.material->transparent && item.material->blending != Blending::None;
            if (!blendEnabled) glDisable(GL_BLEND);
            if (item.material->side == Side::DoubleSide) {
                glDisable(GL_CULL_FACE);
            } else {
                glEnable(GL_CULL_FACE);
                glFrontFace(GL_CCW);
                glCullFace(item.material->side == Side::BackSide ? GL_FRONT : GL_BACK);
            }
            if (forceDepthOff) glDisable(GL_DEPTH_TEST);
            if (forceDoubleSide) glDisable(GL_CULL_FACE);
        }
        setCommonUniforms(program, item, scene, camera);
        auto& glg = resources.getOrCreateGeometry(*item.geometry);
        bindingStates.bindVertexArray(glg.vao);
        GLenum mode = drawMode(item.primitiveMode);
        int drawInstances = 1;
        if (auto* inst = dynamic_cast<InstancedMesh*>(item.object)) {
            drawInstances = std::max(0, inst->count);
            resources.updateInstancedAttributes(*inst);
        }
        const int totalCount = glg.hasIndex ? glg.indexCount : glg.vertexCount;
        const int drawStart = glm::clamp(item.groupStart, 0, std::max(0, totalCount));
        const int requestedCount = item.groupCount < 0 ? (totalCount - drawStart) : item.groupCount;
        const int drawCount = glm::clamp(requestedCount, 0, std::max(0, totalCount - drawStart));
        if (drawCount <= 0) { bindingStates.reset(); continue; }
        if (debugDraw) {
            const glm::mat4 mvp = camera.projectionMatrix * camera.matrixWorldInverse * item.object->matrixWorld;
            const glm::vec4 clipOrigin = mvp * glm::vec4(0, 0, 0, 1);
            std::cerr << "[threecpp][draw] #" << drawIndex
                      << " object='" << item.object->name << "'"
                      << " materialType=" << static_cast<int>(item.material->type)
                      << " program=" << program.id
                      << " vao=" << glg.vao
                      << " indexed=" << glg.hasIndex
                      << " total=" << totalCount
                      << " start=" << drawStart
                      << " count=" << drawCount
                      << " mode=" << mode
                      << " clipOrigin=(" << clipOrigin.x << "," << clipOrigin.y << "," << clipOrigin.z << "," << clipOrigin.w << ")"
                      << "\n";
        }
        const void* indexOffset = reinterpret_cast<const void*>(static_cast<std::uintptr_t>(drawStart) * sizeof(std::uint32_t));
        if (drawInstances > 1) {
            if (glg.hasIndex) glDrawElementsInstanced(mode, drawCount, GL_UNSIGNED_INT, indexOffset, drawInstances);
            else glDrawArraysInstanced(mode, drawStart, drawCount, drawInstances);
            info.instancedCalls++;
            info.instances += drawInstances;
        } else {
            if (glg.hasIndex) glDrawElements(mode, drawCount, GL_UNSIGNED_INT, indexOffset);
            else glDrawArrays(mode, drawStart, drawCount);
        }
        if (debugDraw) log_gl_errors("after draw");
        bindingStates.reset();
        info.calls++;
        drawIndex++;
        const int primitiveCount = drawCount;
        const int multiplier = std::max(1, drawInstances);
        if (item.primitiveMode == PrimitiveMode::Points) info.points += primitiveCount * multiplier;
        else if (item.primitiveMode == PrimitiveMode::Lines || item.primitiveMode == PrimitiveMode::LineStrip || item.primitiveMode == PrimitiveMode::LineLoop) info.lines += (primitiveCount / 2) * multiplier;
        else info.triangles += (primitiveCount / 3) * multiplier;
    }
}

GLProgram& GLRenderer::getProgram(const RenderItem& item, Scene& scene) {
    ProgramKey key;
    key.materialType = env_enabled("THREECPP_FORCE_UNLIT") ? MaterialType::MeshBasic : item.material->type;
    key.primitiveMode = item.primitiveMode;
    key.useFlatShading = item.material->flatShading;
    key.usePremultipliedAlpha = item.material->premultipliedAlpha;
    if (auto* inst = dynamic_cast<InstancedMesh*>(item.object)) {
        key.useInstancing = inst->count > 0;
        key.useInstanceColor = key.useInstancing && !inst->instanceColors.empty();
    }
    key.useVertexColor = item.material->vertexColors && item.geometry->hasAttribute("color");
    key.hasUv2 = item.geometry->hasAttribute("uv2");
    key.useSkinning = item.object->kind == ObjectKind::SkinnedMesh && item.geometry->hasAttribute("skinIndex") && item.geometry->hasAttribute("skinWeight");
    key.useMorphTargets = item.geometry->morphTargetCount("position") > 0;
    key.useMorphNormals = item.geometry->morphTargetCount("normal") > 0;
    if (key.useInstancing) key.useMorphNormals = false; // keep attribute usage within portable GL/WebGL2 limits
    key.morphTargetsRelative = item.geometry->morphTargetsRelative;
    key.morphTargetCount = std::min(4, std::max(item.geometry->morphTargetCount("position"), item.geometry->morphTargetCount("normal")));
    key.useIBL = scene.environment != nullptr && (item.material->type == MaterialType::MeshStandard || item.material->type == MaterialType::MeshPhysical);
    key.useEnvMapEquirect = key.useIBL && scene.environment && scene.environment->equirectangularMap != nullptr;
    key.usePMREM = key.useIBL && scene.environment && scene.environment->hasPMREM && scene.environment->irradianceMap && scene.environment->prefilterMap && scene.environment->brdfLUT;

    // v6.0.40: restore IBL/PMREM in the experimental PBR path behind explicit
    // stability gates.  The PBR core and shadows are now stable; environment
    // specular can be disabled independently if a model/environment exposes a
    // driver-specific sampler issue.
    if ((item.material->type == MaterialType::MeshStandard || item.material->type == MaterialType::MeshPhysical) &&
        experimental_pbr_enabled()) {
        if (env_enabled("THREECPP_DISABLE_PBR_IBL")) {
            key.useIBL = false;
            key.useEnvMapEquirect = false;
            key.usePMREM = false;
        }
        if (env_enabled("THREECPP_DISABLE_PBR_PMREM")) {
            key.usePMREM = false;
        }
        if (!env_enabled("THREECPP_ENABLE_PBR_PMREM", true)) {
            key.usePMREM = false;
        }
    }

    key.useShadowMap = !env_enabled("THREECPP_DISABLE_PBR_SHADOWS") && !shadowMap.items.empty() && (item.material->type == MaterialType::MeshLambert || item.material->type == MaterialType::MeshPhong || item.material->type == MaterialType::MeshStandard || item.material->type == MaterialType::MeshPhysical);

    // v6.0.29: experimental PBR no longer uses the legacy USE_SHADOWMAP array/cube
    // branch. That path made valid PBR draws disappear on macOS. When explicitly
    // requested, compile a minimal one-directional-light shadow sampler instead.
    if ((item.material->type == MaterialType::MeshStandard || item.material->type == MaterialType::MeshPhysical) &&
        experimental_pbr_enabled()) {
        key.useShadowMap = false;

        // v6.0.38: compile isolated PBR shadow samplers only when a matching
        // shadow item exists. On macOS OpenGL, merely declaring active sampler2D
        // and samplerCube uniforms that default to the same texture unit can make
        // a program silently produce no visible fragments. The previous code
        // enabled Directional/Spot/Point shadow branches together whenever
        // THREECPP_ENABLE_PBR_SHADOWS=1, even in single-light tests.
        bool hasDirectionalShadow = false;
        bool hasSpotShadow = false;
        bool hasPointShadow = false;
        if (env_enabled("THREECPP_ENABLE_PBR_SHADOWS")) {
            for (const auto& sitem : shadowMap.items) {
                if (sitem.isPointShadow && sitem.depthCubeTexture && sitem.light && sitem.light->lightType == LightType::Point) {
                    hasPointShadow = true;
                } else if (!sitem.isPointShadow && sitem.depthTexture && sitem.light && sitem.light->lightType == LightType::Directional) {
                    hasDirectionalShadow = true;
                } else if (!sitem.isPointShadow && sitem.depthTexture && sitem.light && sitem.light->lightType == LightType::Spot) {
                    hasSpotShadow = true;
                }
            }
        }

        key.usePBRDirectionalShadow = hasDirectionalShadow;
        key.usePBRSpotShadow = hasSpotShadow && env_enabled("THREECPP_ENABLE_PBR_SPOT_SHADOWS", true);
        key.usePBRPointShadow = hasPointShadow && env_enabled("THREECPP_ENABLE_PBR_POINT_SHADOWS", true);
    }
    key.useDashedLine = item.material->type == MaterialType::LineDashed;
    if (item.material->type == MaterialType::FatLine) {
        if (auto* fat = dynamic_cast<FatLineMaterial*>(item.material)) key.useDashedLine = fat->dashed;
    }
    key.numDirLights = 0; key.numPointLights = 0; key.numSpotLights = 0;
    for (const auto& l : lightCache.gpuLights) { if (l.type == 1) key.numDirLights++; else if (l.type == 2) key.numPointLights++; else if (l.type == 3) key.numSpotLights++; }
    if (auto* p = dynamic_cast<PointsMaterial*>(item.material)) key.useSizeAttenuation = p->sizeAttenuation;
    if (auto* m = dynamic_cast<MeshBasicMaterial*>(item.material)) { key.useMap = !!m->map; key.useAlphaMap = !!m->alphaMap; }
    if (auto* m = dynamic_cast<MeshLambertMaterial*>(item.material)) { key.useMap = !!m->map; key.useAOMap = !!m->aoMap; key.useLightMap = !!m->lightMap; key.useEmissiveMap = !!m->emissiveMap; }
    if (auto* m = dynamic_cast<MeshPhongMaterial*>(item.material)) { key.useMap = !!m->map; key.useNormalMap = !!m->normalMap; key.useLightMap = !!m->lightMap; key.useEmissiveMap = !!m->emissiveMap; key.useSpecularMap = !!m->specularMap; }
    if (auto* m = dynamic_cast<PointsMaterial*>(item.material)) { key.useMap = !!m->map; key.useAlphaMap = !!m->alphaMap; }
    if (auto* m = dynamic_cast<MeshStandardMaterial*>(item.material)) {
        key.useMap = !!m->map; key.useAlphaMap = !!m->alphaMap; key.useNormalMap = !!m->normalMap; key.useBumpMap = !!m->bumpMap; key.useDisplacementMap = !!m->displacementMap; key.useRoughnessMap = !!m->roughnessMap; key.useMetalnessMap = !!m->metalnessMap; key.useAOMap = !!m->aoMap; key.useLightMap = !!m->lightMap; key.useEmissiveMap = !!m->emissiveMap;
    }
    if (auto* m = dynamic_cast<MeshPhysicalMaterial*>(item.material)) {
        key.usePhysical = true;
        key.useTransmission = m->transmission > 0.0f || m->transmissionMap != nullptr || m->thickness > 0.0f || m->thicknessMap != nullptr;
        key.useTransmissionMap = !!m->transmissionMap;
        key.useTransmissionRenderTarget = params.transmission && key.useTransmission && !env_enabled("THREECPP_DISABLE_TRANSMISSION_CAPTURE");
        key.useThicknessMap = !!m->thicknessMap;
        key.useSpecularMap = !!m->specularMap || !!m->specularIntensityMap || !!m->specularColorMap;
        key.useClearcoat = m->clearcoat > 0.0f || m->clearcoatMap || m->clearcoatRoughnessMap || m->clearcoatNormalMap;
        key.useClearcoatMap = !!m->clearcoatMap;
        key.useClearcoatRoughnessMap = !!m->clearcoatRoughnessMap;
        key.useClearcoatNormalMap = !!m->clearcoatNormalMap;
        key.useSheen = m->sheen > 0.0f || m->sheenColorMap || m->sheenRoughnessMap;
        key.useSheenColorMap = !!m->sheenColorMap;
        key.useSheenRoughnessMap = !!m->sheenRoughnessMap;
        key.useIridescence = m->iridescence > 0.0f || m->iridescenceMap || m->iridescenceThicknessMap;
        key.useIridescenceMap = !!m->iridescenceMap;
        key.useIridescenceThicknessMap = !!m->iridescenceThicknessMap;
        key.useAnisotropy = std::abs(m->anisotropy) > 0.0f || m->anisotropyMap;
        key.useAnisotropyMap = !!m->anisotropyMap;
        key.useDispersion = m->dispersion > 0.0f;
    }

    if (env_enabled("THREECPP_FORCE_NO_TEXTURES")) {
        key.useMap = false;
        key.useAlphaMap = false;
        key.useNormalMap = false;
        key.useBumpMap = false;
        key.useDisplacementMap = false;
        key.useRoughnessMap = false;
        key.useMetalnessMap = false;
        key.useAOMap = false;
        key.useLightMap = false;
        key.useEmissiveMap = false;
        key.useSpecularMap = false;
        key.useTransmissionMap = false;
        key.useThicknessMap = false;
        key.useClearcoatMap = false;
        key.useClearcoatRoughnessMap = false;
        key.useClearcoatNormalMap = false;
        key.useSheenColorMap = false;
        key.useSheenRoughnessMap = false;
        key.useIridescenceMap = false;
        key.useIridescenceThicknessMap = false;
        key.useAnisotropyMap = false;
        key.useEnvMapEquirect = false;
        key.usePMREM = false;
    }

    // Stability fallback for the v6.0 regression phase:
    // The full PBR/physical fragment path is still under active repair on macOS
    // Core GL.  When it produces an invisible frame, we still want examples and
    // import validation to render visible geometry/textures.  This fallback keeps
    // the normal mesh vertex path, UVs, vertex colors, alpha/map defines, groups,
    // instancing and draw ranges, but uses the robust MeshBasic fragment path.
    // The repaired Standard/Physical safe PBR path is enabled by default.
    // Set THREECPP_DISABLE_EXPERIMENTAL_PBR=1 to force the legacy MeshBasic fallback.
    const bool experimentalPBR = experimental_pbr_enabled();
    const bool safeMeshFallback = !experimentalPBR &&
        (key.primitiveMode == PrimitiveMode::Triangles) &&
        (key.materialType == MaterialType::MeshLambert ||
         key.materialType == MaterialType::MeshPhong ||
         key.materialType == MaterialType::MeshStandard ||
         key.materialType == MaterialType::MeshPhysical);
    if (safeMeshFallback) {
        key.materialType = MaterialType::MeshBasic;
        key.useNormalMap = false;
        key.useBumpMap = false;
        key.useDisplacementMap = false;
        key.useRoughnessMap = false;
        key.useMetalnessMap = false;
        key.useAOMap = false;
        key.useLightMap = false;
        key.useEmissiveMap = false;
        key.useSpecularMap = false;
        key.useIBL = false;
        key.useEnvMapEquirect = false;
        key.usePMREM = false;
        key.useShadowMap = false;
        key.usePhysical = false;
        key.useTransmission = false;
        key.useTransmissionMap = false;
        key.useTransmissionRenderTarget = false;
        key.useThicknessMap = false;
        key.useClearcoat = false;
        key.useClearcoatMap = false;
        key.useClearcoatRoughnessMap = false;
        key.useClearcoatNormalMap = false;
        key.useSheen = false;
        key.useSheenColorMap = false;
        key.useSheenRoughnessMap = false;
        key.useIridescence = false;
        key.useIridescenceMap = false;
        key.useIridescenceThicknessMap = false;
        key.useAnisotropy = false;
        key.useAnisotropyMap = false;
        key.useDispersion = false;
        key.numDirLights = 0;
        key.numPointLights = 0;
        key.numSpotLights = 0;
    }

    if (env_enabled("THREECPP_FORCE_UNLIT")) {
        const PrimitiveMode pm = key.primitiveMode;
        key = ProgramKey{};
        key.materialType = MaterialType::MeshBasic;
        key.primitiveMode = pm;
    }
    return programCache.get(key);
}

void GLRenderer::setCommonUniforms(GLProgram& p, const RenderItem& item, Scene& scene, Camera& camera) {
    glUniformMatrix4fv(p.uniform("modelMatrix"), 1, GL_FALSE, &item.object->matrixWorld[0][0]);
    glUniformMatrix4fv(p.uniform("viewMatrix"), 1, GL_FALSE, &camera.matrixWorldInverse[0][0]);
    glUniformMatrix4fv(p.uniform("projectionMatrix"), 1, GL_FALSE, &camera.projectionMatrix[0][0]);
    glm::mat4 modelView = camera.matrixWorldInverse * item.object->matrixWorld;
    glUniformMatrix4fv(p.uniform("modelViewMatrix"), 1, GL_FALSE, &modelView[0][0]);
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelView)));
    glUniformMatrix3fv(p.uniform("normalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);
    glm::vec3 cameraPosition = glm::vec3(camera.matrixWorld[3]);
    glUniform3fv(p.uniform("cameraPosition"), 1, &cameraPosition[0]);

    glm::vec3 color{1.0f};
    glm::vec3 emissive{0.0f};
    glm::vec2 normalScale{1.0f, 1.0f};
    float roughness = 1.0f;
    float metalness = 0.0f;
    float opacity = item.material->opacity;
    float aoMapIntensity = 1.0f;
    float lightMapIntensity = 1.0f;
    float bumpScale = 1.0f;
    int roughnessChannel = static_cast<int>(TextureChannel::G);
    int metalnessChannel = static_cast<int>(TextureChannel::B);
    int aoChannel = static_cast<int>(TextureChannel::R);
    int alphaChannel = static_cast<int>(TextureChannel::R);
    float displacementScale = 1.0f;
    float displacementBias = 0.0f;
    float envMapIntensity = scene.environment ? scene.environmentIntensity * scene.environment->intensity * scene.environment->envMapIntensity : 0.0f;
    float ior = 1.5f;
    float transmission = 0.0f;
    float thickness = 0.0f;
    float attenuationDistance = std::numeric_limits<float>::infinity();
    glm::vec3 attenuationColor{1.0f};
    float specularIntensity = 1.0f;
    glm::vec3 specularColor{1.0f};
    float clearcoat = 0.0f;
    float clearcoatRoughness = 0.0f;
    glm::vec2 clearcoatNormalScale{1.0f, 1.0f};
    float sheen = 0.0f;
    glm::vec3 sheenColor{0.0f};
    float sheenRoughness = 1.0f;
    float iridescence = 0.0f;
    float iridescenceIOR = 1.3f;
    float iridescenceThicknessMinimum = 100.0f;
    float iridescenceThicknessMaximum = 400.0f;
    float anisotropy = 0.0f;
    float anisotropyRotation = 0.0f;
    float dispersion = 0.0f;
    float dashScale = 1.0f;
    float dashSize = 1.0f;
    float gapSize = 1.0f;
    int unit = 0;
    glm::mat3 uvTransform(1.0f);
    auto useUvTransformFrom = [&](const std::shared_ptr<Texture>& tex) {
        if (tex) uvTransform = tex->uvTransform();
    };

    if (auto* m = dynamic_cast<MeshBasicMaterial*>(item.material)) {
        color = m->color;
        if (m->map) { useUvTransformFrom(m->map); resources.bindTexture2D(m->map, p.uniform("map"), unit++); }
        if (m->alphaMap) resources.bindTexture2D(m->alphaMap, p.uniform("alphaMap"), unit++);
    } else if (auto* m = dynamic_cast<MeshLambertMaterial*>(item.material)) {
        color = m->color;
        emissive = m->emissive * m->emissiveIntensity;
        if (m->map) { useUvTransformFrom(m->map); resources.bindTexture2D(m->map, p.uniform("map"), unit++); }
        lightMapIntensity = m->lightMapIntensity;
        if (m->aoMap) resources.bindTexture2D(m->aoMap, p.uniform("aoMap"), unit++);
        if (m->lightMap) resources.bindTexture2D(m->lightMap, p.uniform("lightMap"), unit++);
        if (m->emissiveMap) resources.bindTexture2D(m->emissiveMap, p.uniform("emissiveMap"), unit++);
    } else if (auto* m = dynamic_cast<MeshPhongMaterial*>(item.material)) {
        color = m->color;
        emissive = m->emissive * m->emissiveIntensity;
        normalScale = {1.0f, 1.0f};
        roughness = glm::clamp(1.0f / std::sqrt(std::max(m->shininess, 1.0f)), 0.045f, 1.0f);
        specularColor = m->specular;
        specularIntensity = 1.0f;
        if (m->map) { useUvTransformFrom(m->map); resources.bindTexture2D(m->map, p.uniform("map"), unit++); }
        if (m->normalMap) resources.bindTexture2D(m->normalMap, p.uniform("normalMap"), unit++, true);
        lightMapIntensity = m->lightMapIntensity;
        if (m->specularMap) resources.bindTexture2D(m->specularMap, p.uniform("specularMap"), unit++);
        if (m->lightMap) resources.bindTexture2D(m->lightMap, p.uniform("lightMap"), unit++);
        if (m->emissiveMap) resources.bindTexture2D(m->emissiveMap, p.uniform("emissiveMap"), unit++);
    } else if (auto* m = dynamic_cast<MeshStandardMaterial*>(item.material)) {
        color = m->color;
        roughness = glm::clamp(m->roughness, 0.045f, 1.0f);
        metalness = glm::clamp(m->metalness, 0.0f, 1.0f);
        emissive = m->emissive * m->emissiveIntensity;
        normalScale = m->normalScale;
        aoMapIntensity = m->aoMapIntensity;
        lightMapIntensity = m->lightMapIntensity;
        displacementScale = m->displacementScale;
        displacementBias = m->displacementBias;
        bumpScale = m->bumpScale;
        roughnessChannel = static_cast<int>(m->roughnessChannel);
        metalnessChannel = static_cast<int>(m->metalnessChannel);
        aoChannel = static_cast<int>(m->aoChannel);
        alphaChannel = static_cast<int>(m->alphaChannel);
        envMapIntensity *= m->envMapIntensity;
        if (m->map) { useUvTransformFrom(m->map); resources.bindTexture2D(m->map, p.uniform("map"), unit++); }
        if (m->alphaMap) resources.bindTexture2D(m->alphaMap, p.uniform("alphaMap"), unit++);
        if (m->normalMap) resources.bindTexture2D(m->normalMap, p.uniform("normalMap"), unit++, true);
        if (m->bumpMap) resources.bindTexture2D(m->bumpMap, p.uniform("bumpMap"), unit++);
        if (m->displacementMap) resources.bindTexture2D(m->displacementMap, p.uniform("displacementMap"), unit++);
        if (m->roughnessMap) resources.bindTexture2D(m->roughnessMap, p.uniform("roughnessMap"), unit++);
        if (m->metalnessMap) resources.bindTexture2D(m->metalnessMap, p.uniform("metalnessMap"), unit++);
        if (m->aoMap) resources.bindTexture2D(m->aoMap, p.uniform("aoMap"), unit++);
        if (m->lightMap) resources.bindTexture2D(m->lightMap, p.uniform("lightMap"), unit++);
        if (m->emissiveMap) resources.bindTexture2D(m->emissiveMap, p.uniform("emissiveMap"), unit++);
        if (auto* phys = dynamic_cast<MeshPhysicalMaterial*>(m)) {
            ior = phys->ior;
            transmission = glm::clamp(phys->transmission, 0.0f, 1.0f);
            thickness = std::max(0.0f, phys->thickness);
            attenuationDistance = phys->attenuationDistance;
            attenuationColor = phys->attenuationColor;
            specularIntensity = phys->specularIntensity;
            specularColor = phys->specularColor;
            clearcoat = glm::clamp(phys->clearcoat, 0.0f, 1.0f);
            clearcoatRoughness = glm::clamp(phys->clearcoatRoughness, 0.0f, 1.0f);
            clearcoatNormalScale = phys->clearcoatNormalScale;
            sheen = glm::clamp(phys->sheen, 0.0f, 1.0f);
            sheenColor = phys->sheenColor;
            sheenRoughness = glm::clamp(phys->sheenRoughness, 0.0f, 1.0f);
            iridescence = glm::clamp(phys->iridescence, 0.0f, 1.0f);
            iridescenceIOR = phys->iridescenceIOR;
            iridescenceThicknessMinimum = phys->iridescenceThicknessMinimum;
            iridescenceThicknessMaximum = phys->iridescenceThicknessMaximum;
            anisotropy = phys->anisotropy;
            anisotropyRotation = phys->anisotropyRotation;
            dispersion = phys->dispersion;
            if (phys->transmissionMap) resources.bindTexture2D(phys->transmissionMap, p.uniform("transmissionMap"), unit++);
            if (phys->thicknessMap) resources.bindTexture2D(phys->thicknessMap, p.uniform("thicknessMap"), unit++);
            if (phys->specularMap || phys->specularIntensityMap || phys->specularColorMap) {
                resources.bindTexture2D(phys->specularIntensityMap ? phys->specularIntensityMap : phys->specularMap, p.uniform("specularIntensityMap"), unit++);
                resources.bindTexture2D(phys->specularColorMap, p.uniform("specularColorMap"), unit++);
            }
            if (phys->clearcoatMap) resources.bindTexture2D(phys->clearcoatMap, p.uniform("clearcoatMap"), unit++);
            if (phys->clearcoatRoughnessMap) resources.bindTexture2D(phys->clearcoatRoughnessMap, p.uniform("clearcoatRoughnessMap"), unit++);
            if (phys->clearcoatNormalMap) resources.bindTexture2D(phys->clearcoatNormalMap, p.uniform("clearcoatNormalMap"), unit++, true);
            if (phys->sheenColorMap) resources.bindTexture2D(phys->sheenColorMap, p.uniform("sheenColorMap"), unit++);
            if (phys->sheenRoughnessMap) resources.bindTexture2D(phys->sheenRoughnessMap, p.uniform("sheenRoughnessMap"), unit++);
            if (phys->iridescenceMap) resources.bindTexture2D(phys->iridescenceMap, p.uniform("iridescenceMap"), unit++);
            if (phys->iridescenceThicknessMap) resources.bindTexture2D(phys->iridescenceThicknessMap, p.uniform("iridescenceThicknessMap"), unit++);
            if (phys->anisotropyMap) resources.bindTexture2D(phys->anisotropyMap, p.uniform("anisotropyMap"), unit++);
        }
    } else if (auto* m = dynamic_cast<LineBasicMaterial*>(item.material)) {
        color = m->color;
        if (auto* dashed = dynamic_cast<LineDashedMaterial*>(m)) {
            dashScale = dashed->scale;
            dashSize = dashed->dashSize;
            gapSize = dashed->gapSize;
        }
        if (m->map) { useUvTransformFrom(m->map); resources.bindTexture2D(m->map, p.uniform("map"), unit++); }
    } else if (auto* m = dynamic_cast<PointsMaterial*>(item.material)) {
        color = m->color;
        glUniform1f(p.uniform("pointSize"), m->size);
        glUniform1f(p.uniform("pointScale"), m->scale);
        if (m->map) { useUvTransformFrom(m->map); resources.bindTexture2D(m->map, p.uniform("map"), unit++); }
        if (m->alphaMap) resources.bindTexture2D(m->alphaMap, p.uniform("alphaMap"), unit++);
    } else if (auto* m = dynamic_cast<FatLineMaterial*>(item.material)) {
        color = m->color;
        dashScale = m->dashScale;
        dashSize = m->dashSize;
        gapSize = m->gapSize;
        glUniform1f(p.uniform("linewidth"), m->linewidth);
        glUniform2f(p.uniform("resolution"), static_cast<float>(params.width), static_cast<float>(params.height));
    }

    glUniform1f(p.uniform("dashScale"), dashScale);
    glUniform1f(p.uniform("dashSize"), dashSize);
    glUniform1f(p.uniform("gapSize"), gapSize);
    glUniformMatrix3fv(p.uniform("uvTransform"), 1, GL_FALSE, &uvTransform[0][0]);
    glUniform3fv(p.uniform("diffuse"), 1, &color[0]);
    glUniform3fv(p.uniform("emissive"), 1, &emissive[0]);
    glUniform1f(p.uniform("roughness"), roughness);
    glUniform1f(p.uniform("metalness"), metalness);
    glUniform1f(p.uniform("opacity"), opacity);
    glUniform1f(p.uniform("alphaTest"), item.material->alphaTest);
    glUniform2fv(p.uniform("normalScale"), 1, &normalScale[0]);
    glUniform1f(p.uniform("aoMapIntensity"), aoMapIntensity);
    glUniform1f(p.uniform("lightMapIntensity"), lightMapIntensity);
    glUniform1f(p.uniform("bumpScale"), bumpScale);
    glUniform1i(p.uniform("roughnessChannel"), roughnessChannel);
    glUniform1i(p.uniform("metalnessChannel"), metalnessChannel);
    glUniform1i(p.uniform("aoChannel"), aoChannel);
    glUniform1i(p.uniform("alphaChannel"), alphaChannel);
    glUniform1f(p.uniform("displacementScale"), displacementScale);
    glUniform1f(p.uniform("displacementBias"), displacementBias);
    glUniform3fv(p.uniform("ambientLightColor"), 1, &lightCache.ambient[0]);
    glUniform3fv(p.uniform("hemisphereSkyColor"), 1, &lightCache.hemisphereSky[0]);
    glUniform3fv(p.uniform("hemisphereGroundColor"), 1, &lightCache.hemisphereGround[0]);
    const glm::vec3 envSky = scene.environment ? scene.environment->skyColor : glm::vec3(0.0f);
    const glm::vec3 envGround = scene.environment ? scene.environment->groundColor : glm::vec3(0.0f);
    const glm::vec3 envSpecular = scene.environment ? scene.environment->specularColor : glm::vec3(0.0f);
    glm::mat3 envRot(1.0f);
    if (scene.environment) envRot = scene.environment->environmentRotation * scene.environment->rotation;
    glUniform3fv(p.uniform("envSkyColor"), 1, &envSky[0]);
    glUniform3fv(p.uniform("envGroundColor"), 1, &envGround[0]);
    glUniform3fv(p.uniform("envSpecularColor"), 1, &envSpecular[0]);
    glUniformMatrix3fv(p.uniform("environmentRotation"), 1, GL_FALSE, &envRot[0][0]);
    glUniform1f(p.uniform("envMapIntensity"), envMapIntensity);
    int pbrIblDebugMode = 0;
    if (env_enabled("THREECPP_DEBUG_PBR_IBL_DIFFUSE")) pbrIblDebugMode = 1;
    if (env_enabled("THREECPP_DEBUG_PBR_IBL_SPECULAR")) pbrIblDebugMode = 2;
    if (env_enabled("THREECPP_DEBUG_PBR_PMREM_LOD")) pbrIblDebugMode = 3;
    glUniform1i(p.uniform("pbrIblDebugMode"), pbrIblDebugMode);
    float pbrPmremSpecularStrength = 1.0f;
    if (const char* v = std::getenv("THREECPP_PBR_PMREM_SPECULAR_STRENGTH")) {
        try { pbrPmremSpecularStrength = std::stof(v); } catch (...) { pbrPmremSpecularStrength = 1.0f; }
    }
    glUniform1f(p.uniform("pbrPmremSpecularStrength"), glm::clamp(pbrPmremSpecularStrength, 0.0f, 4.0f));
    if (scene.environment && scene.environment->equirectangularMap) {
        resources.bindTexture2D(scene.environment->equirectangularMap, p.uniform("envMapEquirect"), unit++);
    }
    if (scene.environment && scene.environment->hasPMREM) {
        resources.bindCubeTexture(scene.environment->irradianceMap, p.uniform("irradianceMap"), unit++);
        resources.bindCubeTexture(scene.environment->prefilterMap, p.uniform("prefilteredEnvMap"), unit++);
        resources.bindTexture2D(scene.environment->brdfLUT, p.uniform("brdfLUT"), unit++);
        glUniform1f(p.uniform("pmremMipLevels"), static_cast<float>(std::max(1, scene.environment->pmremMipLevels)));
    }
    glUniform1f(p.uniform("toneMappingExposure"), params.toneMappingExposure);
    glUniform1i(p.uniform("toneMappingMode"), item.material->toneMapped ? static_cast<int>(params.toneMapping) : static_cast<int>(ToneMapping::None));
    glUniform1i(p.uniform("outputColorSpace"), params.outputColorSpace == ColorSpace::SRGB ? 1 : 0);
    glUniform1i(p.uniform("premultipliedAlpha"), item.material->premultipliedAlpha ? 1 : 0);
    int materialDebugMode = 0;
    if (env_enabled("THREECPP_FORCE_MATERIAL_BASECOLOR")) materialDebugMode = 1;
    if (env_enabled("THREECPP_FORCE_NORMAL_VIEW")) materialDebugMode = 2;
    glUniform1i(p.uniform("materialDebugMode"), materialDebugMode);
    glUniform1f(p.uniform("ior"), ior);
    glUniform1f(p.uniform("transmission"), transmission);
    glUniform1f(p.uniform("thickness"), thickness);
    glUniform1f(p.uniform("attenuationDistance"), attenuationDistance);
    glUniform3fv(p.uniform("attenuationColor"), 1, &attenuationColor[0]);
    glUniform1f(p.uniform("specularIntensity"), specularIntensity);
    glUniform3fv(p.uniform("specularColor"), 1, &specularColor[0]);
    glUniform1f(p.uniform("clearcoat"), clearcoat);
    glUniform1f(p.uniform("clearcoatRoughness"), clearcoatRoughness);
    glUniform2fv(p.uniform("clearcoatNormalScale"), 1, &clearcoatNormalScale[0]);
    glUniform1f(p.uniform("sheen"), sheen);
    glUniform3fv(p.uniform("sheenColor"), 1, &sheenColor[0]);
    glUniform1f(p.uniform("sheenRoughness"), sheenRoughness);
    glUniform1f(p.uniform("iridescence"), iridescence);
    glUniform1f(p.uniform("iridescenceIOR"), iridescenceIOR);
    glUniform1f(p.uniform("iridescenceThicknessMinimum"), iridescenceThicknessMinimum);
    glUniform1f(p.uniform("iridescenceThicknessMaximum"), iridescenceThicknessMaximum);
    glUniform1f(p.uniform("anisotropy"), anisotropy);
    glUniform1f(p.uniform("anisotropyRotation"), anisotropyRotation);
    glUniform1f(p.uniform("dispersion"), dispersion);
    const float transmissionW = transmissionTarget ? static_cast<float>(transmissionTarget->options.width) : static_cast<float>(params.width);
    const float transmissionH = transmissionTarget ? static_cast<float>(transmissionTarget->options.height) : static_cast<float>(params.height);
    glUniform2f(p.uniform("transmissionResolution"), transmissionW, transmissionH);
    glUniform1f(p.uniform("transmissionSamplerMapLevel"), params.transmissionMipLevel);
    int transmissionDebugMode = 0;
    if (env_enabled("THREECPP_DEBUG_TRANSMISSION_TARGET")) transmissionDebugMode = 1;
    if (env_enabled("THREECPP_DEBUG_TRANSMISSION_UV")) transmissionDebugMode = 2;
    if (env_enabled("THREECPP_DEBUG_TRANSMISSION_ATTENUATION")) transmissionDebugMode = 3;
    if (env_enabled("THREECPP_DEBUG_TRANSMISSION_THICKNESS")) transmissionDebugMode = 4;
    if (env_enabled("THREECPP_DEBUG_TRANSMISSION_FRESNEL")) transmissionDebugMode = 5;
    if (env_enabled("THREECPP_DEBUG_TRANSMISSION_BACKFACE_THICKNESS")) transmissionDebugMode = 6;
    glUniform1i(p.uniform("transmissionDebugMode"), transmissionDebugMode);
    float cameraFarForTransmission = 2000.0f;
    if (auto* pc = dynamic_cast<PerspectiveCamera*>(&camera)) cameraFarForTransmission = std::max(0.001f, pc->farPlane);
    else if (auto* oc = dynamic_cast<OrthographicCamera*>(&camera)) cameraFarForTransmission = std::max(0.001f, oc->farPlane);
    glUniform1f(p.uniform("transmissionCameraFar"), cameraFarForTransmission);
    glUniform1i(p.uniform("transmissionUseBackfaceMap"), (transmissionBackfaceTarget && transmissionBackfaceTarget->colorTexture && !env_enabled("THREECPP_DISABLE_TRANSMISSION_BACKFACE")) ? 1 : 0);
    if (transmissionBackfaceTarget && transmissionBackfaceTarget->colorTexture) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, transmissionBackfaceTarget->colorTexture);
        glUniform1i(p.uniform("transmissionBackfaceMap"), unit++);
    }
    if (transmissionTarget && transmissionTarget->colorTexture) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, transmissionTarget->colorTexture);
        glUniform1i(p.uniform("transmissionSamplerMap"), unit++);
    }

    glUniform1i(p.uniform("lightCount"), static_cast<int>(std::min<std::size_t>(lightCache.gpuLights.size(), 16)));
    for (std::size_t i = 0; i < lightCache.gpuLights.size() && i < 16; ++i) {
        const auto& l = lightCache.gpuLights[i];
        const std::string base = "lights[" + std::to_string(i) + "]";
        glUniform1i(p.uniform(base + ".type"), l.type);
        glUniform1i(p.uniform(base + ".shadowIndex"), l.shadowIndex);
        glUniform4fv(p.uniform(base + ".colorIntensity"), 1, &l.colorIntensity[0]);
        glUniform4fv(p.uniform(base + ".positionRange"), 1, &l.positionRange[0]);
        glUniform4fv(p.uniform(base + ".directionCone"), 1, &l.directionCone[0]);
        glUniform4fv(p.uniform(base + ".params"), 1, &l.params[0]);
    }

    // v6.0.29: minimal, standalone directional shadow binding for safe experimental PBR.
    int pbrDirectionalShadowEnabled = 0;
    const ShadowRenderItem* pbrDirectionalShadow = nullptr;
    if (env_enabled("THREECPP_ENABLE_PBR_SHADOWS")) {
        for (const auto& sitem : shadowMap.items) {
            if (!sitem.isPointShadow && sitem.depthTexture && sitem.light && sitem.light->lightType == LightType::Directional) {
                pbrDirectionalShadow = &sitem;
                pbrDirectionalShadowEnabled = 1;
                break;
            }
        }
    }
    glUniform1i(p.uniform("pbrDirectionalShadowEnabled"), pbrDirectionalShadowEnabled);
    glUniform1i(p.uniform("pbrDirectionalShadowDebug"), env_enabled("THREECPP_DEBUG_PBR_DIRECTIONAL_SHADOW") ? 1 : 0);
    if (pbrDirectionalShadow) {
        glUniformMatrix4fv(p.uniform("pbrDirectionalShadowMatrix"), 1, GL_FALSE, &pbrDirectionalShadow->shadowMatrix[0][0]);
        glUniform2f(p.uniform("pbrDirectionalShadowMapSize"), static_cast<float>(pbrDirectionalShadow->mapSizeX), static_cast<float>(pbrDirectionalShadow->mapSizeY));
        glUniform1f(p.uniform("pbrDirectionalShadowBias"), pbrDirectionalShadow->bias);
        glUniform1f(p.uniform("pbrDirectionalShadowRadius"), pbrDirectionalShadow->radius);
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, pbrDirectionalShadow->depthTexture);
        glUniform1i(p.uniform("pbrDirectionalShadowMap"), unit++);
    }

    // v6.0.30: minimal standalone SpotLight shadow binding for safe experimental PBR.
    int pbrSpotShadowEnabled = 0;
    const ShadowRenderItem* pbrSpotShadow = nullptr;
    if (env_enabled("THREECPP_ENABLE_PBR_SHADOWS") && env_enabled("THREECPP_ENABLE_PBR_SPOT_SHADOWS", true)) {
        for (const auto& sitem : shadowMap.items) {
            if (!sitem.isPointShadow && sitem.depthTexture && sitem.light && sitem.light->lightType == LightType::Spot) {
                pbrSpotShadow = &sitem;
                pbrSpotShadowEnabled = 1;
                break;
            }
        }
    }
    glUniform1i(p.uniform("pbrSpotShadowEnabled"), pbrSpotShadowEnabled);
    glUniform1f(p.uniform("pbrSpotShadowStrength"), 1.0f);
    glUniform1i(p.uniform("pbrSpotShadowDebug"), env_enabled("THREECPP_DEBUG_PBR_SPOT_SHADOW") ? 1 : 0);
    if (pbrSpotShadow) {
        glUniformMatrix4fv(p.uniform("pbrSpotShadowMatrix"), 1, GL_FALSE, &pbrSpotShadow->shadowMatrix[0][0]);
        glUniform2f(p.uniform("pbrSpotShadowMapSize"), static_cast<float>(pbrSpotShadow->mapSizeX), static_cast<float>(pbrSpotShadow->mapSizeY));
        glUniform1f(p.uniform("pbrSpotShadowBias"), pbrSpotShadow->bias);
        glUniform1f(p.uniform("pbrSpotShadowRadius"), pbrSpotShadow->radius);
        glUniform1f(p.uniform("pbrSpotShadowStrength"), glm::clamp(pbrSpotShadow->intensity, 0.0f, 1.0f));
        glUniform1i(p.uniform("pbrSpotShadowDebug"), env_enabled("THREECPP_DEBUG_PBR_SPOT_SHADOW") ? 1 : 0);
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, pbrSpotShadow->depthTexture);
        glUniform1i(p.uniform("pbrSpotShadowMap"), unit++);
    }

    // v6.0.32: minimal standalone PointLight cube shadow binding for safe experimental PBR.
    int pbrPointShadowEnabled = 0;
    const ShadowRenderItem* pbrPointShadow = nullptr;
    if (env_enabled("THREECPP_ENABLE_PBR_SHADOWS") && env_enabled("THREECPP_ENABLE_PBR_POINT_SHADOWS", true)) {
        for (const auto& sitem : shadowMap.items) {
            if (sitem.isPointShadow && sitem.depthCubeTexture && sitem.light && sitem.light->lightType == LightType::Point) {
                pbrPointShadow = &sitem;
                pbrPointShadowEnabled = 1;
                break;
            }
        }
    }
    glUniform1i(p.uniform("pbrPointShadowEnabled"), pbrPointShadowEnabled);
    glUniform1f(p.uniform("pbrPointShadowStrength"), 1.0f);
    glUniform1i(p.uniform("pbrPointShadowDebug"), env_enabled("THREECPP_DEBUG_PBR_POINT_SHADOW") ? 1 : 0);
    if (pbrPointShadow) {
        glUniform3fv(p.uniform("pbrPointShadowPosition"), 1, &pbrPointShadow->lightPosition[0]);
        glUniform1f(p.uniform("pbrPointShadowNear"), pbrPointShadow->cameraNear);
        glUniform1f(p.uniform("pbrPointShadowFar"), pbrPointShadow->cameraFar);
        glUniform1f(p.uniform("pbrPointShadowBias"), pbrPointShadow->bias);
        glUniform1f(p.uniform("pbrPointShadowRadius"), pbrPointShadow->radius);
        glUniform2f(p.uniform("pbrPointShadowMapSize"), static_cast<float>(pbrPointShadow->mapSizeX), static_cast<float>(pbrPointShadow->mapSizeY));
        glUniform1f(p.uniform("pbrPointShadowStrength"), glm::clamp(pbrPointShadow->intensity, 0.0f, 1.0f));
        glUniform1i(p.uniform("pbrPointShadowDebug"), env_enabled("THREECPP_DEBUG_PBR_POINT_SHADOW") ? 1 : 0);
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_CUBE_MAP, pbrPointShadow->depthCubeTexture);
        glUniform1i(p.uniform("pbrPointShadowMap"), unit++);
    }

    glUniform1i(p.uniform("shadowMapCount"), static_cast<int>(std::min<std::size_t>(shadowMap.items.size(), 4)));
    for (std::size_t i = 0; i < shadowMap.items.size() && i < 4; ++i) {
        const auto& sitem = shadowMap.items[i];
        glUniformMatrix4fv(p.uniform("shadowMatrix[" + std::to_string(i) + "]"), 1, GL_FALSE, &sitem.shadowMatrix[0][0]);
        glUniform1f(p.uniform("shadowBias[" + std::to_string(i) + "]"), sitem.bias);
        glUniform1f(p.uniform("shadowRadius[" + std::to_string(i) + "]"), sitem.radius);
        glUniform2f(p.uniform("shadowMapSize[" + std::to_string(i) + "]"), static_cast<float>(sitem.mapSizeX), static_cast<float>(sitem.mapSizeY));
        glUniform1i(p.uniform("shadowMapIsPoint[" + std::to_string(i) + "]"), sitem.isPointShadow ? 1 : 0);
        glUniform1f(p.uniform("shadowCameraNear[" + std::to_string(i) + "]"), sitem.cameraNear);
        glUniform1f(p.uniform("shadowCameraFar[" + std::to_string(i) + "]"), sitem.cameraFar);
        glUniform3fv(p.uniform("shadowLightPosition[" + std::to_string(i) + "]"), 1, &sitem.lightPosition[0]);
        if (sitem.isPointShadow && sitem.depthCubeTexture) {
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_CUBE_MAP, sitem.depthCubeTexture);
            glUniform1i(p.uniform("pointShadowMap" + std::to_string(i)), unit++);
        } else if (sitem.depthTexture) {
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_2D, sitem.depthTexture);
            glUniform1i(p.uniform("shadowMap" + std::to_string(i)), unit++);
        }
    }


    if (auto* mesh = dynamic_cast<Mesh*>(item.object)) {
        mesh->syncMorphTargets();
        float influences[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        const int n = std::min<int>(4, static_cast<int>(mesh->morphTargetInfluences.size()));
        for (int i = 0; i < n; ++i) influences[i] = mesh->morphTargetInfluences[static_cast<std::size_t>(i)];
        glUniform1fv(p.uniform("morphTargetInfluences"), 4, influences);
    }

    if (auto* skinned = dynamic_cast<SkinnedMesh*>(item.object); skinned && skinned->skeleton) {
        skinned->skeleton->update();
        auto& bm = skinned->skeleton->boneMatrices;
        if (!bm.empty()) glUniformMatrix4fv(p.uniform("boneMatrices[0]"), static_cast<GLsizei>(std::min<std::size_t>(bm.size(), 128)), GL_FALSE, &bm[0][0][0]);
        glUniformMatrix4fv(p.uniform("bindMatrix"), 1, GL_FALSE, &skinned->bindMatrix[0][0]);
        glUniformMatrix4fv(p.uniform("bindMatrixInverse"), 1, GL_FALSE, &skinned->bindMatrixInverse[0][0]);
    }
}

GLenum GLRenderer::drawMode(PrimitiveMode mode) const {
    switch (mode) {
        case PrimitiveMode::Lines: return GL_LINES;
        case PrimitiveMode::LineStrip: return GL_LINE_STRIP;
        case PrimitiveMode::LineLoop: return GL_LINE_LOOP;
        case PrimitiveMode::Points: return GL_POINTS;
        case PrimitiveMode::FatLines:
        case PrimitiveMode::Triangles:
        default: return GL_TRIANGLES;
    }
}

} // namespace threecpp
