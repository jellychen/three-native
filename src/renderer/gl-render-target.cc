#include "renderer/gl-render-target.h"

namespace THREE {

GLRenderTarget::GLRenderTarget(GLRenderTargetOptions opts) : options(opts) { allocate(); }

GLRenderTarget::GLRenderTarget(GLRenderTarget&& other) noexcept { *this = std::move(other); }

GLRenderTarget& GLRenderTarget::operator=(GLRenderTarget&& other) noexcept {
    if (this == &other) return *this;
    release();
    options = other.options;
    framebuffer = std::exchange(other.framebuffer, 0);
    colorTexture = std::exchange(other.colorTexture, 0);
    depthRenderbuffer = std::exchange(other.depthRenderbuffer, 0);
    return *this;
}

GLRenderTarget::~GLRenderTarget() { release(); }

void GLRenderTarget::release() {
    if (depthRenderbuffer) glDeleteRenderbuffers(1, &depthRenderbuffer);
    if (colorTexture) glDeleteTextures(1, &colorTexture);
    if (framebuffer) glDeleteFramebuffers(1, &framebuffer);
    depthRenderbuffer = 0;
    colorTexture = 0;
    framebuffer = 0;
}

void GLRenderTarget::allocate() {
    release();
    options.width = std::max(1, options.width);
    options.height = std::max(1, options.height);

    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, options.width, options.height, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, options.generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

    if (options.depth) {
        glGenRenderbuffers(1, &depthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, options.stencil ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT24, options.width, options.height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, options.stencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        release();
        throw std::runtime_error("GLRenderTarget framebuffer is incomplete");
    }
}

void GLRenderTarget::resize(int w, int h) {
    w = std::max(1, w);
    h = std::max(1, h);
    if (w == options.width && h == options.height) return;
    options.width = w;
    options.height = h;
    allocate();
}

void GLRenderTarget::bind() { glBindFramebuffer(GL_FRAMEBUFFER, framebuffer); }
void GLRenderTarget::unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void GLRenderTarget::generateMipmaps() {
    if (!options.generateMipmaps || !colorTexture) return;
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glGenerateMipmap(GL_TEXTURE_2D);
}

} // namespace THREE
