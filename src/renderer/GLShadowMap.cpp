#include "renderer/GLShadowMap.hpp"
#include "shader/ShaderLib.hpp"

namespace threecpp {

namespace {
static bool intersect_shadow_range(const BufferGeometry& geometry, int start, int count, int total, int& outStart, int& outCount) {
    const int drawStart = std::max(0, geometry.drawRange.start);
    const int drawLimit = geometry.drawRange.count == std::numeric_limits<int>::max() ? total : std::max(0, geometry.drawRange.count);
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

GLShadowMap::~GLShadowMap() {
    for (auto& [_, t] : targets) {
        if (t.framebuffer) glDeleteFramebuffers(1, &t.framebuffer);
        if (t.depthTexture) glDeleteTextures(1, &t.depthTexture);
    }
    for (auto& [_, t] : cubeTargets) {
        if (t.framebuffer) glDeleteFramebuffers(1, &t.framebuffer);
        if (t.depthCubeTexture) glDeleteTextures(1, &t.depthCubeTexture);
    }
}

GLShadowMap::ShadowTarget& GLShadowMap::ensureTarget(ObjectId lightId, int w, int h) {
    auto& t = targets[lightId];
    if (t.framebuffer && t.depthTexture && t.width == w && t.height == h) return t;
    if (t.framebuffer) glDeleteFramebuffers(1, &t.framebuffer);
    if (t.depthTexture) glDeleteTextures(1, &t.depthTexture);
    t.width = w; t.height = h;
    glGenTextures(1, &t.depthTexture);
    glBindTexture(GL_TEXTURE_2D, t.depthTexture);
#if THREECPP_USE_ANGLE
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
#endif
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &t.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, t.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, t.depthTexture, 0);
#if !THREECPP_USE_ANGLE
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
#endif
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("GLShadowMap: incomplete depth framebuffer");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return t;
}


GLShadowMap::CubeShadowTarget& GLShadowMap::ensureCubeTarget(ObjectId lightId, int size) {
    auto& t = cubeTargets[lightId];
    if (t.framebuffer && t.depthCubeTexture && t.size == size) return t;
    if (t.framebuffer) glDeleteFramebuffers(1, &t.framebuffer);
    if (t.depthCubeTexture) glDeleteTextures(1, &t.depthCubeTexture);
    t.size = size;
    glGenTextures(1, &t.depthCubeTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, t.depthCubeTexture);
    for (int face = 0; face < 6; ++face) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_DEPTH_COMPONENT24, size, size, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#if !THREECPP_USE_ANGLE
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
#endif
    glGenFramebuffers(1, &t.framebuffer);
    return t;
}

void GLShadowMap::buildShadowItems(Scene& scene) {
    items.clear();
    scene.traverseVisible([&](Object3D& o) {
        auto* light = dynamic_cast<Light*>(&o);
        if (!light || !light->castShadow || !light->shadow.enabled) return;
        if (light->lightType != LightType::Directional && light->lightType != LightType::Spot && light->lightType != LightType::Point) return;
        ShadowRenderItem item;
        item.light = light;
        const int resolvedMapSizeX = (light->shadow.mapSize.x != 1024 || light->shadow.mapSize.y != 1024)
            ? light->shadow.mapSize.x
            : light->shadow.mapSizeX;
        const int resolvedMapSizeY = (light->shadow.mapSize.x != 1024 || light->shadow.mapSize.y != 1024)
            ? light->shadow.mapSize.y
            : light->shadow.mapSizeY;
        item.mapSizeX = std::max(1, resolvedMapSizeX);
        item.mapSizeY = std::max(1, resolvedMapSizeY);
        item.bias = light->shadow.bias;
        item.normalBias = light->shadow.normalBias;
        item.radius = light->shadow.radius;
        item.intensity = light->shadow.intensity;
        item.cameraNear = std::max(0.001f, light->shadow.cameraNear);
        item.cameraFar = std::max(item.cameraNear + 0.001f, light->shadow.cameraFar);
        item.lightPosition = glm::vec3(light->matrixWorld[3]);
        item.isPointShadow = light->lightType == LightType::Point;
        if (item.isPointShadow) {
            if (auto* point = dynamic_cast<PointLight*>(light)) {
                if (point->distance > 0.0f) item.cameraFar = point->distance;
                item.pointFaceViewProjection = computePointLightViewProjections(*point, item.cameraNear, item.cameraFar);
            }
            auto& target = ensureCubeTarget(light->id, std::max(item.mapSizeX, item.mapSizeY));
            item.framebuffer = target.framebuffer;
            item.depthCubeTexture = target.depthCubeTexture;
            item.mapSizeX = target.size; item.mapSizeY = target.size;
            // Keep matrix populated for debug tools; point shadows use cube faces.
            item.lightViewProjection = item.pointFaceViewProjection[0];
            item.shadowMatrix = biasMatrix() * item.lightViewProjection;
        } else {
            item.lightViewProjection = computeLightViewProjection(*light);
            item.shadowMatrix = biasMatrix() * item.lightViewProjection;
            auto& target = ensureTarget(light->id, item.mapSizeX, item.mapSizeY);
            item.framebuffer = target.framebuffer;
            item.depthTexture = target.depthTexture;
        }
        light->shadow.matrix = item.shadowMatrix;
        items.push_back(item);
    });
}

GLProgram& GLShadowMap::depthProgram(ProgramCache& programs, const RenderableObject& object) {
    ProgramKey key;
    key.materialType = MaterialType::Depth;
    key.primitiveMode = PrimitiveMode::Triangles;
    key.useInstancing = dynamic_cast<const InstancedMesh*>(&object) != nullptr;
    key.useInstanceColor = false;
    key.useSkinning = object.kind == ObjectKind::SkinnedMesh && object.geometry && object.geometry->hasAttribute("skinIndex") && object.geometry->hasAttribute("skinWeight");
    if (object.geometry) {
        key.useMorphTargets = object.geometry->morphTargetCount("position") > 0;
        key.morphTargetsRelative = object.geometry->morphTargetsRelative;
        key.morphTargetCount = std::min(4, object.geometry->morphTargetCount("position"));
    }
    return programs.get(key);
}

void GLShadowMap::renderDepthObject(RenderableObject& object, const glm::mat4& lightVP, GLResourceManager& resources, GLProgram& program, int drawStart, int drawCount) {
    if (!object.geometry) return;
    glUseProgram(program.id);
    glUniformMatrix4fv(program.uniform("modelMatrix"), 1, GL_FALSE, &object.matrixWorld[0][0]);
    glUniformMatrix4fv(program.uniform("lightViewProjectionMatrix"), 1, GL_FALSE, &lightVP[0][0]);
    if (auto* mesh = dynamic_cast<Mesh*>(&object)) {
        mesh->syncMorphTargets();
        float influences[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        const int n = std::min<int>(4, static_cast<int>(mesh->morphTargetInfluences.size()));
        for (int i = 0; i < n; ++i) influences[i] = mesh->morphTargetInfluences[static_cast<std::size_t>(i)];
        glUniform1fv(program.uniform("morphTargetInfluences"), 4, influences);
    }
    if (auto* skinned = dynamic_cast<SkinnedMesh*>(&object); skinned && skinned->skeleton) {
        skinned->skeleton->update();
        auto& bm = skinned->skeleton->boneMatrices;
        if (!bm.empty()) glUniformMatrix4fv(program.uniform("boneMatrices[0]"), static_cast<GLsizei>(std::min<std::size_t>(bm.size(), 128)), GL_FALSE, &bm[0][0][0]);
        glUniformMatrix4fv(program.uniform("bindMatrix"), 1, GL_FALSE, &skinned->bindMatrix[0][0]);
        glUniformMatrix4fv(program.uniform("bindMatrixInverse"), 1, GL_FALSE, &skinned->bindMatrixInverse[0][0]);
    }
    auto& glg = resources.getOrCreateGeometry(*object.geometry);
    glBindVertexArray(glg.vao);
    const int totalCount = glg.hasIndex ? glg.indexCount : glg.vertexCount;
    const int first = glm::clamp(drawStart, 0, std::max(0, totalCount));
    const int requested = drawCount < 0 ? (totalCount - first) : drawCount;
    const int count = glm::clamp(requested, 0, std::max(0, totalCount - first));
    if (count > 0) {
        const void* indexOffset = reinterpret_cast<const void*>(static_cast<std::uintptr_t>(first) * sizeof(std::uint32_t));
        if (auto* inst = dynamic_cast<InstancedMesh*>(&object)) {
            const int drawInstances = std::max(0, inst->count);
            resources.updateInstancedAttributes(*inst);
            if (drawInstances > 0) {
                if (glg.hasIndex) glDrawElementsInstanced(GL_TRIANGLES, count, GL_UNSIGNED_INT, indexOffset, drawInstances);
                else glDrawArraysInstanced(GL_TRIANGLES, first, count, drawInstances);
            }
        } else {
            if (glg.hasIndex) glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, indexOffset);
            else glDrawArrays(GL_TRIANGLES, first, count);
        }
    }
    glBindVertexArray(0);
}

void GLShadowMap::render(Scene& scene, GLResourceManager& resources, ProgramCache& programs) {
    if (!enabled) return;

    // Save default framebuffer state BEFORE buildShadowItems(). ensureTarget()
    // may bind depth-only FBOs and call glDrawBuffer(GL_NONE) / glReadBuffer(GL_NONE)
    // while allocating shadow targets. On macOS core profile this draw-buffer
    // state can leak back to the default framebuffer and make the following
    // forward pass render only the clear/background, with all meshes invisible.
    GLint oldViewport[4];
    GLint oldFramebuffer = 0;
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFramebuffer);
#if !THREECPP_USE_ANGLE
    GLint oldDrawBuffer = GL_BACK;
    GLint oldReadBuffer = GL_BACK;
    glGetIntegerv(GL_DRAW_BUFFER, &oldDrawBuffer);
    glGetIntegerv(GL_READ_BUFFER, &oldReadBuffer);
#endif

    buildShadowItems(scene);
    if (items.empty()) {
        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(oldFramebuffer));
        glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
#if !THREECPP_USE_ANGLE
        glDrawBuffer(static_cast<GLenum>(oldDrawBuffer));
        glReadBuffer(static_cast<GLenum>(oldReadBuffer));
#endif
        return;
    }

    GLboolean wasCullEnabled = glIsEnabled(GL_CULL_FACE);
    GLint oldCullFace = GL_BACK;
    glGetIntegerv(GL_CULL_FACE_MODE, &oldCullFace);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    for (const auto& item : items) {
        glBindFramebuffer(GL_FRAMEBUFFER, item.framebuffer);
        glViewport(0, 0, item.mapSizeX, item.mapSizeY);
        const int faceCount = item.isPointShadow ? 6 : 1;
        for (int face = 0; face < faceCount; ++face) {
            if (item.isPointShadow) {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, item.depthCubeTexture, 0);
#if !THREECPP_USE_ANGLE
                glDrawBuffer(GL_NONE);
                glReadBuffer(GL_NONE);
#endif
            }
            glClear(GL_DEPTH_BUFFER_BIT);
            const glm::mat4 lightVP = item.isPointShadow ? item.pointFaceViewProjection[face] : item.lightViewProjection;
            scene.traverseVisible([&](Object3D& o) {
                auto* mesh = dynamic_cast<Mesh*>(&o);
                auto* renderable = dynamic_cast<RenderableObject*>(&o);
                if (!mesh || !renderable || !mesh->castShadow || !renderable->geometry || !renderable->material) return;
                if (renderable->material->transparent && renderable->material->alphaTest <= 0.0f) return;
                GLProgram& program = depthProgram(programs, *renderable);
                const int total = !renderable->geometry->indices.empty() ? renderable->geometry->indexCount() : renderable->geometry->vertexCount();
                if (!renderable->geometry->groups.empty()) {
                    for (const auto& group : renderable->geometry->groups) {
                        Material* mat = renderable->materialAt(group.materialIndex);
                        if (mat && mat->transparent && mat->alphaTest <= 0.0f) continue;
                        int start = 0, count = 0;
                        if (!intersect_shadow_range(*renderable->geometry, group.start, group.count, total, start, count)) continue;
                        renderDepthObject(*renderable, lightVP, resources, program, start, count);
                    }
                } else {
                    int start = 0, count = 0;
                    if (!intersect_shadow_range(*renderable->geometry, 0, total, total, start, count)) return;
                    renderDepthObject(*renderable, lightVP, resources, program, start, count);
                }
            });
        }
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glCullFace(static_cast<GLenum>(oldCullFace));
    if (!wasCullEnabled) glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(oldFramebuffer));
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
#if !THREECPP_USE_ANGLE
    glDrawBuffer(static_cast<GLenum>(oldDrawBuffer));
    glReadBuffer(static_cast<GLenum>(oldReadBuffer));
#endif
}

glm::mat4 GLShadowMap::biasMatrix() {
    return glm::mat4(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f);
}

glm::mat4 GLShadowMap::computeLightViewProjection(const Light& light) {
    glm::vec3 pos = glm::vec3(light.matrixWorld[3]);
    glm::vec3 target{0.0f};
    float nearPlane = 0.1f;
    float farPlane = 200.0f;
    if (auto* d = dynamic_cast<const DirectionalLight*>(&light)) {
        target = d->target;
        glm::vec3 up = std::abs(glm::dot(glm::normalize(target - pos), glm::vec3(0,1,0))) > 0.98f ? glm::vec3(0,0,1) : glm::vec3(0,1,0);
        glm::mat4 view = glm::lookAt(pos, target, up);
        nearPlane = std::max(0.001f, light.shadow.cameraNear);
        farPlane = std::max(nearPlane + 0.001f, light.shadow.cameraFar);
        glm::mat4 proj = glm::ortho(light.shadow.cameraLeft, light.shadow.cameraRight, light.shadow.cameraBottom, light.shadow.cameraTop, nearPlane, farPlane);
        return proj * view;
    }
    if (auto* s = dynamic_cast<const SpotLight*>(&light)) {
        target = s->target;
        glm::vec3 up = std::abs(glm::dot(glm::normalize(target - pos), glm::vec3(0,1,0))) > 0.98f ? glm::vec3(0,0,1) : glm::vec3(0,1,0);
        glm::mat4 view = glm::lookAt(pos, target, up);
        nearPlane = std::max(0.001f, light.shadow.cameraNear);
        farPlane = s->distance > 0.0f ? s->distance : std::max(nearPlane + 0.001f, light.shadow.cameraFar);
        glm::mat4 proj = glm::perspective(glm::clamp(s->angle * 2.0f, 0.01f, glm::pi<float>() - 0.01f), 1.0f, nearPlane, farPlane);
        return proj * view;
    }
    glm::mat4 view = glm::lookAt(pos, pos + glm::vec3(0, -1, 0), glm::vec3(0, 0, 1));
    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);
    return proj * view;
}

std::array<glm::mat4, 6> GLShadowMap::computePointLightViewProjections(const PointLight& light, float nearPlane, float farPlane) {
    glm::vec3 pos = glm::vec3(light.matrixWorld[3]);
    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);
    return {
        proj * glm::lookAt(pos, pos + glm::vec3( 1, 0, 0), glm::vec3(0,-1, 0)),
        proj * glm::lookAt(pos, pos + glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
        proj * glm::lookAt(pos, pos + glm::vec3( 0, 1, 0), glm::vec3(0, 0, 1)),
        proj * glm::lookAt(pos, pos + glm::vec3( 0,-1, 0), glm::vec3(0, 0,-1)),
        proj * glm::lookAt(pos, pos + glm::vec3( 0, 0, 1), glm::vec3(0,-1, 0)),
        proj * glm::lookAt(pos, pos + glm::vec3( 0, 0,-1), glm::vec3(0,-1, 0))
    };
}

} // namespace threecpp
