#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "geometry/fat-line-geometry.h"
#include "helpers/geometry-factory.h"

using namespace THREE;

int main() {
    Window window(1280, 720, "threecpp 04 fatline");
    Scene scene;
    PerspectiveCamera camera(50.0f, 1280.0f / 720.0f, 0.1f, 100.0f);
    camera.position = {0, 3, 8};
    camera.quaternion = glm::quatLookAt(glm::normalize(glm::vec3(0, 0.5f, 0) - camera.position), glm::vec3(0, 1, 0));

    auto gridMat = make_ref<LineBasicMaterial>();
    gridMat->color = {0.18f,0.18f,0.2f};
    scene.add(make_ref<LineSegments>(GeometryFactory::makeGrid(20, 10.0f), gridMat));

    std::vector<glm::vec3> pts;
    for (int i = 0; i < 80; ++i) {
        float t = float(i) / 79.0f * glm::two_pi<float>() * 2.0f;
        pts.push_back({std::cos(t) * 2.5f, 1.2f + std::sin(t * 2.0f) * 0.6f, std::sin(t) * 2.5f});
    }
    auto mat = make_ref<FatLineMaterial>();
    mat->color = {1.0f, 0.82f, 0.12f};
    mat->linewidth = 12.0f;
    scene.add(make_ref<FatLine>(FatLineGeometry::fromPolyline(pts, 0.08f), mat));

    GLRenderer renderer({1280, 720});
    while (!window.shouldClose()) {
        window.pollEvents();
        renderer.render(scene, camera);
        window.swapBuffers();
    }
}
