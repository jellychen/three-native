#pragma once
#include "common.hpp"
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

namespace threecpp {

class Window {
    GLFWwindow* handle_ = nullptr;
    int width_ = 1280;
    int height_ = 720;
    double scrollX_ = 0.0;
    double scrollY_ = 0.0;
public:
    Window(int width, int height, const char* title);
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void pollEvents();
    void poll() { pollEvents(); } // three.js-style/examples compatibility alias
    void swapBuffers();
    GLFWwindow* native() const { return handle_; }
    int width() const { return width_; }
    int height() const { return height_; }
    glm::ivec2 framebufferSize() const;
    float aspect() const;
    void requestClose();
    double time() const;

    // Lightweight input helpers used by examples and OrbitControls.
    // The renderer still owns no platform input state; Window only exposes GLFW events in a stable wrapper.
    glm::dvec2 cursorPosition() const;
    bool mouseButtonPressed(int glfwButton) const;
    bool keyPressed(int glfwKey) const;
    glm::dvec2 consumeScrollDelta();
    void addScrollDelta(double x, double y);
};

} // namespace threecpp
