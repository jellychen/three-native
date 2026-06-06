#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "core/Renderable.hpp"
#include "light/Light.hpp"
#include "helpers/GeometryFactory.hpp"
#include "geometry/FatLineGeometry.hpp"

#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>

using namespace threecpp;

static std::shared_ptr<BufferGeometry> makePointCloud(int rings = 6, int pointsPerRing = 48) {
    auto g = std::make_shared<BufferGeometry>();
    std::vector<float> positions;
    std::vector<float> colors;
    for (int r = 0; r < rings; ++r) {
        float radius = 0.55f + 0.28f * float(r);
        float y = 0.12f * std::sin(float(r));
        for (int i = 0; i < pointsPerRing; ++i) {
            float a = glm::two_pi<float>() * float(i) / float(pointsPerRing);
            positions.insert(positions.end(), {radius * std::cos(a), y, radius * std::sin(a)});
            colors.insert(colors.end(), {
                0.4f + 0.5f * float(r) / float(std::max(rings - 1, 1)),
                0.65f,
                1.0f - 0.5f * float(r) / float(std::max(rings - 1, 1))
            });
        }
    }
    g->setAttribute("position", BufferAttribute::fromVector(positions, 3, AttributeType::Float32));
    g->setAttribute("color", BufferAttribute::fromVector(colors, 3, AttributeType::Float32));
    return g;
}

int main() {
    try {
        Window window(1280, 720, "threecpp v0.8 macOS GLFW native OpenGL renderer test");

        auto fb = window.framebufferSize();
        RendererParameters params;
        params.width = fb.x;
        params.height = fb.y;
        params.clearColor = {0.012f, 0.014f, 0.018f, 1.0f};
        params.transmission = true;
        params.transmissionResolutionScale = 1.0f;
        params.transmissionMipLevel = 4.0f;

        GLRenderer renderer(params);
        renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);

        Scene scene;
        scene.backgroundColor = {0.012f, 0.014f, 0.018f};
        scene.environment = make_ref<Environment>();
        scene.environmentIntensity = 1.0f;

        PerspectiveCamera camera(50.0f, window.aspect(), 0.1f, 200.0f);

        auto ambient = make_ref<AmbientLight>();
        ambient->color = {0.18f, 0.20f, 0.25f};
        ambient->intensity = 0.65f;
        scene.add(ambient);

        auto key = make_ref<DirectionalLight>();
        key->position = {5.0f, 6.5f, 4.0f};
        key->target = {0.0f, 0.8f, 0.0f};
        key->color = {1.0f, 0.96f, 0.88f};
        key->intensity = 5.0f;
        scene.add(key);

        auto fill = make_ref<PointLight>();
        fill->position = {-3.2f, 2.2f, 2.8f};
        fill->color = {0.45f, 0.65f, 1.0f};
        fill->intensity = 12.0f;
        fill->distance = 8.0f;
        scene.add(fill);

        auto gridMat = make_ref<LineBasicMaterial>();
        gridMat->color = {0.28f, 0.30f, 0.34f};
        auto grid = make_ref<LineSegments>(GeometryFactory::makeGrid(24, 12.0f), gridMat);
        grid->position.y = -0.02f;
        scene.add(grid);

        auto pbrRed = make_ref<MeshStandardMaterial>();
        pbrRed->color = {1.0f, 0.18f, 0.08f};
        pbrRed->roughness = 0.36f;
        pbrRed->metalness = 0.05f;
        auto sphereA = make_ref<Mesh>(GeometryFactory::makeUVSphere(0.72f, 64, 32), pbrRed);
        sphereA->position = {-1.75f, 0.74f, 0.0f};
        scene.add(sphereA);

        auto pbrMetal = make_ref<MeshStandardMaterial>();
        pbrMetal->color = {0.86f, 0.80f, 0.68f};
        pbrMetal->roughness = 0.18f;
        pbrMetal->metalness = 1.0f;
        auto sphereB = make_ref<Mesh>(GeometryFactory::makeUVSphere(0.72f, 64, 32), pbrMetal);
        sphereB->position = {0.0f, 0.74f, 0.0f};
        scene.add(sphereB);

        auto glass = make_ref<MeshPhysicalMaterial>();
        glass->color = {0.78f, 0.95f, 1.0f};
        glass->roughness = 0.02f;
        glass->metalness = 0.0f;
        glass->ior = 1.52f;
        glass->transmission = 0.92f;
        glass->thickness = 0.75f;
        glass->attenuationDistance = 3.5f;
        glass->attenuationColor = {0.84f, 0.95f, 1.0f};
        glass->clearcoat = 1.0f;
        glass->clearcoatRoughness = 0.03f;
        glass->transparent = true;
        glass->opacity = 0.88f;
        auto sphereC = make_ref<Mesh>(GeometryFactory::makeUVSphere(0.72f, 64, 32), glass);
        sphereC->position = {1.75f, 0.74f, 0.0f};
        scene.add(sphereC);

        auto cubeMat = make_ref<MeshBasicMaterial>();
        cubeMat->color = {0.08f, 0.55f, 1.0f};
        auto cube = make_ref<Mesh>(GeometryFactory::makeCube(0.42f), cubeMat);
        cube->position = {0.0f, 0.42f, -1.35f};
        scene.add(cube);

        auto pointsMat = make_ref<PointsMaterial>();
        pointsMat->color = {0.9f, 0.95f, 1.0f};
        pointsMat->size = 6.0f;
        pointsMat->vertexColors = true;
        auto points = make_ref<Points>(makePointCloud(), pointsMat);
        points->position = {0.0f, 1.7f, -0.1f};
        scene.add(points);

        std::vector<glm::vec3> curve;
        for (int i = 0; i < 96; ++i) {
            float u = float(i) / 95.0f;
            float x = -2.9f + 5.8f * u;
            float y = 1.7f + 0.25f * std::sin(u * glm::two_pi<float>() * 2.0f);
            float z = -1.1f + 0.4f * std::cos(u * glm::two_pi<float>() * 3.0f);
            curve.emplace_back(x, y, z);
        }
        auto fatMat = make_ref<FatLineMaterial>();
        fatMat->color = {1.0f, 0.74f, 0.18f};
        fatMat->linewidth = 7.0f;
        fatMat->resolution = {float(fb.x), float(fb.y)};
        auto fatLine = make_ref<FatLine>(FatLineGeometry::fromPolyline(curve), fatMat);
        scene.add(fatLine);

        auto groundMat = make_ref<MeshBasicMaterial>();
        groundMat->color = {0.045f, 0.047f, 0.052f};
        auto ground = make_ref<Mesh>(GeometryFactory::makePlane(12.0f), groundMat);
        scene.add(ground);

        std::cout << "threecpp v0.8 macOS GLFW native OpenGL renderer test running. Press ESC to exit.\n";

        while (!window.shouldClose()) {
            window.pollEvents();
            if (glfwGetKey(window.native(), GLFW_KEY_ESCAPE) == GLFW_PRESS) window.requestClose();

            auto currentFb = window.framebufferSize();
            if (currentFb.x != params.width || currentFb.y != params.height) {
                params.width = currentFb.x;
                params.height = currentFb.y;
                renderer.setSize(currentFb.x, currentFb.y);
                camera.aspect = window.aspect();
                camera.updateProjectionMatrix();
                fatMat->resolution = {float(currentFb.x), float(currentFb.y)};
            }

            float t = static_cast<float>(glfwGetTime());
            camera.position = {std::cos(t * 0.32f) * 5.2f, 2.4f, std::sin(t * 0.32f) * 5.2f};
            camera.quaternion = glm::quatLookAt(glm::normalize(glm::vec3(0.0f, 0.75f, 0.0f) - camera.position), glm::vec3(0, 1, 0));
            sphereA->quaternion = glm::angleAxis(t * 0.55f, glm::normalize(glm::vec3(0.2f, 1.0f, 0.0f)));
            sphereB->quaternion = glm::angleAxis(t * 0.35f, glm::normalize(glm::vec3(1.0f, 0.4f, 0.0f)));
            sphereC->quaternion = glm::angleAxis(t * 0.42f, glm::normalize(glm::vec3(0.15f, 1.0f, 0.05f)));
            cube->quaternion = glm::angleAxis(t, glm::normalize(glm::vec3(0.45f, 1.0f, 0.2f)));
            points->quaternion = glm::angleAxis(t * 0.18f, glm::vec3(0, 1, 0));

            renderer.render(scene, camera);
            window.swapBuffers();
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
}
