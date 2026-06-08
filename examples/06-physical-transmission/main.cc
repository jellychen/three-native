#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "light/light.h"
#include "helpers/geometry-factory.h"

using namespace THREE;

int main() {
    Window window(1280, 720, "threecpp 06 physical transmission");
    Scene scene;
    scene.environment = make_ref<Environment>();
    scene.environmentIntensity = 1.2f;

    PerspectiveCamera camera(50.0f, 1280.0f / 720.0f, 0.1f, 200.0f);
    camera.position = {4.0f, 2.6f, 7.0f};
    camera.quaternion = glm::quatLookAt(glm::normalize(glm::vec3(0, 0.9f, 0) - camera.position), glm::vec3(0, 1, 0));

    auto ambient = make_ref<AmbientLight>();
    ambient->color = {0.12f, 0.14f, 0.18f};
    ambient->intensity = 1.0f;
    scene.add(ambient);

    auto key = make_ref<DirectionalLight>();
    key->position = {4, 5, 4};
    key->target = {0, 0.8f, 0};
    key->intensity = 5.0f;
    scene.add(key);

    auto rim = make_ref<PointLight>();
    rim->position = {-3, 2.2f, 2.5f};
    rim->color = {0.35f, 0.6f, 1.0f};
    rim->intensity = 18.0f;
    rim->distance = 12.0f;
    scene.add(rim);

    auto sphereGeo = GeometryFactory::makeUVSphere(0.58f, 64, 32);

    auto glass = make_ref<MeshPhysicalMaterial>();
    glass->name = "transmission glass approximation";
    glass->color = {0.72f, 0.92f, 1.0f};
    glass->roughness = 0.04f;
    glass->metalness = 0.0f;
    glass->ior = 1.52f;
    glass->transmission = 0.85f;
    glass->thickness = 0.65f;
    glass->attenuationDistance = 2.8f;
    glass->attenuationColor = {0.82f, 0.94f, 1.0f};
    glass->clearcoat = 0.8f;
    glass->clearcoatRoughness = 0.05f;
    glass->transparent = true;
    glass->opacity = 0.82f;

    auto glassSphere = make_ref<Mesh>(sphereGeo, glass);
    glassSphere->position = {-1.45f, 1.0f, 0.0f};
    scene.add(glassSphere);

    auto coated = make_ref<MeshPhysicalMaterial>();
    coated->name = "clearcoat sheen iridescence";
    coated->color = {0.9f, 0.22f, 0.11f};
    coated->roughness = 0.38f;
    coated->metalness = 0.0f;
    coated->clearcoat = 1.0f;
    coated->clearcoatRoughness = 0.12f;
    coated->sheen = 0.65f;
    coated->sheenColor = {1.0f, 0.45f, 0.25f};
    coated->sheenRoughness = 0.5f;
    coated->iridescence = 0.35f;

    auto coatSphere = make_ref<Mesh>(sphereGeo, coated);
    coatSphere->position = {0.0f, 1.0f, 0.0f};
    scene.add(coatSphere);

    auto metal = make_ref<MeshPhysicalMaterial>();
    metal->name = "physical metal with specular controls";
    metal->color = {0.93f, 0.78f, 0.45f};
    metal->metalness = 1.0f;
    metal->roughness = 0.22f;
    metal->specularIntensity = 1.0f;
    metal->specularColor = {1.0f, 0.94f, 0.82f};
    metal->clearcoat = 0.35f;
    metal->clearcoatRoughness = 0.18f;

    auto metalSphere = make_ref<Mesh>(sphereGeo, metal);
    metalSphere->position = {1.45f, 1.0f, 0.0f};
    scene.add(metalSphere);

    auto groundMat = make_ref<MeshBasicMaterial>();
    groundMat->color = {0.055f, 0.058f, 0.065f};
    auto ground = make_ref<Mesh>(GeometryFactory::makePlane(10.0f), groundMat);
    scene.add(ground);

    auto gridMat = make_ref<LineBasicMaterial>();
    gridMat->color = {0.22f, 0.23f, 0.26f};
    scene.add(make_ref<LineSegments>(GeometryFactory::makeGrid(20, 10.0f), gridMat));

    GLRenderer renderer({1280, 720, true, {0.012f, 0.014f, 0.018f, 1.0f}});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);

    while (!window.shouldClose()) {
        window.pollEvents();
        float t = static_cast<float>(glfwGetTime());
        glassSphere->quaternion = glm::angleAxis(t * 0.35f, glm::normalize(glm::vec3(0.0f, 1.0f, 0.15f)));
        coatSphere->quaternion = glm::angleAxis(t * 0.45f, glm::normalize(glm::vec3(0.25f, 1.0f, 0.1f)));
        metalSphere->quaternion = glm::angleAxis(t * 0.25f, glm::vec3(0, 1, 0));
        renderer.render(scene, camera);
        window.swapBuffers();
    }

    return 0;
}
