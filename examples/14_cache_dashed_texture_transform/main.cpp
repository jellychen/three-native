#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "helpers/GeometryFactory.hpp"
#include "texture/TextureFactory.hpp"

using namespace threecpp;

int main() {
    Window window(1280, 720, "threecpp v2.1 cache / dashed line / texture transform");
    window.poll(); // Let macOS display the window before CPU-side setup work.
    Scene scene;
    scene.backgroundColor = {0.018f, 0.018f, 0.022f};

    PerspectiveCamera camera(55.0f, 1280.0f / 720.0f, 0.1f, 100.0f);
    camera.position = {4.0f, 3.0f, 5.0f};
    camera.lookAt({0.0f, 0.4f, 0.0f});

    scene.environment = PMREMGenerator().fromEquirectangular(TextureFactory::makeEquirectangularGradient(512, 256));
    scene.environmentIntensity = 1.0f;

    auto ambient = make_ref<AmbientLight>();
    ambient->intensity = 0.18f;
    scene.add(ambient);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {4.0f, 5.0f, 3.0f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->intensity = 3.0f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    scene.add(sun);

    auto tex = TextureFactory::makeCheckerboard(256, 256, 8, {1.0f, 0.92f, 0.72f}, {0.08f, 0.10f, 0.13f});
    tex->wrapS = TextureWrap::Repeat;
    tex->wrapT = TextureWrap::Repeat;
    tex->repeat = {3.0f, 3.0f};
    tex->rotation = glm::radians(18.0f);
    tex->center = {0.5f, 0.5f};
    tex->markNeedsUpdate();

    auto mat = make_ref<MeshStandardMaterial>();
    mat->map = tex;
    mat->roughness = 0.62f;
    mat->metalness = 0.0f;

    auto plane = make_ref<Mesh>(GeometryFactory::makePlaneSegments(4.0f, 4.0f, 12, 12), mat);
    plane->receiveShadow = true;
    scene.add(plane);

    auto sphereMat = make_ref<MeshStandardMaterial>();
    sphereMat->color = {0.45f, 0.68f, 1.0f};
    sphereMat->roughness = 0.34f;
    sphereMat->metalness = 0.4f;
    auto sphere = make_ref<Mesh>(GeometryFactory::makeUVSphere(0.55f, 64, 32), sphereMat);
    sphere->position = {0.0f, 0.75f, 0.0f};
    sphere->castShadow = true;
    sphere->receiveShadow = true;
    scene.add(sphere);

    auto dashedGeo = GeometryFactory::makeGrid(24, 5.0f);
    dashedGeo->computeLineDistances(true);
    auto dashedMat = make_ref<LineDashedMaterial>();
    dashedMat->color = {0.85f, 0.72f, 0.35f};
    dashedMat->scale = 2.4f;
    dashedMat->dashSize = 0.28f;
    dashedMat->gapSize = 0.18f;
    auto dashed = make_ref<LineSegments>(dashedGeo, dashedMat);
    dashed->position.y = 0.012f;
    scene.add(dashed);

    GLRenderer renderer({1280, 720});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);

    while (!window.shouldClose()) {
        window.poll();
        auto fb = window.framebufferSize();
        camera.aspect = float(std::max(1, fb.x)) / float(std::max(1, fb.y));
        camera.updateProjectionMatrix();
        renderer.setSize(fb.x, fb.y);

        float t = static_cast<float>(window.time());
        sphere->position.x = std::sin(t) * 0.6f;
        // This touches only Object3D transforms. Geometry and texture GL caches
        // remain stable unless their version changes.
        sphere->rotation.y = t;
        sphere->matrixWorldNeedsUpdate = true;

        camera.position = {std::sin(t * 0.25f) * 5.0f, 3.0f, std::cos(t * 0.25f) * 5.0f};
        camera.lookAt({0.0f, 0.35f, 0.0f});
        renderer.render(scene, camera);
        window.swapBuffers();
    }
}
