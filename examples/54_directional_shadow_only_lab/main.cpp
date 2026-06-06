#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "core/Renderable.hpp"
#include "light/Light.hpp"
#include "helpers/GeometryFactory.hpp"
#include "helpers/Helpers.hpp"
#include "texture/TextureFactory.hpp"
#include "ibl/Environment.hpp"
#include "controls/OrbitControls.hpp"

using namespace threecpp;

static std::shared_ptr<MeshStandardMaterial> mat(const glm::vec3& c, float r = 0.55f, float m = 0.0f) {
    auto out = make_ref<MeshStandardMaterial>();
    out->color = c;
    out->roughness = r;
    out->metalness = m;
    return out;
}

int main() {
    Window window(1440, 860, "threecpp v6.0.36 DirectionalLight shadow only lab");
    window.poll();

    Scene scene;
    scene.backgroundColor = {0.012f, 0.014f, 0.020f, 1.0f};

    // Fast-start environment: avoid synchronous PMREM generation in focused shadow labs.
    // Keep only a very small analytic IBL fill so the DirectionalLight shadow is visible.
    scene.environment = make_ref<Environment>();
    scene.environment->skyColor = {0.03f, 0.035f, 0.045f};
    scene.environment->groundColor = {0.018f, 0.017f, 0.016f};
    scene.environment->specularColor = {0.06f, 0.055f, 0.05f};
    scene.environmentIntensity = 0.10f;

    PerspectiveCamera camera(50.0f, 1440.0f / 860.0f, 0.05f, 220.0f);
    camera.position = {9.0f, 6.2f, 9.0f};
    camera.lookAt({0.0f, 0.8f, 0.0f});
    OrbitControls controls(camera, window);
    controls.target = {0.0f, 0.8f, 0.0f};

    // Directional-light-only scene. No ambient/hemisphere/spot/point lights.
    auto sun = make_ref<DirectionalLight>();
    sun->position = {7.0f, 9.0f, 5.0f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->color = {1.0f, 0.95f, 0.86f};
    sun->intensity = 3.2f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.mapSize = {2048, 2048};
    sun->shadow.cameraLeft = -8.0f;
    sun->shadow.cameraRight = 8.0f;
    sun->shadow.cameraBottom = -8.0f;
    sun->shadow.cameraTop = 8.0f;
    sun->shadow.cameraNear = 0.1f;
    sun->shadow.cameraFar = 40.0f;
    sun->shadow.bias = 0.0025f;
    sun->shadow.normalBias = 0.0f;
    sun->shadow.radius = 2.0f;
    sun->shadow.intensity = 1.0f;
    scene.add(sun);

    auto groundMat = mat({0.62f, 0.61f, 0.56f}, 0.82f, 0.0f);
    groundMat->map = TextureFactory::makeCheckerboard(512, 512, 16, {0.62f,0.61f,0.56f}, {0.35f,0.36f,0.38f});
    groundMat->map->repeat = {8, 8};
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(22.0f, 18.0f, 20, 14), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    auto cubeGeo = GeometryFactory::makeCube(1.1f);
    auto sphereGeo = GeometryFactory::makeUVSphere(0.55f, 48, 24);
    auto torusGeo = GeometryFactory::makeTorus(0.45f, 0.13f, 48, 16);

    std::vector<std::shared_ptr<Mesh>> movers;
    for (int i = 0; i < 15; ++i) {
        float x = float(i % 5 - 2) * 1.8f;
        float z = float(i / 5 - 1) * 2.4f;
        auto geo = (i % 3 == 0) ? cubeGeo : ((i % 3 == 1) ? sphereGeo : torusGeo);
        auto material = mat(glm::mix(glm::vec3(1.0f, 0.42f, 0.24f), glm::vec3(0.18f, 0.55f, 1.0f), float(i) / 14.0f), 0.25f + 0.55f * float(i % 5) / 4.0f, (i % 4 == 0) ? 0.55f : 0.0f);
        auto mesh = make_ref<Mesh>(geo, material);
        mesh->position = {x, 0.62f, z};
        mesh->castShadow = true;
        mesh->receiveShadow = true;
        scene.add(mesh);
        if (i % 2 == 0) movers.push_back(mesh);
    }

    auto helper = make_ref<DirectionalLightHelper>(*sun, 1.0f);
    helper->position = sun->position;
    scene.add(helper);

    GLRenderer renderer({1440, 860});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);

    while (!window.shouldClose()) {
        window.poll();
        controls.update();
        float t = static_cast<float>(window.time());

        // Move only the directional light. The moving shadow should remain broad and stable.
        sun->position = {std::cos(t * 0.35f) * 8.0f, 9.0f, std::sin(t * 0.35f) * 8.0f};
        helper->position = sun->position;

        for (std::size_t i = 0; i < movers.size(); ++i) {
            movers[i]->rotation.y = t * 0.45f + float(i);
            movers[i]->position.y = 0.62f + std::abs(std::sin(t * 0.7f + float(i))) * 0.55f;
        }

        renderer.render(scene, camera);
        window.swapBuffers();
    }
}
