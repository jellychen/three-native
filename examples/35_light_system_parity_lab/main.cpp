#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "core/Renderable.hpp"
#include "light/Light.hpp"
#include "helpers/GeometryFactory.hpp"
#include "helpers/Helpers.hpp"
#include "controls/OrbitControls.hpp"
#include "ibl/Environment.hpp"

using namespace threecpp;

static std::shared_ptr<MeshStandardMaterial> mat(const glm::vec3& color, float roughness, float metalness = 0.0f) {
    auto m = make_ref<MeshStandardMaterial>();
    m->color = color;
    m->roughness = roughness;
    m->metalness = metalness;
    return m;
}

int main() {
    Window window(1440, 860, "threecpp v4.2 light system parity lab");

    Scene scene;
    scene.backgroundColor = {0.015f, 0.017f, 0.022f};
    scene.environment = make_ref<Environment>();
    scene.environment->skyColor = {0.16f, 0.20f, 0.28f};
    scene.environment->groundColor = {0.035f, 0.032f, 0.030f};
    scene.environment->specularColor = {0.75f, 0.82f, 0.95f};
    scene.environment->envMapIntensity = 0.65f;

    PerspectiveCamera camera(50.0f, 1440.0f / 860.0f, 0.05f, 260.0f);
    camera.position = {8.5f, 5.8f, 9.5f};
    camera.lookAt({0.0f, 1.0f, 0.0f});
    OrbitControls controls(window, camera);
    controls.target = {0.0f, 1.0f, 0.0f};

    auto ambient = make_ref<AmbientLight>();
    ambient->color = {0.025f, 0.030f, 0.040f};
    ambient->intensity = 1.0f;
    scene.add(ambient);

    auto hemi = make_ref<HemisphereLight>();
    hemi->skyColor = {0.32f, 0.42f, 0.66f};
    hemi->groundColor = {0.10f, 0.075f, 0.050f};
    hemi->intensity = 0.75f;
    scene.add(hemi);

    auto sun = make_ref<DirectionalLight>();
    sun->name = "moving directional";
    sun->position = {7.0f, 11.0f, 6.0f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->color = {1.0f, 0.94f, 0.82f};
    sun->intensity = 3.8f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.mapSize = {2048, 2048};
    sun->shadow.cameraLeft = -14.0f;
    sun->shadow.cameraRight = 14.0f;
    sun->shadow.cameraBottom = -14.0f;
    sun->shadow.cameraTop = 14.0f;
    sun->shadow.cameraNear = 0.2f;
    sun->shadow.cameraFar = 70.0f;
    sun->shadow.bias = 0.0009f;
    sun->shadow.normalBias = 0.02f;
    sun->shadow.radius = 1.5f;
    scene.add(sun);

    auto point = make_ref<PointLight>();
    point->name = "moving point";
    point->position = {3.0f, 2.4f, -3.5f};
    point->color = {1.0f, 0.48f, 0.20f};
    point->intensity = 70.0f;
    point->distance = 16.0f;
    point->decay = 2.0f;
    scene.add(point);

    auto spot = make_ref<SpotLight>();
    spot->name = "moving spot";
    spot->position = {-4.5f, 6.5f, 4.0f};
    spot->target = {0.0f, 0.8f, 0.0f};
    spot->color = {0.58f, 0.78f, 1.0f};
    spot->intensity = 90.0f;
    spot->distance = 22.0f;
    spot->decay = 2.0f;
    spot->angle = glm::radians(23.0f);
    spot->penumbra = 0.55f;
    spot->castShadow = true;
    spot->shadow.enabled = true;
    spot->shadow.mapSize = {1024, 1024};
    spot->shadow.cameraNear = 0.2f;
    spot->shadow.cameraFar = 35.0f;
    spot->shadow.bias = 0.0008f;
    spot->shadow.normalBias = 0.015f;
    spot->shadow.radius = 1.0f;
    scene.add(spot);

    auto area = make_ref<RectAreaLight>();
    area->name = "rect area approximation";
    area->position = {0.0f, 4.2f, -5.0f};
    area->target = {0.0f, 0.8f, 0.0f};
    area->color = {0.40f, 0.95f, 0.78f};
    area->intensity = 9.0f;
    area->width = 4.0f;
    area->height = 1.5f;
    scene.add(area);

    auto groundMat = mat({0.42f, 0.42f, 0.39f}, 0.86f);
    auto ground = make_ref<Mesh>(GeometryFactory::makePlane(26.0f), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    auto sphereGeo = GeometryFactory::makeUVSphere(0.55f, 48, 24);
    auto cubeGeo = GeometryFactory::makeCube(1.0f);
    auto torusGeo = GeometryFactory::makeTorus(0.52f, 0.16f, 48, 16);
    for (int z = 0; z < 4; ++z) {
        for (int x = 0; x < 6; ++x) {
            float u = float(x) / 5.0f;
            float v = float(z) / 3.0f;
            auto m = mat(glm::mix(glm::vec3(0.90f, 0.18f, 0.10f), glm::vec3(0.10f, 0.42f, 0.95f), u), glm::mix(0.18f, 0.88f, v), x >= 4 ? 0.75f : 0.0f);
            std::shared_ptr<BufferGeometry> g = (x % 3 == 0) ? sphereGeo : ((x % 3 == 1) ? cubeGeo : torusGeo);
            auto mesh = make_ref<Mesh>(g, m);
            mesh->position = {float(x - 2.5f) * 1.55f, 0.58f, float(z - 1.5f) * 1.55f};
            mesh->castShadow = true;
            mesh->receiveShadow = true;
            scene.add(mesh);
        }
    }

    auto gridMat = make_ref<LineBasicMaterial>();
    gridMat->color = {0.10f, 0.11f, 0.13f};
    auto grid = make_ref<LineSegments>(GeometryFactory::makeGrid(26, 26.0f), gridMat);
    grid->position.y = 0.006f;
    scene.add(grid);

    GLRenderer renderer({1440, 860});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);

    while (!window.shouldClose()) {
        window.poll();
        const float t = static_cast<float>(window.time());
        point->position = {std::sin(t * 1.25f) * 4.5f, 2.1f + std::sin(t * 1.7f) * 0.55f, std::cos(t * 1.25f) * 4.5f};
        spot->position = {std::sin(t * 0.65f) * 5.2f, 6.2f, std::cos(t * 0.65f) * 5.2f};
        spot->target = {std::sin(t * 0.5f) * 1.2f, 0.8f, std::cos(t * 0.5f) * 1.2f};
        sun->position = {std::sin(t * 0.24f) * 9.0f, 11.0f, std::cos(t * 0.24f) * 9.0f};
        sun->target = {0.0f, 0.0f, 0.0f};
        area->position.x = std::sin(t * 0.42f) * 3.0f;
        area->target = {0.0f, 0.75f, 0.0f};
        controls.update();
        renderer.render(scene, camera);
        window.swapBuffers();
    }
}
