#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "renderer/RenderQueues.hpp"
#include "helpers/GeometryFactory.hpp"
#include "core/Renderable.hpp"
#include "light/Light.hpp"
#include "ibl/Environment.hpp"
#include "texture/TextureFactory.hpp"
#include <iostream>

using namespace threecpp;

int main() {
    Window window(1280, 720, "threecpp v3.9 renderer architecture lab");

    Scene scene;
    scene.backgroundColor = {0.018f, 0.020f, 0.028f};
    scene.environment = PMREMGenerator({64, 6, true, 32, 32}).fromEquirectangular(TextureFactory::makeEquirectangularGradient(256, 128));

    PerspectiveCamera camera(50.0f, window.aspect(), 0.05f, 200.0f);
    camera.position = {7.0f, 5.0f, 9.0f};
    camera.lookAt({0.0f, 0.0f, 0.0f});

    auto ambient = make_ref<AmbientLight>();
    ambient->intensity = 0.12f;
    scene.add(ambient);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {5.0f, 8.0f, 4.0f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->intensity = 4.0f;
    scene.add(sun);

    auto geo = GeometryFactory::makeUVSphere(0.45f, 32, 16);
    for (int i = 0; i < 30; ++i) {
        auto mat = make_ref<MeshStandardMaterial>();
        mat->color = glm::vec3(0.25f + 0.02f * float(i % 8), 0.55f, 0.95f - 0.02f * float(i % 8));
        mat->roughness = 0.15f + 0.75f * float(i % 10) / 9.0f;
        mat->metalness = (i % 3 == 0) ? 1.0f : 0.0f;
        auto mesh = make_ref<Mesh>(geo, mat);
        mesh->position = {float(i % 10 - 5) * 1.05f, 0.0f, float(i / 10 - 1) * 1.15f};
        mesh->renderOrder = 0.0f;
        scene.add(mesh);
    }

    auto glassMat = make_ref<MeshPhysicalMaterial>();
    glassMat->color = {0.8f, 0.95f, 1.0f};
    glassMat->roughness = 0.05f;
    glassMat->metalness = 0.0f;
    glassMat->transmission = 0.8f;
    glassMat->transparent = true;
    glassMat->opacity = 0.55f;
    glassMat->depthWrite = false;
    auto glass = make_ref<Mesh>(geo, glassMat);
    glass->position = {0.0f, 1.15f, 0.0f};
    glass->renderOrder = 1.0f;
    scene.add(glass);

    auto overlayMat = make_ref<MeshBasicMaterial>();
    overlayMat->color = {1.0f, 0.4f, 0.15f};
    overlayMat->transparent = true;
    overlayMat->opacity = 0.35f;
    overlayMat->depthWrite = false;
    auto overlay = make_ref<Mesh>(GeometryFactory::makeCube(1.0f), overlayMat);
    overlay->position = {0.0f, 0.65f, 0.0f};
    overlay->scale = {2.0f, 0.04f, 2.0f};
    overlay->renderOrder = 2.0f;
    scene.add(overlay);

    GLRenderer renderer({1280, 720});
    renderer.setScissor(0, 0, 1280, 720);
    renderer.setScissorTest(false);

    double lastReport = 0.0;
    while (!window.shouldClose()) {
        window.poll();
        float t = static_cast<float>(window.time());
        camera.position = {std::sin(t * 0.25f) * 8.0f, 4.2f, std::cos(t * 0.25f) * 8.0f};
        camera.lookAt({0.0f, 0.25f, 0.0f});

        glass->rotation.y = t * 0.6f;
        overlay->rotation.y = -t * 0.35f;

        renderer.render(scene, camera);
        if (window.time() - lastReport > 2.0) {
            lastReport = window.time();
            std::cout << "calls=" << renderer.info.calls
                      << " tris=" << renderer.info.triangles
                      << " programs=" << renderer.info.programs
                      << " -- v3.9 queues: opaque/transmissive/transparent are sorted separately\n";
        }
        window.swapBuffers();
    }
    return 0;
}
