#include "platform/window.h"
#include "renderer/gl-renderer.h"

using namespace THREE;

int main() {
    Window window(1280, 720, "threecpp 00 clear screen");
    GLRenderer renderer({1280, 720, true, {0.05f, 0.05f, 0.08f, 1.0f}});
    while (!window.shouldClose()) {
        window.pollEvents();
        renderer.clear();
        window.swapBuffers();
    }
    return 0;
}
