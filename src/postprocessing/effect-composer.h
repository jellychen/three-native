#pragma once
#include "renderer/gl-renderer.h"
#include "renderer/gl-render-target.h"

namespace THREE {

class EffectComposer;

class Pass {
public:
    EffectComposer* composer = nullptr;
    bool enabled = true;
    bool needsSwap = true;
    bool clearBefore = false;
    virtual ~Pass() = default;
    virtual void render(GLRenderer& renderer, Scene& scene, Camera& camera) = 0;
};

class RenderPass : public Pass {
public:
    RenderPass() { needsSwap = true; clearBefore = true; }
    void render(GLRenderer& renderer, Scene& scene, Camera& camera) override {
        renderer.render(scene, camera);
    }
};

class EffectComposer {
public:
    explicit EffectComposer(GLRenderer& renderer) : renderer(renderer) {}
    ~EffectComposer() = default;

    template <class T, class... Args>
    T& addPass(Args&&... args) {
        auto pass = std::make_unique<T>(std::forward<Args>(args)...);
        pass->composer = this;
        auto& ref = *pass;
        passes.push_back(std::move(pass));
        return ref;
    }

    void setSize(int w, int h) {
        w = std::max(1, w);
        h = std::max(1, h);
        sizeW = w; sizeH = h;
        if (internalTarget1) {
            internalTarget1->resize(w, h);
            internalTarget2->resize(w, h);
        }
    }

    GLuint readTexture() const { return readBuf ? readBuf->colorTexture : 0; }
    int width() const { return sizeW; }
    int height() const { return sizeH; }

    void render(Scene& scene, Camera& camera) {
        int w = std::max(1, sizeW);
        int h = std::max(1, sizeH);

        if (!internalTarget1) {
            GLRenderTargetOptions opts{w, h, false, false, false};
            internalTarget1 = std::make_unique<GLRenderTarget>(opts);
            internalTarget2 = std::make_unique<GLRenderTarget>(opts);
            readBuf = internalTarget2.get();
            writeBuf = internalTarget1.get();
        }

        for (auto& pass : passes) {
            if (!pass->enabled) continue;
            if (pass->needsSwap && writeBuf) {
                // Use setRenderTarget so renderer.render() redirects to our target
                renderer.setRenderTarget(writeBuf);
                glViewport(0, 0, writeBuf->options.width, writeBuf->options.height);
                if (pass->clearBefore) {
                    glClearColor(0, 0, 0, 0);
                    glClear(GL_COLOR_BUFFER_BIT);
                }
            }
            pass->render(renderer, scene, camera);
            if (pass->needsSwap) {
                // Unbind our target and swap buffers
                renderer.setRenderTarget(nullptr);
                GLint prev = 0;
                glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev);
                std::swap(readBuf, writeBuf);
            }
        }

        // Blit final read buffer to screen
        if (readBuf && readBuf->valid()) {
            GLint prevFbo = 0;
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, readBuf->framebuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(0, 0, readBuf->options.width, readBuf->options.height,
                              0, 0, w, h,
                              GL_COLOR_BUFFER_BIT, GL_LINEAR);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, prevFbo);
        }
    }

private:
    GLRenderer& renderer;
    std::vector<std::unique_ptr<Pass>> passes;
    std::unique_ptr<GLRenderTarget> internalTarget1;
    std::unique_ptr<GLRenderTarget> internalTarget2;
    GLRenderTarget* readBuf = nullptr;
    GLRenderTarget* writeBuf = nullptr;
    int sizeW = 1280;
    int sizeH = 720;
};

} // namespace THREE
