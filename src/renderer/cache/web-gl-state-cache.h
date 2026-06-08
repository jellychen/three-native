#pragma once
#include "common.h"
#include "material/material.h"
#include "platform/gl-headers.h"

namespace THREE {

struct WebGLStateCacheStats {
    int stateChanges = 0;
    int redundantDepthTest = 0;
    int redundantDepthWrite = 0;
    int redundantBlend = 0;
    int redundantCull = 0;
    int redundantProgram = 0;
    int redundantFramebuffer = 0;
};

class WebGLStateCache {
public:
    void reset() {
        currentProgram = 0;
        currentFramebuffer = 0;
        depthTest.reset();
        depthWrite.reset();
        blend.reset();
        cullFace.reset();
        viewport.reset();
        scissor.reset();
        stats = {};
    }

    void useProgram(GLuint p) {
        if (currentProgram == p) { ++stats.redundantProgram; return; }
        currentProgram = p;
        ++stats.stateChanges;
        glUseProgram(p);
    }

    void bindFramebuffer(GLenum target, GLuint fbo) {
        if (currentFramebuffer == fbo) { ++stats.redundantFramebuffer; return; }
        currentFramebuffer = fbo;
        ++stats.stateChanges;
        glBindFramebuffer(target, fbo);
    }

    void setDepthTest(bool enabled) {
        if (depthTest && *depthTest == enabled) { ++stats.redundantDepthTest; return; }
        depthTest = enabled;
        ++stats.stateChanges;
        enabled ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
    }

    void setDepthWrite(bool enabled) {
        if (depthWrite && *depthWrite == enabled) { ++stats.redundantDepthWrite; return; }
        depthWrite = enabled;
        ++stats.stateChanges;
        glDepthMask(enabled ? GL_TRUE : GL_FALSE);
    }

    void setBlend(bool enabled) {
        if (blend && *blend == enabled) { ++stats.redundantBlend; return; }
        blend = enabled;
        ++stats.stateChanges;
        enabled ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
    }

    void setCullFace(bool enabled) {
        if (cullFace && *cullFace == enabled) { ++stats.redundantCull; return; }
        cullFace = enabled;
        ++stats.stateChanges;
        enabled ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    }

    void setViewport(int x, int y, int w, int h) {
        const glm::ivec4 v{x, y, w, h};
        if (viewport && *viewport == v) return;
        viewport = v;
        ++stats.stateChanges;
        glViewport(x, y, w, h);
    }

    void setScissor(int x, int y, int w, int h) {
        const glm::ivec4 s{x, y, w, h};
        if (scissor && *scissor == s) return;
        scissor = s;
        ++stats.stateChanges;
        glScissor(x, y, w, h);
    }

    const WebGLStateCacheStats& getStats() const { return stats; }

private:
    GLuint currentProgram = 0;
    GLuint currentFramebuffer = 0;
    std::optional<bool> depthTest;
    std::optional<bool> depthWrite;
    std::optional<bool> blend;
    std::optional<bool> cullFace;
    std::optional<glm::ivec4> viewport;
    std::optional<glm::ivec4> scissor;
    WebGLStateCacheStats stats;
};

} // namespace THREE
