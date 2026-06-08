#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "core/Renderable.hpp"
#include "light/Light.hpp"
#include "ibl/Environment.hpp"
#include "helpers/GeometryFactory.hpp"

using namespace threecpp;

int main() {
    Window window(1280, 720, "threecpp 57 furnace test");
    Scene scene;
    PerspectiveCamera camera(55.0f, 1280.0f / 720.0f, 0.1f, 100.0f);
    camera.position = {0.0f, 1.5f, 4.0f};
    camera.lookAt({0.0f, 0.5f, 0.0f});

    int cols = 5, rows = 2;
    float spacing = 0.7f;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            auto mat = make_ref<MeshStandardMaterial>();
            mat->color = {0.85f, 0.72f, 0.55f};
            mat->roughness = static_cast<float>(c) / static_cast<float>(cols - 1);
            mat->metalness = static_cast<float>(r);
            mat->envMapIntensity = 1.0f;
            auto mesh = make_ref<Mesh>(GeometryFactory::makeSphere(0.28f, 48, 32), mat);
            mesh->position = {
                (static_cast<float>(c) - static_cast<float>(cols - 1) * 0.5f) * spacing,
                (static_cast<float>(r) - static_cast<float>(rows - 1) * 0.5f) * spacing + 1.0f,
                0.0f
            };
            scene.add(mesh);
        }
    }

    auto planeMat = make_ref<MeshStandardMaterial>();
    planeMat->color = {0.25f, 0.25f, 0.25f};
    planeMat->roughness = 0.8f;
    planeMat->metalness = 0.0f;
    auto plane = make_ref<Mesh>(GeometryFactory::makePlane(5.0f), planeMat);
    plane->position = {0.0f, 0.3f, 0.0f};
    plane->rotation = {-1.5708f, 0.0f, 0.0f};
    scene.add(plane);

    auto hemi = make_ref<HemisphereLight>();
    hemi->skyColor = {0.4f, 0.6f, 1.0f};
    hemi->groundColor = {0.2f, 0.15f, 0.05f};
    hemi->intensity = 0.6f;
    scene.add(hemi);

    PMREMGenerator pmrem({64, 6, true});
    auto hdrTex = make_ref<Texture>();
    hdrTex->width = 2;
    hdrTex->height = 1;
    hdrTex->channels = 3;
    hdrTex->pixels = {
        std::byte(180), std::byte(200), std::byte(255),
        std::byte(100), std::byte(120), std::byte(180)
    };
    hdrTex->colorSpace = ColorSpace::LinearSRGB;
    hdrTex->mapping = TextureMapping::EquirectangularReflection;
    auto env = pmrem.fromEquirectangular(hdrTex);
    scene.environment = env;

    GLRenderer renderer({1280, 720, true, {0.1f, 0.1f, 0.12f, 1.0f}});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);
    while (!window.shouldClose()) {
        window.pollEvents();
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
