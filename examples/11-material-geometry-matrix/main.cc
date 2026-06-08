#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "light/light.h"
#include "helpers/geometry-factory.h"
#include "texture/texture-factory.h"
#include "ibl/environment.h"
#include <iostream>

using namespace THREE;

int main() {
    Window window(1280, 720, "threecpp v1.2 material / geometry matrix");
    window.poll(); // Let macOS display the window before CPU-side setup work.
    Scene scene;
    scene.backgroundColor = {0.018f, 0.020f, 0.026f};
    // Keep this example responsive on macOS/Retina: a full 256px CPU PMREM can block
    // before the OS has displayed the window. Use a fast preview PMREM here; the
    // dedicated PMREM examples can use larger cube sizes for quality testing.
    window.pollEvents();
    PMREMOptions pmremOptions;
    pmremOptions.cubeSize = 64;
    pmremOptions.maxMipLevels = 6;
    pmremOptions.generateBRDFLUT = true;
    pmremOptions.irradianceSamples = 32;
    pmremOptions.prefilterSamples = 32;
    PMREMGenerator pmrem(pmremOptions);
    scene.environment = pmrem.fromEquirectangular(TextureFactory::makeEquirectangularGradient(256, 128));
    scene.environmentIntensity = 1.2f;

    PerspectiveCamera camera(55.0f, 1280.0f / 720.0f, 0.1f, 200.0f);
    camera.position = {5.5f, 3.2f, 8.0f};
    camera.lookAt({0.0f, 0.8f, 0.0f});

    auto ambient = make_ref<AmbientLight>();
    ambient->intensity = 0.04f;
    scene.add(ambient);

    auto key = make_ref<DirectionalLight>();
    key->position = {5.0f, 8.0f, 4.0f};
    key->target = {0.0f, 0.0f, 0.0f};
    key->intensity = 3.2f;
    key->castShadow = true;
    key->shadow.enabled = true;
    scene.add(key);

    auto fill = make_ref<PointLight>();
    fill->position = {-3.0f, 2.5f, 3.5f};
    fill->color = {0.55f, 0.72f, 1.0f};
    fill->intensity = 14.0f;
    fill->distance = 12.0f;
    scene.add(fill);

    auto checker = TextureFactory::makeCheckerboard(256, 256, 8, {0.94f, 0.72f, 0.36f}, {0.08f, 0.08f, 0.09f});
    auto sphere = GeometryFactory::makeUVSphere(0.45f, 64, 32);
    auto torus = GeometryFactory::makeTorus(0.45f, 0.16f, 64, 18);
    auto cyl = GeometryFactory::makeCylinder(0.36f, 0.5f, 1.0f, 48, 2, true);

    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 6; ++col) {
            auto mat = make_ref<MeshStandardMaterial>();
            mat->color = glm::mix(glm::vec3(0.92f, 0.35f, 0.22f), glm::vec3(0.75f, 0.82f, 0.95f), float(col) / 5.0f);
            mat->metalness = float(col) / 5.0f;
            mat->roughness = glm::clamp(0.06f + float(row) * 0.42f, 0.045f, 1.0f);
            mat->envMapIntensity = 1.0f;
            if (row == 0 && col == 0) mat->map = checker;
            std::shared_ptr<BufferGeometry> geo = row == 0 ? sphere : (row == 1 ? torus : cyl);
            auto mesh = make_ref<Mesh>(geo, mat);
            mesh->position = {float(col - 2.5f) * 1.15f, float(2 - row) * 1.25f + 0.4f, 0.0f};
            mesh->castShadow = true;
            mesh->receiveShadow = true;
            scene.add(mesh);
        }
    }

    auto physical = make_ref<MeshPhysicalMaterial>();
    physical->color = {0.68f, 0.9f, 1.0f};
    physical->roughness = 0.03f;
    physical->metalness = 0.0f;
    physical->ior = 1.45f;
    physical->transmission = 0.72f;
    physical->thickness = 0.55f;
    physical->attenuationColor = {0.8f, 0.95f, 1.0f};
    physical->attenuationDistance = 2.0f;
    physical->clearcoat = 0.8f;
    physical->clearcoatRoughness = 0.05f;
    physical->transparent = true;
    auto glass = make_ref<Mesh>(GeometryFactory::makeUVSphere(0.65f, 64, 32), physical);
    glass->position = {0.0f, -2.0f, 0.2f};
    scene.add(glass);

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.16f, 0.16f, 0.17f};
    groundMat->roughness = 0.86f;
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(10.0f, 5.0f, 24, 12), groundMat);
    ground->position = {0.0f, -2.75f, 0.0f};
    ground->receiveShadow = true;
    scene.add(ground);

    GLRenderer renderer({1280, 720});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);
    while (!window.shouldClose()) {
        window.pollEvents();
        float t = static_cast<float>(window.time());
        camera.position = {std::sin(t * 0.25f) * 7.0f, 3.2f, std::cos(t * 0.25f) * 7.0f};
        camera.lookAt({0.0f, 0.1f, 0.0f});
        camera.updateProjectionMatrix();
        auto fb = window.framebufferSize();
        renderer.setSize(fb.x, fb.y);
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
