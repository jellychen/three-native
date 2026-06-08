#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "light/light.h"
#include "helpers/geometry-factory.h"
#include "helpers/helpers.h"
#include "texture/texture-factory.h"
#include "ibl/environment.h"
#include "controls/orbit-controls.h"

using namespace THREE;

static std::shared_ptr<MeshStandardMaterial> mat(const glm::vec3& c, float r = 0.55f, float m = 0.0f) {
    auto out = make_ref<MeshStandardMaterial>();
    out->color = c;
    out->roughness = r;
    out->metalness = m;
    return out;
}

int main() {
    Window window(1440, 860, "threecpp v6.0.36 SpotLight shadow only lab");
    window.poll();

    Scene scene;
    scene.backgroundColor = {0.010f, 0.012f, 0.018f, 1.0f};

    // Fast-start environment: avoid synchronous PMREM generation in focused shadow labs.
    // This removes the startup black pause and keeps shadow validation isolated.
    scene.environment = make_ref<Environment>();
    scene.environment->skyColor = {0.03f, 0.035f, 0.045f};
    scene.environment->groundColor = {0.018f, 0.017f, 0.016f};
    scene.environment->specularColor = {0.06f, 0.055f, 0.05f};
    scene.environmentIntensity = 0.10f;

    PerspectiveCamera camera(50.0f, 1440.0f / 860.0f, 0.05f, 200.0f);
    camera.position = {8.0f, 5.5f, 8.5f};
    camera.lookAt({0.0f, 0.8f, 0.0f});
    OrbitControls controls(camera, window);
    controls.target = {0.0f, 0.7f, 0.0f};

    // Spot-light-only scene. No ambient/hemisphere/directional/point lights.
    auto spot = make_ref<SpotLight>();
    spot->position = {3.5f, 6.0f, 4.5f};
    spot->target = {0.0f, 0.0f, 0.0f};
    spot->color = {1.0f, 0.90f, 0.72f};
    spot->intensity = 72.0f;
    spot->distance = 18.0f;
    spot->decay = 2.0f;
    spot->angle = glm::radians(30.0f);
    spot->penumbra = 0.28f;
    spot->castShadow = true;
    spot->shadow.enabled = true;
    spot->shadow.mapSize = {2048, 2048};
    spot->shadow.cameraNear = 0.15f;
    spot->shadow.cameraFar = 24.0f;
    spot->shadow.bias = 0.0025f;
    spot->shadow.radius = 2.0f;
    spot->shadow.intensity = 1.0f;
    scene.add(spot);

    auto groundMat = mat({0.50f, 0.50f, 0.47f}, 0.86f, 0.0f);
    groundMat->map = TextureFactory::makeCheckerboard(512, 512, 16, {0.54f,0.54f,0.50f}, {0.26f,0.27f,0.30f});
    groundMat->map->repeat = {7, 7};
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(20.0f, 16.0f, 18, 14), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    auto cubeGeo = GeometryFactory::makeCube(1.05f);
    auto sphereGeo = GeometryFactory::makeUVSphere(0.52f, 48, 24);
    auto torusGeo = GeometryFactory::makeTorus(0.45f, 0.13f, 48, 16);

    std::vector<std::shared_ptr<Mesh>> movers;
    for (int i = 0; i < 14; ++i) {
        float x = float(i % 7 - 3) * 1.25f;
        float z = float(i / 7 == 0 ? -1.2f : 1.35f);
        auto geo = (i % 3 == 0) ? cubeGeo : ((i % 3 == 1) ? sphereGeo : torusGeo);
        auto material = mat(glm::mix(glm::vec3(1.0f, 0.38f, 0.18f), glm::vec3(0.25f, 0.66f, 1.0f), float(i) / 13.0f), 0.20f + 0.62f * float(i % 5) / 4.0f, (i % 5 == 0) ? 0.65f : 0.0f);
        auto mesh = make_ref<Mesh>(geo, material);
        mesh->position = {x, 0.60f, z};
        mesh->castShadow = true;
        mesh->receiveShadow = true;
        scene.add(mesh);
        if (i % 2 == 0) movers.push_back(mesh);
    }

    auto helper = make_ref<SpotLightHelper>(*spot, 4.0f);
    scene.add(helper);

    GLRenderer renderer({1440, 860});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);

    while (!window.shouldClose()) {
        window.poll();
        controls.update();
        float t = static_cast<float>(window.time());

        spot->position = {std::cos(t * 0.45f) * 4.2f, 5.8f, std::sin(t * 0.45f) * 4.2f};
        spot->target = {std::sin(t * 0.33f) * 0.9f, 0.0f, std::cos(t * 0.29f) * 0.7f};
        helper->update(*spot);

        for (std::size_t i = 0; i < movers.size(); ++i) {
            movers[i]->rotation.y = t * 0.65f + float(i);
            movers[i]->position.y = 0.60f + std::abs(std::sin(t * 0.8f + float(i) * 0.7f)) * 0.75f;
        }

        renderer.render(scene, camera);
        window.swapBuffers();
    }
}
