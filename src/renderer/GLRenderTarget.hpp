#pragma once
#include "common.hpp"
#include "platform/gl_headers.hpp"

namespace threecpp {

struct GLRenderTargetOptions {
    int width = 1;
    int height = 1;
    bool depth = true;
    bool stencil = false;
    bool generateMipmaps = true;
};

class GLRenderTarget {
public:
    GLRenderTargetOptions options;
    GLuint framebuffer = 0;
    GLuint colorTexture = 0;
    GLuint depthRenderbuffer = 0;

    GLRenderTarget() = default;
    explicit GLRenderTarget(GLRenderTargetOptions opts);
    GLRenderTarget(const GLRenderTarget&) = delete;
    GLRenderTarget& operator=(const GLRenderTarget&) = delete;
    GLRenderTarget(GLRenderTarget&& other) noexcept;
    GLRenderTarget& operator=(GLRenderTarget&& other) noexcept;
    ~GLRenderTarget();

    void resize(int w, int h);
    void bind();
    void unbind();
    void generateMipmaps();
    bool valid() const { return framebuffer != 0 && colorTexture != 0; }

private:
    void allocate();
    void release();
};

} // namespace threecpp
