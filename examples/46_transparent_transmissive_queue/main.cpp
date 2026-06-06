#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "core/Renderable.hpp"
#include "controls/OrbitControls.hpp"
#include "helpers/GeometryFactory.hpp"
#include "helpers/Helpers.hpp"
#include "light/Light.hpp"
#include "texture/TextureFactory.hpp"
#include "ibl/Environment.hpp"
#include <cmath>
#include <iostream>

using namespace threecpp;

static std::shared_ptr<MeshStandardMaterial> opaqueMat(const glm::vec3& color, float metalness, float roughness) {
    auto m = make_ref<MeshStandardMaterial>();
    m->color = color;
    m->metalness = metalness;
    m->roughness = roughness;
    return m;
}

static std::shared_ptr<MeshStandardMaterial> stripeMat(const glm::vec3& color) {
    auto m = make_ref<MeshStandardMaterial>();
    m->color = color;
    m->roughness = 0.38f;
    m->metalness = 0.0f;
    m->emissive = color * 0.20f;
    m->emissiveIntensity = 0.55f;
    return m;
}

static std::shared_ptr<MeshPhysicalMaterial> glassMat(
    const glm::vec3& color,
    float transmission,
    float roughness,
    float thickness,
    float attenuationDistance) {
    auto m = make_ref<MeshPhysicalMaterial>();
    m->color = color;
    m->roughness = roughness;
    m->metalness = 0.0f;
    m->ior = 1.50f;
    m->transmission = transmission;
    m->thickness = thickness;
    m->attenuationColor = glm::mix(glm::vec3(1.0f), color, 0.65f);
    m->attenuationDistance = attenuationDistance;
    m->transparent = true;
    m->depthWrite = false;
    m->envMapIntensity = 1.0f;
    return m;
}

static void addReferenceBackdrop(Scene& scene) {
    // High-contrast opaque background: transmission should visibly refract these.
    auto wallGeo = GeometryFactory::makePlaneSegments(12.0f, 5.0f, 1, 1);
    auto dark = opaqueMat({0.035f, 0.040f, 0.050f}, 0.0f, 0.55f);
    auto wall = make_ref<Mesh>(wallGeo, dark);
    wall->position = {0.0f, 2.6f, -3.2f};
    wall->rotation.x = glm::radians(90.0f);
    wall->receiveShadow = true;
    scene.add(wall);

    const glm::vec3 colors[] = {
        {1.00f, 0.10f, 0.08f}, {0.08f, 0.65f, 1.00f}, {1.00f, 0.90f, 0.08f},
        {0.15f, 1.00f, 0.32f}, {1.00f, 0.20f, 0.85f}, {0.95f, 0.95f, 0.95f}
    };
    for (int i = 0; i < 12; ++i) {
        auto strip = make_ref<Mesh>(GeometryFactory::makePlaneSegments(0.48f, 4.4f, 1, 1), stripeMat(colors[i % 6]));
        strip->position = {-5.4f + i * 0.98f, 2.55f, -3.12f};
        strip->rotation.x = glm::radians(90.0f);
        scene.add(strip);
    }

    // Small opaque reference balls behind the glass.
    auto ballGeo = GeometryFactory::makeUVSphere(0.20f, 24, 12);
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 9; ++x) {
            auto mat = opaqueMat(glm::mix(glm::vec3(0.1f, 0.5f, 1.0f), glm::vec3(1.0f, 0.45f, 0.1f), float(x) / 8.0f), 0.0f, 0.35f);
            auto b = make_ref<Mesh>(ballGeo, mat);
            b->position = {-4.0f + x * 1.0f, 0.8f + y * 0.55f, -2.35f};
            b->castShadow = true;
            b->receiveShadow = true;
            scene.add(b);
        }
    }
}

int main() {
    Window window(1400, 820, "threecpp v6.0.47 glass thickness / Fresnel / rough transmission lab");
    window.poll();

    RendererParameters rp{1400, 820};
    rp.sortObjects = true;
    rp.transmission = true;
    rp.transmissionExcludesTransparent = true;
    rp.transmissionResolutionScale = 1.0f;
    rp.transmissionMipLevel = 4.0f;
    GLRenderer renderer(rp);
    renderer.initialize(window);
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.05f);
    renderer.setClearColor({0.012f, 0.014f, 0.018f}, 1.0f);

    Scene scene;
    scene.backgroundColor = {0.012f, 0.014f, 0.018f, 1.0f};
    // Keep IBL analytic/lightweight: this test is about transmission, not PMREM startup cost.
    scene.environment = make_ref<Environment>();
    scene.environment->skyColor = {0.16f, 0.19f, 0.24f};
    scene.environment->groundColor = {0.05f, 0.045f, 0.038f};
    scene.environment->specularColor = {0.55f, 0.62f, 0.72f};
    scene.environment->intensity = 0.18f;
    scene.environment->envMapIntensity = 0.18f;
    scene.environmentIntensity = 0.35f;

    PerspectiveCamera camera(50.0f, 1400.0f / 820.0f, 0.05f, 250.0f);
    camera.position = {7.5f, 4.2f, 8.0f};
    camera.lookAt({0.0f, 1.5f, 0.0f});
    OrbitControls controls(window, camera);
    controls.target = {0.0f, 1.35f, 0.0f};
    controls.distance = 10.5f;

    scene.add(make_ref<GridHelper>(14.0f, 14, glm::vec3(0.22f)));
    scene.add(make_ref<AxesHelper>(1.8f));
    addReferenceBackdrop(scene);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {5.0f, 6.0f, 4.0f};
    sun->target = {0.0f, 0.8f, 0.0f};
    sun->intensity = 2.2f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.mapSize = {1024, 1024};
    sun->shadow.cameraLeft = -7.0f;
    sun->shadow.cameraRight = 7.0f;
    sun->shadow.cameraBottom = -5.0f;
    sun->shadow.cameraTop = 7.0f;
    sun->shadow.cameraFar = 25.0f;
    sun->shadow.bias = 0.0008f;
    scene.add(sun);

    auto point = make_ref<PointLight>();
    point->position = {-3.2f, 3.2f, 2.6f};
    point->intensity = 18.0f;
    point->distance = 14.0f;
    point->decay = 2.0f;
    scene.add(point);

    auto groundMat = opaqueMat({0.46f, 0.45f, 0.42f}, 0.0f, 0.82f);
    groundMat->map = TextureFactory::makeCheckerboard(512, 512, 18, {0.55f,0.55f,0.51f}, {0.16f,0.17f,0.18f});
    groundMat->map->repeat = {6.0f, 4.0f};
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(14, 9, 14, 9), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    auto sphere = GeometryFactory::makeUVSphere(0.48f, 64, 32);

    // Top row: transmission amount, same thickness.
    for (int i = 0; i < 5; ++i) {
        float u = float(i) / 4.0f;
        auto mat = glassMat({0.68f, 0.88f, 1.0f}, u, 0.02f, 0.45f, 4.0f);
        auto obj = make_ref<Mesh>(sphere, mat);
        obj->position = {-2.8f + i * 1.4f, 2.25f, 0.95f};
        obj->castShadow = true;
        obj->receiveShadow = true;
        scene.add(obj);
    }

    // Middle row: increasing thickness/absorption.
    const float thicknesses[] = {0.0f, 0.45f, 1.1f, 2.2f, 3.8f};
    const glm::vec3 glassColors[] = {{1.0f,1.0f,1.0f}, {0.72f,0.95f,1.0f}, {0.55f,1.0f,0.78f}, {1.0f,0.76f,0.42f}, {0.85f,0.62f,1.0f}};
    for (int i = 0; i < 5; ++i) {
        auto mat = glassMat(glassColors[i], 1.0f, 0.035f, thicknesses[i], 0.85f + i * 0.55f);
        auto obj = make_ref<Mesh>(sphere, mat);
        obj->position = {-2.8f + i * 1.4f, 1.25f, 1.05f};
        obj->castShadow = true;
        obj->receiveShadow = true;
        scene.add(obj);
    }

    // Bottom row: increasing roughness should blur/soften transmission sampling.
    for (int i = 0; i < 5; ++i) {
        float u = float(i) / 4.0f;
        auto mat = glassMat({0.95f, 0.86f, 0.68f}, 1.0f, 0.02f + u * 0.75f, 0.9f, 3.0f);
        auto obj = make_ref<Mesh>(sphere, mat);
        obj->position = {-2.8f + i * 1.4f, 0.35f, 1.15f};
        obj->castShadow = true;
        obj->receiveShadow = true;
        scene.add(obj);
    }

    std::cout << "v6.0.47 transmission/thickness/Fresnel glass lab\n"
              << "Rows: transmission amount / thickness attenuation / roughness blur\n"
              << "Debug: THREECPP_DEBUG_TRANSMISSION_TARGET=1, THREECPP_DEBUG_TRANSMISSION_UV=1, "
              << "THREECPP_DEBUG_TRANSMISSION_ATTENUATION=1, THREECPP_DEBUG_TRANSMISSION_THICKNESS=1, "
              << "THREECPP_DEBUG_TRANSMISSION_FRESNEL=1, THREECPP_DISABLE_TRANSMISSION_CAPTURE=1\n";

    double lastPrint = 0.0;
    while (!window.shouldClose()) {
        window.poll();
        float t = static_cast<float>(window.time());
        controls.update();
        point->position = {std::cos(t * 0.45f) * 3.2f, 3.0f, 2.2f + std::sin(t * 0.45f) * 1.2f};
        renderer.render(scene, camera);
        if (window.time() - lastPrint > 1.0) {
            lastPrint = window.time();
            std::cout << "calls=" << renderer.info.calls
                      << " triangles=" << renderer.info.triangles
                      << " programs=" << renderer.info.programs << std::endl;
        }
        window.swapBuffers();
        if (window.keyPressed(GLFW_KEY_ESCAPE)) window.requestClose();
    }
    return 0;
}
