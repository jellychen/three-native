#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "core/Renderable.hpp"
#include "light/Light.hpp"
#include "helpers/GeometryFactory.hpp"
#include "helpers/Helpers.hpp"
#include "ibl/Environment.hpp"

using namespace threecpp;


static std::shared_ptr<Mesh> makeMarker(const glm::vec3& pos, const glm::vec3& color) {
    auto mat = make_ref<MeshBasicMaterial>();
    mat->color = color;
    auto m = make_ref<Mesh>(GeometryFactory::makeUVSphere(0.08f, 16, 8), mat);
    m->position = pos;
    return m;
}

int main() {
    Window window(1440, 860, "threecpp v3.4 light falloff + spot cone test");

    Scene scene;
    scene.backgroundColor = {0.020f, 0.020f, 0.026f};
    scene.environment = make_ref<Environment>();
    scene.environment->skyColor = {0.12f, 0.14f, 0.20f};
    scene.environment->groundColor = {0.03f, 0.028f, 0.025f};
    scene.environment->specularColor = {0.40f, 0.44f, 0.55f};
    scene.environmentIntensity = 0.06f;

    PerspectiveCamera camera(48.0f, 1440.0f / 860.0f, 0.05f, 200.0f);
    camera.position = {8.0f, 5.0f, 9.0f};
    camera.lookAt({0.0f, 0.7f, 0.0f});

    auto ambient = make_ref<AmbientLight>();
    ambient->color = {0.012f, 0.014f, 0.018f};
    ambient->intensity = 0.12f;
    scene.add(ambient);

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.40f, 0.40f, 0.40f};
    groundMat->roughness = 0.86f;
    auto ground = make_ref<Mesh>(GeometryFactory::makePlane(26.0f), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    auto point = make_ref<PointLight>();
    point->position = {-4.5f, 3.0f, 0.0f};
    point->color = {1.0f, 0.55f, 0.30f};
    point->intensity = 28.0f;
    point->distance = 5.0f;
    point->decay = 2.0f;
    scene.add(point);
    scene.add(makeMarker(point->position, point->color));

    auto spot = make_ref<SpotLight>();
    spot->position = {4.5f, 5.0f, 3.5f};
    spot->target = {4.5f, 0.0f, -1.0f};
    spot->color = {0.35f, 0.72f, 1.0f};
    spot->intensity = 520.0f;
    spot->distance = 14.0f;
    spot->decay = 2.0f;
    spot->angle = glm::radians(12.0f);
    spot->penumbra = 0.25f;
    spot->castShadow = true;
    spot->shadow.enabled = true;
    spot->shadow.mapSizeX = 1024;
    spot->shadow.mapSizeY = 1024;
    spot->shadow.cameraNear = 0.1f;
    spot->shadow.cameraFar = 16.0f;
    spot->shadow.bias = 0.0008f;
    spot->shadow.radius = 1.0f;
    scene.add(spot);
    scene.add(makeMarker(spot->position, spot->color));
    auto spotHelper = make_ref<SpotLightHelper>(*spot, 7.0f, 48);
    scene.add(spotHelper);

    auto obstacleMat = make_ref<MeshStandardMaterial>();
    obstacleMat->color = {0.78f, 0.78f, 0.76f};
    obstacleMat->roughness = 0.45f;
    auto sphereGeo = GeometryFactory::makeUVSphere(0.42f, 32, 16);
    for (int i = 0; i < 17; ++i) {
        auto mat = make_ref<MeshStandardMaterial>();
        mat->color = {0.80f, 0.78f, 0.72f};
        mat->roughness = 0.55f;
        mat->metalness = 0.0f;
        auto m = make_ref<Mesh>(sphereGeo, mat);
        m->position = {-8.0f + float(i), 0.43f, 0.0f};
        m->castShadow = true;
        m->receiveShadow = true;
        scene.add(m);
    }
    for (int i = 0; i < 9; ++i) {
        auto m = make_ref<Mesh>(GeometryFactory::makeCube(0.65f), obstacleMat);
        m->position = {4.5f, 0.33f, -4.0f + float(i)};
        m->castShadow = true;
        m->receiveShadow = true;
        scene.add(m);
    }

    auto gridMat = make_ref<LineBasicMaterial>();
    gridMat->color = {0.11f, 0.12f, 0.14f};
    auto grid = make_ref<LineSegments>(GeometryFactory::makeGrid(26, 26.0f), gridMat);
    grid->position.y = 0.004f;
    scene.add(grid);

    GLRenderer renderer({1440, 860});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);

    while (!window.shouldClose()) {
        window.poll();
        float t = static_cast<float>(window.time());
        // Animate the spot cone over a wide range so the illuminated footprint changes visibly.
        spot->penumbra = 0.05f + 0.70f * (0.5f + 0.5f * std::sin(t * 0.65f));
        spot->angle = glm::radians(8.0f + 30.0f * (0.5f + 0.5f * std::sin(t * 0.45f)));
        point->intensity = 18.0f + 16.0f * (0.5f + 0.5f * std::sin(t * 1.1f));
        spotHelper->rebuild(*spot);
        spotHelper->update(*spot);
        camera.position = {std::sin(t * 0.16f) * 8.0f, 5.4f, std::cos(t * 0.16f) * 10.0f};
        camera.lookAt({0.0f, 0.6f, 0.0f});
        renderer.render(scene, camera);
        window.swapBuffers();
    }
}
