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

int main() {
    Window window(1440, 860, "threecpp v6.0.34 PointLight shadow only lab");
    window.poll();
    Scene scene;
    scene.backgroundColor = {0.015f, 0.017f, 0.023f};

    // Fast-start environment: avoid synchronous PMREM generation in focused shadow labs.
    scene.environment = make_ref<Environment>();
    scene.environment->skyColor = {0.03f, 0.035f, 0.045f};
    scene.environment->groundColor = {0.018f, 0.017f, 0.016f};
    scene.environment->specularColor = {0.06f, 0.055f, 0.05f};
    scene.environmentIntensity = 0.08f; // kept low so PointLight shadow is not washed out

    PerspectiveCamera camera(52.0f, 1440.0f / 860.0f, 0.05f, 250.0f);
    camera.position = {9.5f, 6.0f, 9.0f};
    camera.lookAt({0, 1.0f, 0});
    OrbitControls controls(camera, window);
    controls.target = {0, 0.9f, 0};

    // Point-light-only test scene: intentionally no Ambient / Hemisphere / Directional / Spot lights.
    // This makes PointLight cubemap shadow contribution obvious and avoids other lights washing it out.
    auto point = make_ref<PointLight>();
    point->position = {3.5f, 3.2f, -3.5f};
    point->color = {1.0f, 0.92f, 0.72f};
    point->intensity = 185.0f;
    point->distance = 18.0f;
    point->decay = 2.0f;
    point->castShadow = true;
    point->shadow.enabled = true;
    point->shadow.mapSize = {2048, 2048};
    point->shadow.cameraNear = 0.12f;
    point->shadow.cameraFar = 18.0f;
    point->shadow.bias = 0.018f; // world-unit bias; normalized in shader by cameraFar.
    point->shadow.radius = 3.0f;
    point->shadow.intensity = 1.0f;
    scene.add(point);

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.52f, 0.52f, 0.48f};
    groundMat->roughness = 0.88f;
    groundMat->metalness = 0.0f;
    groundMat->map = TextureFactory::makeCheckerboard(512, 512, 16, {0.58f,0.58f,0.54f}, {0.30f,0.31f,0.34f});
    groundMat->map->repeat = {8, 8};
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(24.0f, 18.0f, 20, 14), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    auto sphereGeo = GeometryFactory::makeUVSphere(0.55f, 48, 24);
    auto cubeGeo = GeometryFactory::makeCube(1.05f);
    auto torusGeo = GeometryFactory::makeTorus(0.46f, 0.14f, 48, 14);
    std::vector<std::shared_ptr<Mesh>> movers;
    for (int i = 0; i < 18; ++i) {
        auto mat = make_ref<MeshStandardMaterial>();
        float f = float(i) / 17.0f;
        mat->color = glm::mix(glm::vec3(1.0f, 0.38f, 0.18f), glm::vec3(0.25f, 0.62f, 1.0f), f);
        mat->roughness = 0.18f + 0.66f * float(i % 5) / 4.0f;
        mat->metalness = (i % 4 == 0) ? 0.7f : 0.05f;
        auto geo = (i % 3 == 0) ? sphereGeo : ((i % 3 == 1) ? cubeGeo : torusGeo);
        auto mesh = make_ref<Mesh>(geo, mat);
        mesh->position = {float(i % 6 - 2.5f) * 1.55f, 0.65f, float(i / 6 - 1) * 2.35f};
        mesh->castShadow = true;
        mesh->receiveShadow = true;
        scene.add(mesh);
        if (i % 2 == 0) movers.push_back(mesh);
    }

    auto gridMat = make_ref<LineBasicMaterial>();
    gridMat->color = {0.08f, 0.09f, 0.11f};
    auto grid = make_ref<LineSegments>(GeometryFactory::makeGrid(24, 24), gridMat);
    grid->position.y = 0.012f;
    scene.add(grid);

    GLRenderer renderer({1440, 860});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);

    while (!window.shouldClose()) {
        window.poll();
        controls.update();
        float t = static_cast<float>(window.time());

        // Move only the point light. Its shadow should be the only dynamic shadow source.
        point->position = {
            std::cos(t * 0.65f) * 4.2f,
            3.0f + std::sin(t * 0.9f) * 0.65f,
            std::sin(t * 0.65f) * 4.2f
        };

        for (std::size_t i = 0; i < movers.size(); ++i) {
            movers[i]->position.y = 0.65f + std::abs(std::sin(t * 1.1f + float(i) * 0.7f)) * 1.15f;
            movers[i]->rotation.y = t * 0.75f + float(i);
        }

        renderer.render(scene, camera);
        window.swapBuffers();
    }
}
