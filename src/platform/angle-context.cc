#include "platform/angle-context.h"
#include <sstream>
#include <stdexcept>

#if THREECPP_USE_ANGLE
    #if defined(_WIN32)
        #define GLFW_EXPOSE_NATIVE_WIN32
    #elif defined(__linux__)
        #define GLFW_EXPOSE_NATIVE_X11
    #elif defined(__APPLE__)
        #define GLFW_EXPOSE_NATIVE_COCOA
    #endif
    #include <GLFW/glfw3native.h>
#endif

namespace THREE {

#if THREECPP_USE_ANGLE
static std::string eglErrorString(EGLint err) {
    switch (err) {
        case EGL_SUCCESS: return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED: return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS: return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC: return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE: return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONTEXT: return "EGL_BAD_CONTEXT";
        case EGL_BAD_CONFIG: return "EGL_BAD_CONFIG";
        case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY: return "EGL_BAD_DISPLAY";
        case EGL_BAD_SURFACE: return "EGL_BAD_SURFACE";
        case EGL_BAD_MATCH: return "EGL_BAD_MATCH";
        case EGL_BAD_PARAMETER: return "EGL_BAD_PARAMETER";
        case EGL_BAD_NATIVE_PIXMAP: return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW: return "EGL_BAD_NATIVE_WINDOW";
        case EGL_CONTEXT_LOST: return "EGL_CONTEXT_LOST";
        default: {
            std::ostringstream os;
            os << "0x" << std::hex << err;
            return os.str();
        }
    }
}

[[noreturn]] static void throwEgl(const char* what) {
    std::ostringstream os;
    os << what << " failed: " << eglErrorString(eglGetError());
    throw std::runtime_error(os.str());
}
#endif

bool AngleContext::initialize(Window& window) {
#if THREECPP_USE_ANGLE
    auto getPlatformDisplay = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));

#if defined(_WIN32)
    // Prefer ANGLE D3D11 on Windows, matching ANGLE's common desktop path.
    if (getPlatformDisplay) {
        const EGLint attrs[] = {
#ifdef EGL_PLATFORM_ANGLE_TYPE_ANGLE
            EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
#endif
#ifdef EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE
            EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE, 11,
#endif
#ifdef EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE
            EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE, 0,
#endif
            EGL_NONE
        };
#ifdef EGL_PLATFORM_ANGLE_ANGLE
        display_ = getPlatformDisplay(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, attrs);
#endif
    }
#elif defined(__linux__)
    if (getPlatformDisplay) {
#ifdef EGL_PLATFORM_X11_EXT
        Display* x11Display = glfwGetX11Display();
        display_ = getPlatformDisplay(EGL_PLATFORM_X11_EXT, x11Display, nullptr);
#endif
    }
#elif defined(__APPLE__)
    if (getPlatformDisplay) {
        // ANGLE-on-mac generally uses the default display with Metal/OpenGL backend selection at build time.
#ifdef EGL_PLATFORM_ANGLE_ANGLE
        display_ = getPlatformDisplay(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, nullptr);
#endif
    }
#endif

    if (display_ == EGL_NO_DISPLAY) display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display_ == EGL_NO_DISPLAY) throwEgl("eglGetDisplay");

    EGLint major = 0, minor = 0;
    if (!eglInitialize(display_, &major, &minor)) throwEgl("eglInitialize");
    if (!eglBindAPI(EGL_OPENGL_ES_API)) throwEgl("eglBindAPI(EGL_OPENGL_ES_API)");

    const EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_SAMPLE_BUFFERS, 1,
        EGL_SAMPLES, 4,
        EGL_NONE
    };

    EGLint num = 0;
    if (!eglChooseConfig(display_, configAttribs, &config_, 1, &num) || num == 0) {
        const EGLint fallbackAttribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_STENCIL_SIZE, 8,
            EGL_NONE
        };
        if (!eglChooseConfig(display_, fallbackAttribs, &config_, 1, &num) || num == 0) throwEgl("eglChooseConfig");
    }

    const EGLint ctxAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, ctxAttribs);
    if (context_ == EGL_NO_CONTEXT) throwEgl("eglCreateContext");

#if defined(_WIN32)
    EGLNativeWindowType nativeWindow = reinterpret_cast<EGLNativeWindowType>(glfwGetWin32Window(window.native()));
#elif defined(__linux__)
    EGLNativeWindowType nativeWindow = static_cast<EGLNativeWindowType>(glfwGetX11Window(window.native()));
#elif defined(__APPLE__)
    EGLNativeWindowType nativeWindow = reinterpret_cast<EGLNativeWindowType>(glfwGetCocoaWindow(window.native()));
#else
    EGLNativeWindowType nativeWindow = reinterpret_cast<EGLNativeWindowType>(window.native());
#endif

    surface_ = eglCreateWindowSurface(display_, config_, nativeWindow, nullptr);
    if (surface_ == EGL_NO_SURFACE) throwEgl("eglCreateWindowSurface");
    makeCurrent();
    setSwapInterval(1);
#else
    (void)window;
#endif
    return true;
}

void AngleContext::makeCurrent() {
#if THREECPP_USE_ANGLE
    if (!eglMakeCurrent(display_, surface_, surface_, context_)) throwEgl("eglMakeCurrent");
#endif
}

void AngleContext::swapBuffers(Window& window) {
#if THREECPP_USE_ANGLE
    (void)window;
    if (!eglSwapBuffers(display_, surface_)) throwEgl("eglSwapBuffers");
#else
    window.swapBuffers();
#endif
}

void AngleContext::setSwapInterval(int interval) {
#if THREECPP_USE_ANGLE
    eglSwapInterval(display_, interval);
#else
    (void)interval;
#endif
}

void AngleContext::shutdown() {
#if THREECPP_USE_ANGLE
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_ != EGL_NO_CONTEXT) eglDestroyContext(display_, context_);
        if (surface_ != EGL_NO_SURFACE) eglDestroySurface(display_, surface_);
        eglTerminate(display_);
    }
    display_ = EGL_NO_DISPLAY;
    context_ = EGL_NO_CONTEXT;
    surface_ = EGL_NO_SURFACE;
    config_ = nullptr;
#endif
}

} // namespace THREE
