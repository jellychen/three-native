#pragma once
#include "common.hpp"
#include "material/Material.hpp"
#include "platform/gl_headers.hpp"

namespace threecpp {

class GLState {
    GLuint currentProgram = 0;
    bool depthTest = false;
    bool blend = false;
    bool cull = false;
    bool depthMask = true;
    bool scissorTest = false;
    bool colorMask = true;
    bool polygonOffsetFill = false;
    bool wireframeMode = false;
public:
    void reset();
    void useProgram(GLuint program);
    void setDepthTest(bool enabled);
    void setDepthWrite(bool enabled);
    void setBlending(Blending blending, bool transparent, bool premultipliedAlpha = false);
    void setCullFace(Side side);
    void setViewport(int x, int y, int w, int h);
    void setScissor(int x, int y, int w, int h);
    void setScissorTest(bool enabled);
    void setColorWrite(bool enabled);
    void setPolygonOffset(bool enabled, float factor, float units);
    void setWireframe(bool enabled);
    void applyMaterial(const Material& material);
};

} // namespace threecpp
