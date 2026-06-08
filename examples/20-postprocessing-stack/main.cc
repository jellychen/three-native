#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "postprocessing/effect-composer.h"
#include "postprocessing/passes.h"
#include "helpers/geometry-factory.h"
#include "core/renderable.h"
#include "light/light.h"
#include "ibl/environment.h"
#include "texture/texture-factory.h"

using namespace THREE;

int main() {
    Window window(1280, 720, "threecpp v3.3 postprocessing stack");
    Scene scene;
    scene.backgroundColor = {0.018f, 0.020f, 0.026f};
    scene.environment = PMREMGenerator({64, 6, true, 32, 32}).fromEquirectangular(TextureFactory::makeEquirectangularGradient(256, 128));

    PerspectiveCamera camera(45.0f, window.aspect(), 0.05f, 200.0f);
    camera.position = {4.5f, 3.2f, 7.0f};
    camera.lookAt({0.0f, 0.8f, 0.0f});

    auto ambient = make_ref<AmbientLight>();
    ambient->intensity = 0.7f;
    scene.add(ambient);
    auto sun = make_ref<DirectionalLight>();
    sun->position = {5.0f, 7.0f, 5.0f};
    sun->intensity = 5.5f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    scene.add(sun);

    auto sphereGeo = GeometryFactory::makeUVSphere(0.55f, 64, 32);
    for (int i = 0; i < 7; ++i) {
        auto mat = make_ref<MeshStandardMaterial>();
        mat->color = glm::mix(glm::vec3(1.0f, 0.45f, 0.18f), glm::vec3(0.35f, 0.62f, 1.0f), float(i) / 6.0f);
        mat->roughness = 0.08f + 0.13f * float(i);
        mat->metalness = float(i) / 6.0f;
        mat->envMapIntensity = 1.25f;
        auto mesh = make_ref<Mesh>(sphereGeo, mat);
        mesh->position = {float(i - 3) * 1.25f, 0.7f, 0.0f};
        mesh->castShadow = true;
        mesh->receiveShadow = true;
        scene.add(mesh);
    }

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->roughness = 0.82f;
    groundMat->color = {0.42f, 0.42f, 0.38f};
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(12.0f, 8.0f, 8, 8), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    GLRenderer renderer({1280, 720});
    EffectComposer composer(renderer);
    composer.addPass<RenderPass>();
    composer.addPass<ToneMappingPass>(ToneMapping::ACESFilmic, 1.1f);
    composer.addPass<BloomPass>(1.0f, 0.25f, 0.3f);
    composer.addPass<FXAAPass>();

    while (!window.shouldClose()) {
        window.poll();
        float t = static_cast<float>(window.time());
        camera.position = {std::sin(t * 0.25f) * 6.0f, 3.2f, std::cos(t * 0.25f) * 6.0f};
        camera.lookAt({0.0f, 0.7f, 0.0f});
        composer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
