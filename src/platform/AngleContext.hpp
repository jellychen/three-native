#pragma once
#include "common.hpp"
#include "platform/Window.hpp"
#include "platform/gl_headers.hpp"

namespace threecpp {

class AngleContext {
#if THREECPP_USE_ANGLE
    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLContext context_ = EGL_NO_CONTEXT;
    EGLSurface surface_ = EGL_NO_SURFACE;
    EGLConfig config_ = nullptr;
#endif
public:
    bool initialize(Window& window);
    void makeCurrent();
    void swapBuffers(Window& window);
    void setSwapInterval(int interval);
    void shutdown();
};

} // namespace threecpp
