#include "platform/window.h"
#include <stdexcept>

namespace THREE {

Window::Window(int width, int height, const char* title) : width_(width), height_(height) {
    if (!glfwInit()) throw std::runtime_error("glfwInit failed");
#if THREECPP_USE_ANGLE
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
    handle_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!handle_) throw std::runtime_error("glfwCreateWindow failed");
    glfwSetWindowUserPointer(handle_, this);
    glfwSetFramebufferSizeCallback(handle_, [](GLFWwindow* w, int fbw, int fbh) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
        if (self) { self->width_ = fbw; self->height_ = fbh; }
    });
    glfwSetScrollCallback(handle_, [](GLFWwindow* w, double sx, double sy) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
        if (self) self->addScrollDelta(sx, sy);
    });
#if !THREECPP_USE_ANGLE
    glfwMakeContextCurrent(handle_);
    glfwSwapInterval(1);
#endif
}

Window::~Window() {
    if (handle_) glfwDestroyWindow(handle_);
    glfwTerminate();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(handle_); }
void Window::pollEvents() { glfwPollEvents(); }
void Window::swapBuffers() { glfwSwapBuffers(handle_); }

glm::ivec2 Window::framebufferSize() const {
    int w = 0, h = 0;
    glfwGetFramebufferSize(handle_, &w, &h);
    return {std::max(w, 1), std::max(h, 1)};
}

float Window::aspect() const {
    auto s = framebufferSize();
    return float(s.x) / float(std::max(s.y, 1));
}

void Window::requestClose() { glfwSetWindowShouldClose(handle_, GLFW_TRUE); }

double Window::time() const { return glfwGetTime(); }

glm::dvec2 Window::cursorPosition() const {
    double x = 0.0, y = 0.0;
    glfwGetCursorPos(handle_, &x, &y);
    return {x, y};
}

bool Window::mouseButtonPressed(int glfwButton) const {
    return glfwGetMouseButton(handle_, glfwButton) == GLFW_PRESS;
}

bool Window::keyPressed(int glfwKey) const {
    return glfwGetKey(handle_, glfwKey) == GLFW_PRESS;
}

glm::dvec2 Window::consumeScrollDelta() {
    glm::dvec2 d{scrollX_, scrollY_};
    scrollX_ = 0.0;
    scrollY_ = 0.0;
    return d;
}

void Window::addScrollDelta(double x, double y) {
    scrollX_ += x;
    scrollY_ += y;
}

} // namespace THREE
