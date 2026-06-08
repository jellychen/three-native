#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "light/light.h"
#include "helpers/geometry-factory.h"
#include "helpers/helpers.h"
#include "ibl/environment.h"

using namespace THREE;


int main() {
    Window window(1440, 860, "threecpp v3.4 moving lights test");

    Scene scene;
    scene.backgroundColor = {0.012f, 0.014f, 0.020f};
    scene.environment = make_ref<Environment>();
    scene.environment->skyColor = {0.10f, 0.13f, 0.20f};
    scene.environment->groundColor = {0.02f, 0.018f, 0.016f};
    scene.environment->specularColor = {0.40f, 0.50f, 0.70f};
    scene.environmentIntensity = 0.4f;

    PerspectiveCamera camera(52.0f, 1440.0f / 860.0f, 0.05f, 220.0f);
    camera.position = {7.5f, 5.0f, 9.0f};
    camera.lookAt({0.0f, 1.0f, 0.0f});

    auto ambient = make_ref<AmbientLight>();
    ambient->color = {0.015f, 0.018f, 0.025f};
    ambient->intensity = 1.0f;
    scene.add(ambient);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {5.0f, 8.0f, 5.0f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->color = {0.80f, 0.88f, 1.0f};
    sun->intensity = 2.2f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.mapSizeX = 1024;
    sun->shadow.mapSizeY = 1024;
    sun->shadow.cameraLeft = -12.0f;
    sun->shadow.cameraRight = 12.0f;
    sun->shadow.cameraBottom = -12.0f;
    sun->shadow.cameraTop = 12.0f;
    sun->shadow.bias = 0.0013f;
    scene.add(sun);

    auto redPoint = make_ref<PointLight>();
    redPoint->color = {1.0f, 0.14f, 0.08f};
    redPoint->intensity = 90.0f;
    redPoint->distance = 12.0f;
    redPoint->decay = 2.0f;
    scene.add(redPoint);

    auto bluePoint = make_ref<PointLight>();
    bluePoint->color = {0.15f, 0.42f, 1.0f};
    bluePoint->intensity = 85.0f;
    bluePoint->distance = 12.0f;
    bluePoint->decay = 2.0f;
    scene.add(bluePoint);

    auto movingSpot = make_ref<SpotLight>();
    movingSpot->color = {0.70f, 1.0f, 0.55f};
    movingSpot->intensity = 95.0f;
    movingSpot->distance = 18.0f;
    movingSpot->decay = 2.0f;
    movingSpot->angle = glm::radians(22.0f);
    movingSpot->penumbra = 0.65f;
    movingSpot->castShadow = true;
    movingSpot->shadow.enabled = true;
    movingSpot->shadow.mapSizeX = 1024;
    movingSpot->shadow.mapSizeY = 1024;
    movingSpot->shadow.cameraNear = 0.2f;
    movingSpot->shadow.cameraFar = 20.0f;
    movingSpot->shadow.bias = 0.0008f;
    movingSpot->shadow.radius = 1.0f;
    scene.add(movingSpot);

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.32f, 0.33f, 0.35f};
    groundMat->roughness = 0.72f;
    auto ground = make_ref<Mesh>(GeometryFactory::makePlane(22.0f), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    auto sphereGeo = GeometryFactory::makeUVSphere(0.55f, 48, 24);
    for (int i = 0; i < 11; ++i) {
        auto mat = make_ref<MeshStandardMaterial>();
        mat->color = glm::mix(glm::vec3(0.82f, 0.82f, 0.78f), glm::vec3(0.08f, 0.16f, 0.24f), float(i) / 10.0f);
        mat->roughness = glm::mix(0.18f, 0.88f, float(i) / 10.0f);
        mat->metalness = i > 7 ? 0.65f : 0.0f;
        auto mesh = make_ref<Mesh>(sphereGeo, mat);
        float a = float(i) / 11.0f * glm::two_pi<float>();
        mesh->position = {std::cos(a) * 3.7f, 0.56f, std::sin(a) * 3.7f};
        mesh->castShadow = true;
        mesh->receiveShadow = true;
        scene.add(mesh);
    }

    auto centerMat = make_ref<MeshStandardMaterial>();
    centerMat->color = {0.93f, 0.75f, 0.22f};
    centerMat->roughness = 0.28f;
    centerMat->metalness = 0.85f;
    auto center = make_ref<Mesh>(GeometryFactory::makeTorus(1.15f, 0.18f, 64, 16), centerMat);
    center->position.y = 1.1f;
    center->castShadow = true;
    center->receiveShadow = true;
    scene.add(center);

    GLRenderer renderer({1440, 860});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);

    while (!window.shouldClose()) {
        window.poll();
        float t = static_cast<float>(window.time());
        redPoint->position = {std::cos(t * 1.20f) * 4.2f, 1.65f + std::sin(t * 2.0f) * 0.35f, std::sin(t * 1.20f) * 4.2f};
        bluePoint->position = {std::cos(t * 0.85f + 3.14f) * 4.8f, 2.15f, std::sin(t * 0.85f + 3.14f) * 4.8f};
        movingSpot->position = {std::cos(t * 0.55f) * 5.0f, 5.2f, std::sin(t * 0.55f) * 5.0f};
        movingSpot->target = {std::sin(t * 0.9f) * 1.8f, 0.4f, std::cos(t * 0.7f) * 1.8f};
        sun->position = {std::cos(t * 0.17f) * 6.0f, 8.0f, std::sin(t * 0.17f) * 6.0f};
        center->rotation.y = t * 0.8f;
        camera.position = {std::sin(t * 0.18f) * 8.5f, 5.0f, std::cos(t * 0.18f) * 9.0f};
        camera.lookAt({0.0f, 1.0f, 0.0f});
        renderer.render(scene, camera);
        window.swapBuffers();
    }
}
