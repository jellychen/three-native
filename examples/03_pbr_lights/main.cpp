#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "core/Renderable.hpp"
#include "light/Light.hpp"
#include "helpers/GeometryFactory.hpp"
#include "texture/TextureFactory.hpp"

using namespace threecpp;

int main() {
    Window window(1280, 720, "threecpp 03 pbr lights");
    Scene scene;
    scene.environment = make_ref<Environment>();
    scene.environmentIntensity = 1.0f;

    PerspectiveCamera camera(55.0f, 1280.0f / 720.0f, 0.1f, 200.0f);
    camera.position = {4.0f, 3.0f, 7.0f};
    camera.quaternion = glm::quatLookAt(glm::normalize(glm::vec3(0, 0.8f, 0) - camera.position), glm::vec3(0, 1, 0));

    auto ambient = make_ref<AmbientLight>();
    ambient->color = {0.08f, 0.09f, 0.12f};
    ambient->intensity = 1.0f;
    scene.add(ambient);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {5, 6, 3};
    sun->target = {0, 0, 0};
    sun->intensity = 4.0f;
    scene.add(sun);

    auto point = make_ref<PointLight>();
    point->position = {-2, 2, 2};
    point->color = {1.0f, 0.55f, 0.25f};
    point->intensity = 16.0f;
    point->distance = 12.0f;
    scene.add(point);

    auto sphereGeo = GeometryFactory::makeUVSphere(0.48f, 48, 24);
    auto checker = TextureFactory::makeCheckerboard(256, 256, 8, {1.0f, 0.82f, 0.45f}, {0.35f, 0.12f, 0.08f});
    for (int x = 0; x < 5; ++x) {
        for (int y = 0; y < 3; ++y) {
            auto mat = make_ref<MeshStandardMaterial>();
            mat->color = glm::mix(glm::vec3(0.95f, 0.35f, 0.18f), glm::vec3(0.9f, 0.72f, 0.45f), float(x) / 4.0f);
            mat->metalness = float(x) / 4.0f;
            mat->roughness = glm::clamp(float(y) / 2.0f, 0.05f, 1.0f);
            if (x == 0 && y == 0) mat->map = checker;
            auto mesh = make_ref<Mesh>(sphereGeo, mat);
            mesh->position = {float(x - 2) * 1.35f, float(y) * 1.15f + 0.55f, 0.0f};
            scene.add(mesh);
        }
    }

    auto groundMat = make_ref<MeshBasicMaterial>();
    groundMat->color = {0.08f, 0.08f, 0.09f};
    auto ground = make_ref<Mesh>(GeometryFactory::makePlane(12.0f), groundMat);
    scene.add(ground);

    GLRenderer renderer({1280, 720});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);
    while (!window.shouldClose()) {
        window.pollEvents();
        renderer.render(scene, camera);
        window.swapBuffers();
    }
}
