#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "core/Renderable.hpp"
#include "light/Light.hpp"
#include "helpers/GeometryFactory.hpp"
#include "helpers/Helpers.hpp"
#include "ibl/Environment.hpp"

using namespace threecpp;


int main() {
    Window window(1440, 860, "threecpp v3.4 multi light + shadow test");

    Scene scene;
    scene.backgroundColor = {0.018f, 0.020f, 0.028f};
    scene.environment = make_ref<Environment>();
    scene.environment->skyColor = {0.15f, 0.19f, 0.28f};
    scene.environment->groundColor = {0.035f, 0.035f, 0.040f};
    scene.environment->specularColor = {0.70f, 0.76f, 0.90f};
    scene.environmentIntensity = 0.55f;

    PerspectiveCamera camera(50.0f, 1440.0f / 860.0f, 0.05f, 220.0f);
    camera.position = {7.0f, 5.2f, 8.5f};
    camera.lookAt({0.0f, 1.0f, 0.0f});

    auto ambient = make_ref<AmbientLight>();
    ambient->color = {0.025f, 0.030f, 0.040f};
    ambient->intensity = 1.0f;
    scene.add(ambient);

    auto hemi = make_ref<HemisphereLight>();
    hemi->color = {0.25f, 0.33f, 0.50f};
    hemi->groundColor = {0.06f, 0.045f, 0.035f};
    hemi->intensity = 0.65f;
    scene.add(hemi);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {7.0f, 10.0f, 6.0f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->color = {1.0f, 0.94f, 0.82f};
    sun->intensity = 4.5f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.mapSizeX = 2048;
    sun->shadow.mapSizeY = 2048;
    sun->shadow.cameraLeft = -13.0f;
    sun->shadow.cameraRight = 13.0f;
    sun->shadow.cameraBottom = -13.0f;
    sun->shadow.cameraTop = 13.0f;
    sun->shadow.cameraFar = 60.0f;
    sun->shadow.bias = 0.0011f;
    sun->shadow.radius = 1.5f;
    scene.add(sun);

    auto spot = make_ref<SpotLight>();
    spot->position = {-4.5f, 6.0f, 4.0f};
    spot->target = {0.0f, 0.7f, 0.0f};
    spot->color = {0.60f, 0.78f, 1.0f};
    spot->intensity = 70.0f;
    spot->distance = 18.0f;
    spot->decay = 2.0f;
    spot->angle = glm::radians(24.0f);
    spot->penumbra = 0.45f;
    spot->castShadow = true;
    spot->shadow.enabled = true;
    spot->shadow.mapSizeX = 1024;
    spot->shadow.mapSizeY = 1024;
    spot->shadow.cameraNear = 0.2f;
    spot->shadow.cameraFar = 25.0f;
    spot->shadow.bias = 0.0009f;
    spot->shadow.radius = 1.0f;
    scene.add(spot);

    auto warmPoint = make_ref<PointLight>();
    warmPoint->position = {3.5f, 2.2f, -3.5f};
    warmPoint->color = {1.0f, 0.48f, 0.22f};
    warmPoint->intensity = 65.0f;
    warmPoint->distance = 14.0f;
    warmPoint->decay = 2.0f;
    scene.add(warmPoint);

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.44f, 0.44f, 0.42f};
    groundMat->roughness = 0.82f;
    auto ground = make_ref<Mesh>(GeometryFactory::makePlane(24.0f), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    auto sphereGeo = GeometryFactory::makeUVSphere(0.62f, 48, 24);
    auto boxGeo = GeometryFactory::makeCube(1.15f);
    for (int z = 0; z < 3; ++z) {
        for (int x = 0; x < 5; ++x) {
            auto mat = make_ref<MeshStandardMaterial>();
            mat->color = glm::mix(glm::vec3(0.92f, 0.28f, 0.12f), glm::vec3(0.12f, 0.46f, 0.95f), float(x) / 4.0f);
            mat->roughness = glm::mix(0.20f, 0.85f, float(z) / 2.0f);
            mat->metalness = x == 4 ? 0.75f : 0.0f;
            auto mesh = make_ref<Mesh>(((x + z) & 1) ? boxGeo : sphereGeo, mat);
            mesh->position = {float(x - 2) * 1.75f, ((x + z) & 1) ? 0.58f : 0.63f, float(z - 1) * 1.9f};
            mesh->castShadow = true;
            mesh->receiveShadow = true;
            scene.add(mesh);
        }
    }

    auto gridMat = make_ref<LineBasicMaterial>();
    gridMat->color = {0.12f, 0.13f, 0.15f};
    auto grid = make_ref<LineSegments>(GeometryFactory::makeGrid(24, 24.0f), gridMat);
    grid->position.y = 0.006f;
    scene.add(grid);

    GLRenderer renderer({1440, 860});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);

    while (!window.shouldClose()) {
        window.poll();
        float t = static_cast<float>(window.time());
        camera.position = {std::sin(t * 0.22f) * 8.0f, 5.5f, std::cos(t * 0.22f) * 8.5f};
        camera.lookAt({0.0f, 0.9f, 0.0f});
        renderer.render(scene, camera);
        window.swapBuffers();
    }
}
