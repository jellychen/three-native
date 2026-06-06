#pragma once
#if THREECPP_USE_ANGLE
    #include <EGL/egl.h>
    #include <EGL/eglext.h>
    #include <GLES3/gl3.h>
#else
    #if defined(__APPLE__)
        #include <OpenGL/gl3.h>
    #else
        #include <GL/gl.h>
    #endif
#endif
