#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "loader/AssimpLoader.hpp"
#include <iostream>

using namespace threecpp;

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "assets/models/model.glb";
    Window window(1280, 720, "threecpp 02 assimp viewer");
    Scene scene;
    PerspectiveCamera camera(55.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);
    camera.position = {0.0f, 1.5f, 6.0f};
    camera.quaternion = glm::quatLookAt(glm::normalize(glm::vec3(0, 0.8f, 0) - camera.position), glm::vec3(0, 1, 0));

    try {
        AssimpLoader loader;
        auto model = loader.load(path);
        scene.add(model);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\nFalling back to cube. Pass a model path as argv[1].\n";
        auto mat = make_ref<MeshBasicMaterial>();
        mat->color = {0.8f, 0.8f, 0.85f};
        scene.add(make_ref<Mesh>(BufferGeometry::makeBox(1.5f), mat));
    }

    auto gridMat = make_ref<LineBasicMaterial>();
    gridMat->color = {0.22f, 0.22f, 0.24f};
    scene.add(make_ref<LineSegments>(BufferGeometry::makeLineGrid(20, 0.5f), gridMat));

    GLRenderer renderer({1280, 720, true, {0.02f, 0.02f, 0.025f, 1.0f}});
    while (!window.shouldClose()) {
        window.pollEvents();
        scene.quaternion = glm::angleAxis(static_cast<float>(glfwGetTime()) * 0.25f, glm::vec3(0, 1, 0));
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
