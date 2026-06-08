#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "light/light.h"
#include "geometry/fat-line-geometry.h"

using namespace THREE;

int main() {
    Window window(1280, 720, "threecpp 01 basic primitives");
    Scene scene;
    PerspectiveCamera camera(55.0f, 1280.0f / 720.0f, 0.1f, 100.0f);
    camera.position = {4.0f, 3.0f, 6.0f};
    camera.quaternion = glm::quatLookAt(glm::normalize(glm::vec3(0, 0, 0) - camera.position), glm::vec3(0, 1, 0));

    auto cubeMat = make_ref<MeshBasicMaterial>();
    cubeMat->color = {0.9f, 0.25f, 0.12f};
    auto cube = make_ref<Mesh>(BufferGeometry::makeBox(1.2f), cubeMat);
    cube->position = {0.0f, 0.8f, 0.0f};
    scene.add(cube);

    auto gridMat = make_ref<LineBasicMaterial>();
    gridMat->color = {0.25f, 0.25f, 0.28f};
    auto grid = make_ref<LineSegments>(BufferGeometry::makeLineGrid(10, 0.5f), gridMat);
    scene.add(grid);

    auto ptsMat = make_ref<PointsMaterial>();
    ptsMat->color = {0.2f, 0.7f, 1.0f};
    ptsMat->size = 6.0f;
    ptsMat->vertexColors = true;
    auto points = make_ref<Points>(BufferGeometry::makeRandomPoints(1500, 1.5f), ptsMat);
    points->position = {-2.5f, 1.2f, 0.0f};
    scene.add(points);

    auto fatMat = make_ref<FatLineMaterial>();
    fatMat->color = {1.0f, 0.85f, 0.15f};
    fatMat->linewidth = 10.0f;
    std::vector<glm::vec3> curve = {{-1.2f,1.5f,-1.0f},{-0.6f,2.1f,-0.4f},{0.1f,1.6f,0.2f},{0.8f,2.0f,0.8f},{1.4f,1.3f,1.1f}};
    auto fat = make_ref<FatLine>(FatLineGeometry::fromPolyline(curve, 0.08f), fatMat);
    scene.add(fat);

    GLRenderer renderer({1280, 720, true, {0.015f, 0.015f, 0.02f, 1.0f}});
    while (!window.shouldClose()) {
        window.pollEvents();
        cube->quaternion = glm::angleAxis(static_cast<float>(glfwGetTime()) * 0.7f, glm::normalize(glm::vec3(0.2f, 1.0f, 0.1f)));
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
