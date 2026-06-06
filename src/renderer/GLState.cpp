#include "renderer/GLState.hpp"

namespace threecpp {

void GLState::reset() {
    currentProgram = 0; depthTest = false; blend = false; cull = false; depthMask = true; scissorTest = false; colorMask = true; polygonOffsetFill = false; wireframeMode = false;
}
void GLState::useProgram(GLuint program) { if (currentProgram != program) { glUseProgram(program); currentProgram = program; } }
void GLState::setDepthTest(bool enabled) { if (depthTest != enabled) { enabled ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST); depthTest = enabled; } }
void GLState::setDepthWrite(bool enabled) { if (depthMask != enabled) { glDepthMask(enabled ? GL_TRUE : GL_FALSE); depthMask = enabled; } }
void GLState::setBlending(Blending b, bool transparent, bool premultipliedAlpha) {
    bool enabled = transparent && b != Blending::None;
    if (blend != enabled) { enabled ? glEnable(GL_BLEND) : glDisable(GL_BLEND); blend = enabled; }
    if (!enabled) return;
    switch (b) {
        case Blending::Additive: glBlendFunc(GL_SRC_ALPHA, GL_ONE); break;
        case Blending::Multiply: glBlendFunc(GL_DST_COLOR, GL_ZERO); break;
        case Blending::Subtractive: glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR); break;
        case Blending::Normal: default: premultipliedAlpha ? glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA) : glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
    }
}
void GLState::setCullFace(Side side) {
    bool enabled = side != Side::DoubleSide;
    if (cull != enabled) { enabled ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE); cull = enabled; }
    if (enabled) glCullFace(side == Side::BackSide ? GL_FRONT : GL_BACK);
}
void GLState::setViewport(int x, int y, int w, int h) { glViewport(x, y, w, h); }
void GLState::setScissor(int x, int y, int w, int h) { glScissor(x, y, w, h); }
void GLState::setScissorTest(bool enabled) { if (scissorTest != enabled) { enabled ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST); scissorTest = enabled; } }
void GLState::setColorWrite(bool enabled) {
    if (colorMask != enabled) { glColorMask(enabled ? GL_TRUE : GL_FALSE, enabled ? GL_TRUE : GL_FALSE, enabled ? GL_TRUE : GL_FALSE, enabled ? GL_TRUE : GL_FALSE); colorMask = enabled; }
}
void GLState::setPolygonOffset(bool enabled, float factor, float units) {
    if (polygonOffsetFill != enabled) { enabled ? glEnable(GL_POLYGON_OFFSET_FILL) : glDisable(GL_POLYGON_OFFSET_FILL); polygonOffsetFill = enabled; }
    if (enabled) glPolygonOffset(factor, units);
}
void GLState::setWireframe(bool enabled) {
#if !THREECPP_USE_ANGLE
    if (wireframeMode != enabled) { glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL); wireframeMode = enabled; }
#else
    (void)enabled;
#endif
}
void GLState::applyMaterial(const Material& m) {
    setDepthTest(m.depthTest);
    setDepthWrite(m.depthWrite);
    setColorWrite(m.colorWrite);
    setBlending(m.blending, m.transparent, m.premultipliedAlpha);
    setCullFace(m.side);
    setPolygonOffset(m.polygonOffset, m.polygonOffsetFactor, m.polygonOffsetUnits);
    setWireframe(m.wireframe);
}

} // namespace threecpp
