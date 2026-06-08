#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "light/light.h"
#include "helpers/geometry-factory.h"

using namespace THREE;

int main() {
    Window window(1280, 720, "threecpp 07 transmission render target");
    Scene scene;
    scene.environment = make_ref<Environment>();
    scene.environmentIntensity = 1.0f;

    PerspectiveCamera camera(50.0f, 1280.0f / 720.0f, 0.1f, 200.0f);
    camera.position = {4.5f, 2.4f, 6.2f};
    camera.quaternion = glm::quatLookAt(glm::normalize(glm::vec3(0, 0.8f, 0) - camera.position), glm::vec3(0, 1, 0));

    auto ambient = make_ref<AmbientLight>();
    ambient->color = {0.16f, 0.17f, 0.2f};
    ambient->intensity = 1.0f;
    scene.add(ambient);

    auto key = make_ref<DirectionalLight>();
    key->position = {5, 5, 5};
    key->target = {0, 0, 0};
    key->intensity = 5.0f;
    scene.add(key);

    auto red = make_ref<MeshBasicMaterial>();
    red->color = {1.0f, 0.12f, 0.08f};
    auto green = make_ref<MeshBasicMaterial>();
    green->color = {0.1f, 0.9f, 0.24f};
    auto blue = make_ref<MeshBasicMaterial>();
    blue->color = {0.15f, 0.38f, 1.0f};

    auto cubeGeo = GeometryFactory::makeCube(0.75f);
    for (int i = 0; i < 9; ++i) {
        auto mat = (i % 3 == 0) ? red : ((i % 3 == 1) ? green : blue);
        auto cube = make_ref<Mesh>(cubeGeo, mat);
        cube->position = {float(i - 4) * 0.65f, 0.55f + 0.25f * float(i % 2), -1.4f};
        cube->quaternion = glm::angleAxis(0.35f * float(i), glm::normalize(glm::vec3(0.4f, 1.0f, 0.2f)));
        scene.add(cube);
    }

    auto glass = make_ref<MeshPhysicalMaterial>();
    glass->name = "framebuffer transmission glass";
    glass->color = {0.86f, 0.96f, 1.0f};
    glass->roughness = 0.03f;
    glass->metalness = 0.0f;
    glass->ior = 1.52f;
    glass->transmission = 0.95f;
    glass->thickness = 0.8f;
    glass->attenuationDistance = 3.0f;
    glass->attenuationColor = {0.82f, 0.94f, 1.0f};
    glass->clearcoat = 1.0f;
    glass->clearcoatRoughness = 0.03f;
    glass->transparent = true;
    glass->opacity = 0.88f;

    auto glassMesh = make_ref<Mesh>(GeometryFactory::makeUVSphere(1.05f, 80, 40), glass);
    glassMesh->position = {0.0f, 1.05f, 0.05f};
    scene.add(glassMesh);

    auto groundMat = make_ref<MeshBasicMaterial>();
    groundMat->color = {0.045f, 0.047f, 0.052f};
    scene.add(make_ref<Mesh>(GeometryFactory::makePlane(10.0f), groundMat));

    RendererParameters rp;
    rp.width = 1280;
    rp.height = 720;
    rp.clearColor = {0.012f, 0.014f, 0.018f, 1.0f};
    rp.transmission = true;
    rp.transmissionResolutionScale = 1.0f;
    rp.transmissionMipLevel = 4.0f;
    GLRenderer renderer(rp);
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);

    while (!window.shouldClose()) {
        window.pollEvents();
        float t = static_cast<float>(glfwGetTime());
        glassMesh->quaternion = glm::angleAxis(t * 0.45f, glm::normalize(glm::vec3(0.15f, 1.0f, 0.05f)));
        renderer.render(scene, camera);
        window.swapBuffers();
    }

    return 0;
}
