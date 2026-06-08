#pragma once
#include "postprocessing/effect-composer.h"
#include <iostream>
#include <cstring>

namespace THREE {

namespace {

// Shared full-screen quad vertex shader
const char* fsQuadVertex = R"GLSL(
#version 330
layout(location=0) in vec2 position;
out vec2 vUv;
void main() {
    vUv = position * 0.5 + 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
}
)GLSL";

// Compile + link a full-screen quad shader program
static GLuint createPPProgram(const char* vertSrc, const char* fragSrc) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertSrc, nullptr);
    glCompileShader(vs);
    GLint ok = 0;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]; GLsizei len = 0;
        glGetShaderInfoLog(vs, 1024, &len, log);
        std::cerr << "[threecpp] PP vertex compile: " << log << "\n";
        glDeleteShader(vs); return 0;
    }
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragSrc, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]; GLsizei len = 0;
        glGetShaderInfoLog(fs, 1024, &len, log);
        std::cerr << "[threecpp] PP frag compile: " << log << "\n";
        glDeleteShader(vs); glDeleteShader(fs); return 0;
    }
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024]; GLsizei len = 0;
        glGetProgramInfoLog(prog, 1024, &len, log);
        std::cerr << "[threecpp] PP link: " << log << "\n";
        glDeleteShader(vs); glDeleteShader(fs); glDeleteProgram(prog); return 0;
    }
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

// Convenience: create a program using the shared full-screen quad vertex shader
static GLuint createPPProgramFromFrag(const char* fragSrc) {
    return createPPProgram(fsQuadVertex, fragSrc);
}

static void bindTex(GLuint prog, const char* name, int unit, GLuint tex) {
    if (!prog) return;
    glUniform1i(glGetUniformLocation(prog, name), unit);
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, tex);
}

} // anonymous namespace

// -------------------------------------------------------------------------
// ShaderPass
// -------------------------------------------------------------------------
class ShaderPass : public Pass {
public:
    std::string name = "ShaderPass";
    std::shared_ptr<ShaderMaterial> material;
    std::string fragmentSrc;
    explicit ShaderPass(std::shared_ptr<ShaderMaterial> mat = {}) : material(std::move(mat)) { clearBefore = true; }

    void render(GLRenderer& renderer, Scene& scene, Camera& camera) override {
        GLuint inputTex = composer ? composer->readTexture() : 0;
        if (!inputTex) { renderer.render(scene, camera); return; }

        if (material && !fragmentSrc.empty()) {
            GLuint prog = 0;
            auto it = shaderCache.find(fragmentSrc);
            if (it != shaderCache.end()) prog = it->second;
            else { prog = createPPProgramFromFrag(fragmentSrc.c_str()); shaderCache[fragmentSrc] = prog; }
            if (!prog) { renderer.render(scene, camera); return; }
            glUseProgram(prog);
            bindTex(prog, "inputTexture", 0, inputTex);
        } else {
            if (!passthroughProg)
                passthroughProg = createPPProgramFromFrag(R"GLSL(
#version 330
in vec2 vUv;
uniform sampler2D inputTexture;
out vec4 fragColor;
void main() { fragColor = texture(inputTexture, vUv); }
)GLSL");
            if (!passthroughProg) { renderer.render(scene, camera); return; }
            glUseProgram(passthroughProg);
            bindTex(passthroughProg, "inputTexture", 0, inputTex);
        }
        renderer.renderFullScreenQuad();
        glUseProgram(0);
    }

private:
    GLuint passthroughProg = 0;
    std::unordered_map<std::string, GLuint> shaderCache;
};

// -------------------------------------------------------------------------
// ToneMappingPass
// -------------------------------------------------------------------------
class ToneMappingPass : public Pass {
public:
    ToneMapping toneMapping = ToneMapping::ACESFilmic;
    float exposure = 1.0f;
    ToneMappingPass(ToneMapping mode = ToneMapping::ACESFilmic, float exp = 1.0f)
        : toneMapping(mode), exposure(exp) { clearBefore = true; }

    void render(GLRenderer& renderer, Scene& scene, Camera& camera) override {
        GLuint inputTex = composer ? composer->readTexture() : 0;
        if (!inputTex) { renderer.render(scene, camera); return; }
        ensureProg();
        if (!prog) { renderer.render(scene, camera); return; }
        glUseProgram(prog);
        bindTex(prog, "inputTexture", 0, inputTex);
        glUniform1f(glGetUniformLocation(prog, "exposure"), exposure);
        glUniform1i(glGetUniformLocation(prog, "toneMappingMode"), static_cast<int>(toneMapping));
        renderer.renderFullScreenQuad();
        glUseProgram(0);
    }

private:
    GLuint prog = 0;
    void ensureProg() {
        if (prog) return;
        prog = createPPProgramFromFrag(R"GLSL(
#version 330
in vec2 vUv;
uniform sampler2D inputTexture;
uniform float exposure;
uniform int toneMappingMode;
out vec4 fragColor;

vec3 applyToneMap(vec3 c) {
    c *= exposure;
    if (toneMappingMode == 1) { // Reinhard
        return c / (c + vec3(1.0));
    } else if (toneMappingMode == 2) { // Cineon
        vec3 x = max(vec3(0.0), c - 0.004);
        return (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
    } else if (toneMappingMode == 3) { // ACESFilmic (default)
        float a = 2.51; float b = 0.03; float c2 = 2.43; float d = 0.59; float e = 0.14;
        return clamp((c * (a * c + b)) / (c * (c2 * c + d) + e), 0.0, 1.0);
    }
    return clamp(c, 0.0, 1.0);
}
void main() {
    vec4 color = texture(inputTexture, vUv);
    fragColor = vec4(applyToneMap(color.rgb), color.a);
}
)GLSL");
    }
};

// -------------------------------------------------------------------------
// BloomPass
// -------------------------------------------------------------------------
class BloomPass : public Pass {
public:
    float threshold = 1.0f;
    float strength = 0.35f;
    float radius = 0.35f;

    BloomPass(float th = 1.0f, float st = 0.35f, float ra = 0.35f)
        : threshold(th), strength(st), radius(ra) { clearBefore = true; }

    void render(GLRenderer& renderer, Scene& scene, Camera& camera) override {
        if (!composer) { renderer.render(scene, camera); return; }
        GLuint inputTex = composer->readTexture();
        int w = composer->width();
        int h = composer->height();
        if (!inputTex || w < 1 || h < 1) { renderer.render(scene, camera); return; }

        // Save the composer's write target for the composite stage
        GLint writeFbo = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &writeFbo);

        if (!tempTarget) {
            tempTarget = std::make_unique<GLRenderTarget>(GLRenderTargetOptions{w, h, false, false, false});
            hTarget = std::make_unique<GLRenderTarget>(GLRenderTargetOptions{w, h, false, false, false});
        } else {
            tempTarget->resize(w, h); hTarget->resize(w, h);
        }
        ensureShaders();

        // Stage 1: Extract brights → tempTarget
        tempTarget->bind();
        glViewport(0, 0, w, h);
        glClearColor(0,0,0,0); glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(brightExtractProg);
        glUniform1f(glGetUniformLocation(brightExtractProg, "threshold"), threshold);
        bindTex(brightExtractProg, "inputTexture", 0, inputTex);
        renderer.renderFullScreenQuad();
        tempTarget->unbind();

        // Stage 2a: Blur H → hTarget
        hTarget->bind();
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(blurHProg);
        glUniform2f(glGetUniformLocation(blurHProg, "texelSize"), 1.0f / w, 1.0f / h);
        bindTex(blurHProg, "inputTexture", 0, tempTarget->colorTexture);
        renderer.renderFullScreenQuad();
        hTarget->unbind();

        // Stage 2b: Blur V → tempTarget
        tempTarget->bind();
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(blurVProg);
        glUniform2f(glGetUniformLocation(blurVProg, "texelSize"), 1.0f / w, 1.0f / h);
        bindTex(blurVProg, "inputTexture", 0, hTarget->colorTexture);
        renderer.renderFullScreenQuad();
        tempTarget->unbind();

        // Stage 3: Composite → composer's write target
        glBindFramebuffer(GL_FRAMEBUFFER, writeFbo);
        glViewport(0, 0, w, h);
        glUseProgram(compositeProg);
        glUniform1f(glGetUniformLocation(compositeProg, "strength"), strength);
        bindTex(compositeProg, "originalTexture", 0, inputTex);
        bindTex(compositeProg, "bloomTexture", 1, tempTarget->colorTexture);
        renderer.renderFullScreenQuad();
        glUseProgram(0);
    }

private:
    std::unique_ptr<GLRenderTarget> tempTarget;
    std::unique_ptr<GLRenderTarget> hTarget;
    GLuint brightExtractProg = 0;
    GLuint blurHProg = 0;
    GLuint blurVProg = 0;
    GLuint compositeProg = 0;

    void ensureShaders() {
        if (brightExtractProg) return;
        brightExtractProg = createPPProgramFromFrag(R"GLSL(
#version 330
in vec2 vUv;
uniform sampler2D inputTexture;
uniform float threshold;
out vec4 fragColor;
void main() {
    vec4 c = texture(inputTexture, vUv);
    float l = dot(c.rgb, vec3(0.2126, 0.7152, 0.0722));
    float amount = max(l - threshold, 0.0) / max(1.0 - threshold, 0.0001);
    fragColor = vec4(c.rgb * amount, c.a);
}
)GLSL");
        blurHProg = createPPProgramFromFrag(R"GLSL(
#version 330
in vec2 vUv;
uniform sampler2D inputTexture;
uniform vec2 texelSize;
out vec4 fragColor;
void main() {
    vec4 c = texture(inputTexture, vUv) * 0.227;
    vec2 off = vec2(texelSize.x, 0.0);
    c += texture(inputTexture, vUv+off) * 0.090;
    c += texture(inputTexture, vUv-off) * 0.090;
    off.x = texelSize.x * 2.0;
    c += texture(inputTexture, vUv+off) * 0.054;
    c += texture(inputTexture, vUv-off) * 0.054;
    off.x = texelSize.x * 3.0;
    c += texture(inputTexture, vUv+off) * 0.034;
    c += texture(inputTexture, vUv-off) * 0.034;
    fragColor = c;
}
)GLSL");
        blurVProg = createPPProgramFromFrag(R"GLSL(
#version 330
in vec2 vUv;
uniform sampler2D inputTexture;
uniform vec2 texelSize;
out vec4 fragColor;
void main() {
    vec4 c = texture(inputTexture, vUv) * 0.227;
    vec2 off = vec2(0.0, texelSize.y);
    c += texture(inputTexture, vUv+off) * 0.090;
    c += texture(inputTexture, vUv-off) * 0.090;
    off.y = texelSize.y * 2.0;
    c += texture(inputTexture, vUv+off) * 0.054;
    c += texture(inputTexture, vUv-off) * 0.054;
    off.y = texelSize.y * 3.0;
    c += texture(inputTexture, vUv+off) * 0.034;
    c += texture(inputTexture, vUv-off) * 0.034;
    fragColor = c;
}
)GLSL");
        compositeProg = createPPProgramFromFrag(R"GLSL(
#version 330
in vec2 vUv;
uniform sampler2D originalTexture;
uniform sampler2D bloomTexture;
uniform float strength;
out vec4 fragColor;
void main() {
    vec4 orig = texture(originalTexture, vUv);
    vec4 bloom = texture(bloomTexture, vUv);
    fragColor = vec4(orig.rgb + bloom.rgb * strength, orig.a);
}
)GLSL");
    }
};

// -------------------------------------------------------------------------
// OutlinePass – stub (needs depth/stencil for edge detection)
// -------------------------------------------------------------------------
class OutlinePass : public Pass {
public:
    glm::vec3 visibleEdgeColor{1.0f, 0.72f, 0.22f};
    float edgeStrength = 2.5f;
    OutlinePass() { clearBefore = true; }
    void render(GLRenderer& renderer, Scene& scene, Camera& camera) override {
        renderer.render(scene, camera);
    }
};

// -------------------------------------------------------------------------
// FXAAPass
// -------------------------------------------------------------------------
class FXAAPass : public Pass {
public:
    FXAAPass() { clearBefore = true; }

    void render(GLRenderer& renderer, Scene& scene, Camera& camera) override {
        if (!composer) { renderer.render(scene, camera); return; }
        GLuint inputTex = composer->readTexture();
        int w = composer->width();
        int h = composer->height();
        if (!inputTex) { renderer.render(scene, camera); return; }
        ensureProg();
        if (!fxaaProg) { renderer.render(scene, camera); return; }
        glUseProgram(fxaaProg);
        bindTex(fxaaProg, "inputTexture", 0, inputTex);
        glUniform2f(glGetUniformLocation(fxaaProg, "texelSize"), 1.0f / w, 1.0f / h);
        renderer.renderFullScreenQuad();
        glUseProgram(0);
    }

private:
    GLuint fxaaProg = 0;
    void ensureProg() {
        if (fxaaProg) return;
        fxaaProg = createPPProgramFromFrag(R"GLSL(
#version 330
in vec2 vUv;
uniform sampler2D inputTexture;
uniform vec2 texelSize;
out vec4 fragColor;
void main() {
    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lC  = dot(texture(inputTexture, vUv).rgb, luma);
    float lNW = dot(texture(inputTexture, vUv+vec2(-1,-1)*texelSize).rgb, luma);
    float lNE = dot(texture(inputTexture, vUv+vec2( 1,-1)*texelSize).rgb, luma);
    float lSW = dot(texture(inputTexture, vUv+vec2(-1, 1)*texelSize).rgb, luma);
    float lSE = dot(texture(inputTexture, vUv+vec2( 1, 1)*texelSize).rgb, luma);
    float c=max(max(lNW,lNE),max(lSW,lSE))-min(min(lNW,lNE),min(lSW,lSE));
    if (c < 0.05) { fragColor = texture(inputTexture, vUv); return; }
    float lMin = min(lC, min(min(lNW,lNE), min(lSW,lSE)));
    float lMax = max(lC, max(max(lNW,lNE), max(lSW,lSE)));
    vec2 dir = vec2(-(lNW+lNE-lSW-lSE), (lNW+lSW-lNE-lSE));
    float dirReduce = max((lNW+lNE+lSW+lSE)*0.25*0.125, 1.0/128.0);
    float rcpDir = 1.0/(min(abs(dir.x),abs(dir.y))+dirReduce);
    dir = min(vec2(8.0), max(vec2(-8.0), dir*rcpDir))*texelSize;
    vec3 rgbA=texture(inputTexture, vUv+dir*(1.0/3.0-0.5)).rgb;
    vec3 rgbB=texture(inputTexture, vUv+dir*(2.0/3.0-0.5)).rgb;
    vec3 rgbC=texture(inputTexture, vUv+dir*0.5).rgb;
    fragColor = vec4((rgbA+rgbB)*0.5+rgbC*0.25, 1.0);
}
)GLSL");
    }
};

} // namespace THREE
