#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/scene.h"
#include "core/camera.h"
#include "core/renderable.h"
#include "helpers/geometry-factory.h"
#include "texture/texture-factory.h"
#include "ibl/environment.h"
#include "light/light.h"
#include <iostream>

using namespace THREE;

int main() {
    Window window(1280, 720, "threecpp v1.1 PMREM / env evolution");
    GLRenderer renderer({1280, 720});
    Scene scene;
    scene.backgroundColor = {0.02f, 0.025f, 0.035f};

    PMREMGenerator pmrem({64, 6, true, 32, 32});
    scene.environment = pmrem.fromEquirectangular(TextureFactory::makeEquirectangularGradient());
    scene.environmentIntensity = 1.0f;

    PerspectiveCamera camera(55.0f, 1280.0f / 720.0f, 0.1f, 100.0f);
    camera.position = {0.0f, 2.2f, 7.0f};

    auto ambient = make_ref<AmbientLight>();
    ambient->intensity = 0.05f;
    scene.add(ambient);

    auto dir = make_ref<DirectionalLight>();
    dir->position = {4.0f, 7.0f, 3.0f};
    dir->target = {0.0f, 0.0f, 0.0f};
    dir->intensity = 2.2f;
    dir->castShadow = true;
    dir->shadow.enabled = true;
    scene.add(dir);

    auto sphereGeo = GeometryFactory::makeUVSphere(0.55f, 64, 32);
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 5; ++x) {
            auto mat = make_ref<MeshStandardMaterial>();
            mat->color = {0.85f, 0.82f, 0.75f};
            mat->metalness = float(x) / 4.0f;
            mat->roughness = glm::clamp(0.08f + float(y) * 0.36f, 0.045f, 1.0f);
            auto mesh = make_ref<Mesh>(sphereGeo, mat);
            mesh->position = {float(x - 2) * 1.35f, float(1 - y) * 1.25f, 0.0f};
            scene.add(mesh);
        }
    }

    std::cout << "PMREM placeholder: cubeSize=" << scene.environment->pmremCubeSize
              << " mipLevels=" << scene.environment->pmremMipLevels << "\n";

    while (!window.shouldClose()) {
        window.pollEvents();
        float t = static_cast<float>(window.time());
        camera.position = {std::sin(t * 0.35f) * 7.0f, 2.2f, std::cos(t * 0.35f) * 7.0f};
        camera.lookAt({0.0f, 0.0f, 0.0f});
        camera.updateProjectionMatrix();
        renderer.setSize(window.width(), window.height());
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
