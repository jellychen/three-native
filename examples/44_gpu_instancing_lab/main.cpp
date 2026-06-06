#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "core/Renderable.hpp"
#include "controls/OrbitControls.hpp"
#include "helpers/GeometryFactory.hpp"
#include "helpers/Helpers.hpp"
#include "light/Light.hpp"
#include <iostream>

using namespace threecpp;

static glm::vec4 instanceColor(int i) {
    float h = float(i % 97) / 97.0f;
    return {
        0.45f + 0.45f * std::sin(h * glm::two_pi<float>() + 0.0f) * 0.5f + 0.20f,
        0.45f + 0.45f * std::sin(h * glm::two_pi<float>() + 2.1f) * 0.5f + 0.20f,
        0.45f + 0.45f * std::sin(h * glm::two_pi<float>() + 4.2f) * 0.5f + 0.20f,
        1.0f
    };
}

int main() {
    Window window(1280, 720, "threecpp v5.1 GPU InstancedMesh lab");
    GLRenderer renderer({1280, 720});
    renderer.setClearColor({0.015f, 0.017f, 0.022f, 1.0f});

    Scene scene;
    scene.backgroundColor = {0.015f, 0.017f, 0.022f};

    PerspectiveCamera camera(55.0f, 1280.0f / 720.0f, 0.1f, 800.0f);
    camera.position = {42.0f, 34.0f, 58.0f};
    camera.lookAt({0.0f, 0.0f, 0.0f});
    OrbitControls controls(window, camera);
    controls.target = {0.0f, 0.0f, 0.0f};
    controls.distance = 76.0f;

    scene.add(make_ref<GridHelper>(96.0f, 96, glm::vec3(0.16f)));
    scene.add(make_ref<AxesHelper>(5.0f));

    auto geometry = GeometryFactory::makeUVSphere(0.32f, 16, 8);
    auto material = make_ref<MeshStandardMaterial>();
    material->color = {1.0f, 1.0f, 1.0f};
    material->roughness = 0.48f;
    material->metalness = 0.12f;
    material->vertexColors = true; // instanceColor multiplies vertex color path in shader

    constexpr int gridX = 100;
    constexpr int gridZ = 100;
    constexpr int instanceCount = gridX * gridZ;
    auto instanced = make_ref<InstancedMesh>(geometry, material, instanceCount);
    instanced->castShadow = true;
    instanced->receiveShadow = true;

    int idx = 0;
    const float spacing = 0.92f;
    for (int z = 0; z < gridZ; ++z) {
        for (int x = 0; x < gridX; ++x) {
            float fx = (float(x) - float(gridX - 1) * 0.5f) * spacing;
            float fz = (float(z) - float(gridZ - 1) * 0.5f) * spacing;
            float height = 0.45f + 0.35f * std::sin(float(x) * 0.31f) * std::cos(float(z) * 0.27f);
            glm::mat4 m(1.0f);
            m = glm::translate(m, glm::vec3(fx, height, fz));
            m = glm::scale(m, glm::vec3(0.72f + 0.22f * std::sin(float(idx) * 0.07f)));
            instanced->setMatrixAt(idx, m);
            instanced->setColorAt(idx, instanceColor(idx));
            ++idx;
        }
    }
    scene.add(instanced);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {22.0f, 42.0f, 26.0f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->intensity = 2.7f;
    sun->castShadow = true;
    sun->shadow.mapSize = {2048, 2048};
    sun->shadow.cameraLeft = -58.0f;
    sun->shadow.cameraRight = 58.0f;
    sun->shadow.cameraTop = 58.0f;
    sun->shadow.cameraBottom = -58.0f;
    sun->shadow.cameraFar = 140.0f;
    scene.add(sun);

    auto hemi = make_ref<HemisphereLight>();
    hemi->skyColor = {0.42f, 0.54f, 0.82f};
    hemi->groundColor = {0.10f, 0.09f, 0.075f};
    hemi->intensity = 0.55f;
    scene.add(hemi);

    auto point = make_ref<PointLight>();
    point->color = {0.42f, 0.62f, 1.0f};
    point->intensity = 18.0f;
    point->distance = 72.0f;
    point->decay = 2.0f;
    scene.add(point);

    std::cout << "v5.1 GPU InstancedMesh lab: instances=" << instanceCount
              << " expected drawCalls≈1 for instanced sphere pass (+helpers/shadows)" << std::endl;

    double lastPrint = 0.0;
    while (!window.shouldClose()) {
        window.poll();
        float t = static_cast<float>(window.time());
        point->position = {std::cos(t * 0.75f) * 24.0f, 10.0f + std::sin(t * 1.2f) * 4.0f, std::sin(t * 0.75f) * 24.0f};
        sun->position = {std::cos(t * 0.18f) * 34.0f, 42.0f, std::sin(t * 0.18f) * 34.0f};

        // Update a small subset every frame to verify dynamic instance buffer upload
        // without turning the test into a CPU bottleneck.
        for (int n = 0; n < 128; ++n) {
            int i = (n * 73 + int(t * 60.0f)) % instanceCount;
            int x = i % gridX;
            int z = i / gridX;
            float fx = (float(x) - float(gridX - 1) * 0.5f) * spacing;
            float fz = (float(z) - float(gridZ - 1) * 0.5f) * spacing;
            float h = 0.5f + 0.75f * std::sin(t * 2.2f + float(i) * 0.017f);
            glm::mat4 m(1.0f);
            m = glm::translate(m, glm::vec3(fx, h, fz));
            m = glm::rotate(m, t + float(i) * 0.001f, glm::vec3(0, 1, 0));
            m = glm::scale(m, glm::vec3(0.72f));
            instanced->setMatrixAt(i, m);
        }

        controls.update();
        renderer.render(scene, camera);
        if (window.time() - lastPrint > 1.0) {
            lastPrint = window.time();
            std::cout << "calls=" << renderer.info.calls
                      << " instancedCalls=" << renderer.info.instancedCalls
                      << " instances=" << renderer.info.instances
                      << " triangles=" << renderer.info.triangles
                      << " programs=" << renderer.info.programs << std::endl;
        }
        window.swapBuffers();
        if (window.keyPressed(GLFW_KEY_ESCAPE)) window.requestClose();
    }
    return 0;
}
