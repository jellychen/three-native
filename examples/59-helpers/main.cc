#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "light/light.h"
#include "helpers/helpers.h"
#include "helpers/geometry-factory.h"

using namespace THREE;

int main() {
    Window window(1280, 720, "threecpp 59 helpers");
    Scene scene;
    PerspectiveCamera camera(55.0f, 1280.0f / 720.0f, 0.1f, 100.0f);
    camera.position = {5.0f, 4.0f, 7.0f};
    camera.lookAt({0.0f, 0.5f, 0.0f});

    auto mat = make_ref<MeshStandardMaterial>();
    mat->color = {0.7f, 0.3f, 0.75f};
    mat->roughness = 0.4f;
    mat->metalness = 0.1f;
    auto mesh = make_ref<Mesh>(GeometryFactory::makeTorus(0.6f, 0.22f, 48, 32), mat);
    mesh->position = {0.0f, 1.0f, 0.0f};
    scene.add(mesh);

    auto grid = make_ref<GridHelper>(8.0f, 8);
    grid->position = {0.0f, 0.0f, 0.0f};
    scene.add(grid);

    auto axes = make_ref<AxesHelper>(2.0f);
    axes->position = {-3.0f, 0.0f, -3.0f};
    scene.add(axes);

    auto box = make_ref<BoxHelper>(glm::vec3(-0.9f), glm::vec3(0.9f));
    box->position = {0.0f, 1.0f, 0.0f};
    scene.add(box);

    auto dirLight = make_ref<DirectionalLight>();
    dirLight->color = {1.0f, 0.95f, 0.9f};
    dirLight->intensity = 1.5f;
    dirLight->position = {3.0f, 5.0f, 3.0f};
    dirLight->target = {0.0f, 0.0f, 0.0f};
    scene.add(dirLight);

    auto dirHelper = make_ref<DirectionalLightHelper>(*dirLight, 1.0f);
    dirHelper->position = dirLight->position;
    scene.add(dirHelper);

    auto pointLight = make_ref<PointLight>();
    pointLight->color = {1.0f, 0.3f, 0.2f};
    pointLight->intensity = 0.8f;
    pointLight->distance = 4.0f;
    pointLight->position = {-2.0f, 2.5f, 2.0f};
    scene.add(pointLight);

    auto pointHelper = make_ref<PointLightHelper>(*pointLight, 0.5f);
    pointHelper->position = pointLight->position;
    scene.add(pointHelper);

    auto camHelper = make_ref<CameraHelper>(camera);
    camHelper->position = camera.position;
    camHelper->quaternion = camera.quaternion;
    scene.add(camHelper);

    auto normHelper = make_ref<VertexNormalsHelper>(*mesh, 0.2f);
    scene.add(normHelper);

    auto hemi = make_ref<HemisphereLight>();
    hemi->skyColor = {0.4f, 0.6f, 1.0f};
    hemi->groundColor = {0.2f, 0.1f, 0.05f};
    hemi->intensity = 0.5f;
    scene.add(hemi);

    GLRenderer renderer({1280, 720, true, {0.04f, 0.04f, 0.045f, 1.0f}});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);
    while (!window.shouldClose()) {
        window.pollEvents();
        mesh->quaternion = glm::angleAxis(static_cast<float>(glfwGetTime()) * 0.4f, glm::normalize(glm::vec3(0.2f, 1.0f, 0.1f)));
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
