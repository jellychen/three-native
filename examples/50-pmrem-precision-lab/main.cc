#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "helpers/geometry-factory.h"
#include "helpers/helpers.h"
#include "light/light.h"
#include "texture/texture-factory.h"
#include "texture/texture-loader.h"
#include "ibl/environment.h"
#include "controls/orbit-controls.h"
#include <iostream>

using namespace THREE;

static std::shared_ptr<Texture> loadEnv(int argc, char** argv) {
    if (argc > 1) {
        std::filesystem::path path(argv[1]);
        try {
            if (path.extension() == ".hdr" || path.extension() == ".HDR") return TextureLoader::loadRGBE(path);
            if (path.extension() == ".pfm" || path.extension() == ".PFM") return TextureLoader::loadPFMAsRGB16FPlaceholder(path);
        } catch (const std::exception& e) {
            std::cerr << "Failed to load env map, using procedural studio HDRI: " << e.what() << "\n";
        }
    }
    return TextureFactory::makeStudioHDRI(1024, 512);
}

int main(int argc, char** argv) {
    Window window(1600, 900, "threecpp v5.7 PMREM precision / cache lab");

    auto hdr = loadEnv(argc, argv);
    PMREMOptions opts;
    opts.cubeSize = 256;
    opts.maxMipLevels = 9;
    opts.irradianceSamples = 128;
    opts.prefilterSamples = 192;
    opts.enableCache = true;
    PMREMGenerator pmrem(opts);

    auto envA = pmrem.fromEquirectangular(hdr);
    auto envB = pmrem.fromEquirectangular(hdr); // intentional cache-hit validation
    auto stats = pmrem.cacheStats();
    std::cout << "PMREM cache: requests=" << stats.requests
              << " hits=" << stats.hits
              << " misses=" << stats.misses
              << " live=" << stats.liveEntries << "\n";
    std::cout << "PMREM: cubeSize=" << envA->pmremCubeSize
              << " mipLevels=" << envA->pmremMipLevels
              << " hasPMREM=" << envA->hasPMREM
              << " secondCallCacheHit=" << envB->pmremCacheHit << "\n";

    Scene scene;
    scene.backgroundColor = {0.015f, 0.016f, 0.020f};
    scene.environment = envA;
    scene.environment->intensity = 1.35f;
    scene.environment->backgroundIntensity = 1.0f;
    scene.environment->backgroundBlurriness = 0.25f;
    scene.environmentIntensity = 1.35f;

    PerspectiveCamera camera(45.0f, 1600.0f / 900.0f, 0.05f, 500.0f);
    camera.position = {10.0f, 5.0f, 12.0f};
    camera.lookAt({0.0f, 1.0f, 0.0f});
    OrbitControls controls(camera, window);
    controls.target = {0.0f, 1.0f, 0.0f};

    auto hemi = make_ref<HemisphereLight>();
    hemi->skyColor = {0.25f, 0.34f, 0.52f};
    hemi->groundColor = {0.035f, 0.032f, 0.030f};
    hemi->intensity = 0.12f;
    scene.add(hemi);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {4.0f, 7.0f, 4.5f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->intensity = 1.0f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.mapSize = {2048, 2048};
    sun->shadow.cameraLeft = -10.0f;
    sun->shadow.cameraRight = 10.0f;
    sun->shadow.cameraTop = 8.0f;
    sun->shadow.cameraBottom = -8.0f;
    scene.add(sun);

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.52f, 0.50f, 0.46f};
    groundMat->roughness = 0.84f;
    groundMat->metalness = 0.0f;
    groundMat->envMapIntensity = 0.65f;
    groundMat->map = TextureFactory::makeCheckerboard(512, 512, 16, {0.58f,0.58f,0.56f}, {0.28f,0.29f,0.31f});
    groundMat->map->repeat = {7.0f, 5.0f};
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(20.0f, 15.0f, 24, 24), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    auto sphereGeo = GeometryFactory::makeUVSphere(0.52f, 80, 40);
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 9; ++col) {
            auto mat = make_ref<MeshStandardMaterial>();
            float rough = glm::clamp(float(col) / 8.0f, 0.02f, 1.0f);
            float metal = glm::clamp(float(row) / 4.0f, 0.0f, 1.0f);
            mat->roughness = rough;
            mat->metalness = metal;
            mat->envMapIntensity = 1.35f;
            mat->color = glm::mix(glm::vec3(1.0f, 0.68f, 0.28f), glm::vec3(0.65f, 0.80f, 1.0f), metal);
            auto mesh = make_ref<Mesh>(sphereGeo, mat);
            mesh->position = {-6.4f + col * 1.6f, 0.62f, -3.8f + row * 1.45f};
            mesh->castShadow = true;
            mesh->receiveShadow = true;
            scene.add(mesh);
        }
    }

    auto clearcoatMat = make_ref<MeshPhysicalMaterial>();
    clearcoatMat->color = {0.95f, 0.10f, 0.05f};
    clearcoatMat->roughness = 0.42f;
    clearcoatMat->metalness = 0.0f;
    clearcoatMat->clearcoat = 1.0f;
    clearcoatMat->clearcoatRoughness = 0.05f;
    clearcoatMat->envMapIntensity = 1.5f;
    auto clearcoat = make_ref<Mesh>(GeometryFactory::makeTorus(1.1f, 0.23f, 128, 32), clearcoatMat);
    clearcoat->position = {-3.5f, 3.0f, -4.4f};
    clearcoat->rotation.x = glm::radians(65.0f);
    clearcoat->castShadow = true;
    scene.add(clearcoat);

    auto glassMat = make_ref<MeshPhysicalMaterial>();
    glassMat->color = {0.80f, 0.93f, 1.0f};
    glassMat->roughness = 0.08f;
    glassMat->metalness = 0.0f;
    glassMat->transmission = 0.85f;
    glassMat->thickness = 0.75f;
    glassMat->attenuationColor = {0.72f, 0.90f, 1.0f};
    glassMat->attenuationDistance = 2.5f;
    glassMat->ior = 1.48f;
    glassMat->transparent = true;
    glassMat->depthWrite = false;
    auto glass = make_ref<Mesh>(GeometryFactory::makeUVSphere(0.92f, 96, 48), glassMat);
    glass->position = {3.7f, 1.15f, -4.2f};
    glass->castShadow = true;
    scene.add(glass);

    auto grid = make_ref<GridHelper>(20.0f, 20);
    grid->position.y = 0.015f;
    scene.add(grid);

    GLRenderer renderer({1600, 900});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);
    renderer.setOutputColorSpace(ColorSpace::SRGB);

    bool iblEnabled = true;
    std::cout << "Controls: wheel dolly, LMB orbit, ESC exit. Press I to toggle IBL.\n";

    while (!window.shouldClose()) {
        window.poll();
        float t = static_cast<float>(window.time());
        if (window.keyPressed(GLFW_KEY_I)) iblEnabled = !iblEnabled;
        scene.environmentIntensity = iblEnabled ? 1.35f : 0.0f;
        scene.environment->intensity = scene.environmentIntensity;
        scene.environment->environmentRotation = glm::mat3(glm::rotate(glm::mat4(1.0f), 0.35f * std::sin(t * 0.25f), glm::vec3(0, 1, 0)));
        scene.environment->backgroundRotation = glm::mat3(glm::rotate(glm::mat4(1.0f), 0.18f * std::sin(t * 0.18f), glm::vec3(0, 1, 0)));
        clearcoat->rotation.z = t * 0.27f;
        controls.update();
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
