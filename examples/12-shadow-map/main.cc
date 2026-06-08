#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "light/light.h"
#include "helpers/geometry-factory.h"

using namespace THREE;

int main() {
    Window window(1280, 720, "threecpp v1.3 shadow map / PCF");

    Scene scene;
    scene.backgroundColor = {0.025f, 0.028f, 0.035f};
    scene.environment = make_ref<Environment>();
    scene.environment->skyColor = {0.18f, 0.22f, 0.30f};
    scene.environment->groundColor = {0.04f, 0.04f, 0.045f};
    scene.environment->specularColor = {0.8f, 0.85f, 0.95f};
    scene.environmentIntensity = 0.55f;

    PerspectiveCamera camera(55.0f, 1280.0f / 720.0f, 0.1f, 200.0f);
    camera.position = {5.5f, 4.0f, 7.0f};
    camera.lookAt({0.0f, 0.8f, 0.0f});

    auto ambient = make_ref<AmbientLight>();
    ambient->color = {0.045f, 0.05f, 0.065f};
    ambient->intensity = 1.0f;
    scene.add(ambient);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {5.0f, 8.0f, 4.0f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->color = {1.0f, 0.95f, 0.86f};
    sun->intensity = 5.0f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.mapSizeX = 2048;
    sun->shadow.mapSizeY = 2048;
    sun->shadow.bias = 0.0012f;
    sun->shadow.radius = 1.25f;
    scene.add(sun);

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.50f, 0.50f, 0.48f};
    groundMat->roughness = 0.78f;
    groundMat->metalness = 0.0f;
    auto ground = make_ref<Mesh>(GeometryFactory::makePlane(16.0f), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    auto sphereGeo = GeometryFactory::makeUVSphere(0.75f, 64, 32);
    auto boxGeo = GeometryFactory::makeCube(1.25f);
    for (int i = 0; i < 5; ++i) {
        auto mat = make_ref<MeshStandardMaterial>();
        mat->color = glm::mix(glm::vec3(0.95f, 0.34f, 0.14f), glm::vec3(0.18f, 0.55f, 0.95f), float(i) / 4.0f);
        mat->roughness = glm::mix(0.25f, 0.8f, float(i) / 4.0f);
        mat->metalness = (i == 4) ? 0.75f : 0.0f;
        auto mesh = make_ref<Mesh>((i % 2 == 0) ? sphereGeo : boxGeo, mat);
        mesh->position = {float(i - 2) * 1.6f, (i % 2 == 0) ? 0.78f : 0.65f, 0.0f};
        mesh->castShadow = true;
        mesh->receiveShadow = true;
        scene.add(mesh);
    }

    auto gridMat = make_ref<LineBasicMaterial>();
    gridMat->color = {0.14f, 0.15f, 0.17f};
    auto grid = make_ref<LineSegments>(GeometryFactory::makeGrid(20, 16.0f), gridMat);
    grid->position.y = 0.004f;
    scene.add(grid);

    GLRenderer renderer({1280, 720});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);

    while (!window.shouldClose()) {
        window.pollEvents();
        float t = static_cast<float>(window.time());
        camera.position = {std::sin(t * 0.35f) * 6.0f, 4.2f, std::cos(t * 0.35f) * 7.0f};
        camera.lookAt({0.0f, 0.7f, 0.0f});
        renderer.render(scene, camera);
        window.swapBuffers();
    }
}
