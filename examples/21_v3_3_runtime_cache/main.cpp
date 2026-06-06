#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "renderer/RenderQueues.hpp"
#include "helpers/GeometryFactory.hpp"
#include "core/Renderable.hpp"
#include "light/Light.hpp"
#include "texture/TextureFactory.hpp"
#include "ibl/Environment.hpp"
#include <iostream>

using namespace threecpp;

int main() {
    Window window(1280, 720, "threecpp v3.3 runtime cache stress");
    Scene scene;
    scene.backgroundColor = {0.018f, 0.020f, 0.026f};
    scene.environment = PMREMGenerator({64, 6, true, 32, 32}).fromEquirectangular(TextureFactory::makeEquirectangularGradient(256, 128));

    PerspectiveCamera camera(50.0f, window.aspect(), 0.05f, 250.0f);
    camera.position = {7.0f, 5.0f, 9.0f};
    camera.lookAt({0.0f, 0.0f, 0.0f});

    auto light = make_ref<DirectionalLight>();
    light->position = {5.0f, 9.0f, 5.0f};
    light->intensity = 5.0f;
    scene.add(light);

    auto sharedGeo = GeometryFactory::makeUVSphere(0.24f, 24, 12);
    std::vector<std::shared_ptr<MeshStandardMaterial>> materials;
    for (int i = 0; i < 8; ++i) {
        auto mat = make_ref<MeshStandardMaterial>();
        mat->color = glm::vec3(0.3f + 0.08f * float(i), 0.45f, 0.9f - 0.08f * float(i));
        mat->roughness = 0.15f + 0.1f * float(i);
        mat->metalness = (i % 3 == 0) ? 0.7f : 0.0f;
        materials.push_back(mat);
    }

    for (int z = 0; z < 18; ++z) {
        for (int x = 0; x < 24; ++x) {
            auto mesh = make_ref<Mesh>(sharedGeo, materials[std::size_t((x + z) % materials.size())]);
            mesh->position = {float(x - 12) * 0.62f, 0.0f, float(z - 9) * 0.62f};
            scene.add(mesh);
        }
    }

    GLRenderer renderer({1280, 720});
    double lastReport = 0.0;
    while (!window.shouldClose()) {
        window.poll();
        float t = static_cast<float>(window.time());
        camera.position = {std::sin(t * 0.22f) * 9.0f, 5.2f, std::cos(t * 0.22f) * 9.0f};
        camera.lookAt({0.0f, 0.0f, 0.0f});
        renderer.render(scene, camera);
        if (window.time() - lastReport > 2.0) {
            lastReport = window.time();
            std::cout << "calls=" << renderer.info.calls << " tris=" << renderer.info.triangles << " programs=" << renderer.info.programs << "\n";
        }
        window.swapBuffers();
    }
    return 0;
}
