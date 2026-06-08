#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "light/light.h"
#include "helpers/geometry-factory.h"

using namespace THREE;

int main() {
    Window window(1280, 720, "threecpp 61 rectarealight");
    Scene scene;
    PerspectiveCamera camera(55.0f, 1280.0f / 720.0f, 0.1f, 100.0f);
    camera.position = {3.0f, 2.5f, 5.0f};
    camera.lookAt({0.0f, 0.5f, 0.0f});

    for (int i = 0; i < 6; ++i) {
        auto mat = make_ref<MeshStandardMaterial>();
        float t = static_cast<float>(i) / 5.0f;
        mat->color = glm::mix(glm::vec3(1.0f, 0.2f, 0.1f), glm::vec3(0.1f, 0.3f, 1.0f), t);
        mat->roughness = t;
        mat->metalness = 0.3f;
        auto mesh = make_ref<Mesh>(GeometryFactory::makeSphere(0.35f, 48, 32), mat);
        mesh->position = {static_cast<float>(i) * 0.55f - 1.4f, 0.5f, 0.0f};
        scene.add(mesh);
    }

    auto rectLight = make_ref<RectAreaLight>();
    rectLight->color = {1.0f, 0.9f, 0.7f};
    rectLight->intensity = 2.5f;
    rectLight->width = 3.0f;
    rectLight->height = 2.0f;
    rectLight->position = {0.0f, 2.5f, 2.0f};
    rectLight->lookAt({0.0f, 0.5f, 0.0f});
    scene.add(rectLight);

    auto hemi = make_ref<HemisphereLight>();
    hemi->skyColor = {0.2f, 0.3f, 0.6f};
    hemi->groundColor = {0.1f, 0.05f, 0.05f};
    hemi->intensity = 0.3f;
    scene.add(hemi);

    auto gridMat = make_ref<LineBasicMaterial>();
    gridMat->color = {0.3f, 0.3f, 0.35f};
    auto grid = make_ref<LineSegments>(BufferGeometry::makeLineGrid(12, 0.5f), gridMat);
    grid->position = {0.0f, -0.1f, 0.0f};
    scene.add(grid);

    GLRenderer renderer({1280, 720, true, {0.02f, 0.02f, 0.025f, 1.0f}});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);
    while (!window.shouldClose()) {
        window.pollEvents();
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
