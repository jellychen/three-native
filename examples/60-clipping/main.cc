#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "light/light.h"
#include "helpers/helpers.h"
#include "helpers/geometry-factory.h"

using namespace THREE;

int main() {
    Window window(1280, 720, "threecpp 60 clipping");
    Scene scene;
    PerspectiveCamera camera(55.0f, 1280.0f / 720.0f, 0.1f, 100.0f);
    camera.position = {3.0f, 2.0f, 5.0f};
    camera.lookAt({0.0f, 0.5f, 0.0f});

    auto mat = make_ref<MeshStandardMaterial>();
    mat->color = {0.3f, 0.6f, 1.0f};
    mat->roughness = 0.4f;
    mat->metalness = 0.2f;
    mat->clippingPlanes = {glm::vec4(0.0f, -1.0f, 0.0f, 0.3f)};
    auto mesh = make_ref<Mesh>(GeometryFactory::makeSphere(1.0f, 64, 48), mat);
    mesh->position = {0.0f, 0.5f, 0.0f};
    scene.add(mesh);

    auto mat2 = make_ref<MeshStandardMaterial>();
    mat2->color = {1.0f, 0.3f, 0.3f};
    mat2->roughness = 0.6f;
    mat2->metalness = 0.1f;
    mat2->transparent = true;
    mat2->opacity = 0.7f;
    mat2->clippingPlanes = {glm::vec4(0.0f, -1.0f, 0.0f, 0.5f)};
    auto mesh2 = make_ref<Mesh>(GeometryFactory::makeSphere(0.7f, 48, 36), mat2);
    mesh2->position = {1.2f, 0.5f, 0.8f};
    scene.add(mesh2);

    auto grid = make_ref<GridHelper>(6.0f, 6);
    scene.add(grid);

    auto dirLight = make_ref<DirectionalLight>();
    dirLight->color = {1.0f, 0.95f, 0.9f};
    dirLight->intensity = 1.5f;
    dirLight->position = {3.0f, 5.0f, 3.0f};
    dirLight->target = {0.0f, 0.0f, 0.0f};
    scene.add(dirLight);

    auto hemi = make_ref<HemisphereLight>();
    hemi->skyColor = {0.4f, 0.6f, 1.0f};
    hemi->groundColor = {0.2f, 0.1f, 0.05f};
    hemi->intensity = 0.4f;
    scene.add(hemi);

    GLRenderer renderer({1280, 720, true, {0.04f, 0.04f, 0.045f, 1.0f}});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);
    while (!window.shouldClose()) {
        window.pollEvents();
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
