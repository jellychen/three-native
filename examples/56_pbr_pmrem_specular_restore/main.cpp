#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "core/Renderable.hpp"
#include "helpers/GeometryFactory.hpp"
#include "helpers/Helpers.hpp"
#include "light/Light.hpp"
#include "texture/TextureFactory.hpp"
#include "texture/TextureLoader.hpp"
#include "ibl/Environment.hpp"
#include "controls/OrbitControls.hpp"
#include <iostream>

using namespace threecpp;

static std::shared_ptr<Texture> loadEnvironment(int argc, char** argv) {
    if (argc > 1) {
        std::filesystem::path p(argv[1]);
        try {
            if (p.extension() == ".hdr" || p.extension() == ".HDR") return TextureLoader::loadRGBE(p);
            if (p.extension() == ".pfm" || p.extension() == ".PFM") return TextureLoader::loadPFMAsRGB16FPlaceholder(p);
        } catch (const std::exception& e) {
            std::cerr << "[pmrem] failed to load environment, using procedural fallback: " << e.what() << "\n";
        }
    }
    return TextureFactory::makeStudioHDRI(768, 384);
}

int main(int argc, char** argv) {
    Window window(1500, 880, "threecpp v6.0.40 PBR PMREM specular restore lab");
    window.poll();

    auto hdr = loadEnvironment(argc, argv);
    PMREMOptions opts;
    // Fast-start defaults for interactive labs. The high quality PMREM path is
    // intentionally kept for dedicated quality tests, but this example should
    // show the window immediately instead of blocking the main thread on a
    // large CPU GGX prefilter pass.
    opts.cubeSize = 64;
    opts.maxMipLevels = 6;
    opts.irradianceSamples = 32;
    opts.prefilterSamples = 32;
    if (const char* v = std::getenv("THREECPP_PMREM_CUBE_SIZE")) { try { opts.cubeSize = std::max(16, std::stoi(v)); } catch (...) {} }
    if (const char* v = std::getenv("THREECPP_PMREM_IRRADIANCE_SAMPLES")) { try { opts.irradianceSamples = std::max(1, std::stoi(v)); } catch (...) {} }
    if (const char* v = std::getenv("THREECPP_PMREM_PREFILTER_SAMPLES")) { try { opts.prefilterSamples = std::max(1, std::stoi(v)); } catch (...) {} }
    opts.enableCache = true;
    PMREMGenerator pmrem(opts);
    auto env = pmrem.fromEquirectangular(hdr);
    auto stats = pmrem.cacheStats();
    std::cout << "PMREM cache requests=" << stats.requests << " hits=" << stats.hits
              << " misses=" << stats.misses << " live=" << stats.liveEntries
              << " cubeSize=" << opts.cubeSize
              << " irradianceSamples=" << opts.irradianceSamples
              << " prefilterSamples=" << opts.prefilterSamples << "\n";

    Scene scene;
    scene.backgroundColor = {0.012f, 0.014f, 0.018f, 1.0f};
    scene.environment = env;
    scene.environment->intensity = 1.1f;
    scene.environment->envMapIntensity = 1.0f;
    scene.environment->backgroundIntensity = 0.85f;
    scene.environment->backgroundBlurriness = 0.18f;
    scene.environmentIntensity = 1.1f;

    PerspectiveCamera camera(45.0f, 1500.0f / 880.0f, 0.05f, 300.0f);
    camera.position = {9.0f, 5.5f, 10.0f};
    camera.lookAt({0.0f, 1.0f, 0.0f});
    OrbitControls controls(camera, window);
    controls.target = {0.0f, 1.0f, 0.0f};
    controls.distance = 12.5f;

    auto key = make_ref<DirectionalLight>();
    key->position = {5.0f, 8.0f, 5.0f};
    key->target = {0.0f, 0.0f, 0.0f};
    key->intensity = 0.65f;
    scene.add(key);

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.50f, 0.49f, 0.46f};
    groundMat->roughness = 0.82f;
    groundMat->metalness = 0.0f;
    groundMat->envMapIntensity = 0.65f;
    groundMat->map = TextureFactory::makeCheckerboard(512, 512, 16, {0.55f, 0.55f, 0.52f}, {0.24f, 0.25f, 0.27f});
    groundMat->map->repeat = {5.0f, 4.0f};
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(16.0f, 12.0f, 16, 16), groundMat);
    scene.add(ground);

    auto sphereGeo = GeometryFactory::makeUVSphere(0.55f, 80, 40);
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 8; ++col) {
            auto mat = make_ref<MeshStandardMaterial>();
            float rough = glm::clamp(float(col) / 7.0f, 0.025f, 1.0f);
            float metal = glm::clamp(float(row) / 3.0f, 0.0f, 1.0f);
            mat->roughness = rough;
            mat->metalness = metal;
            mat->envMapIntensity = 1.25f;
            mat->color = glm::mix(glm::vec3(0.92f, 0.72f, 0.40f), glm::vec3(0.62f, 0.78f, 1.0f), metal);
            auto mesh = make_ref<Mesh>(sphereGeo, mat);
            mesh->position = {-5.6f + col * 1.6f, 0.68f, -2.6f + row * 1.55f};
            scene.add(mesh);
        }
    }

    auto clearcoatMat = make_ref<MeshPhysicalMaterial>();
    clearcoatMat->color = {0.95f, 0.06f, 0.04f};
    clearcoatMat->roughness = 0.36f;
    clearcoatMat->clearcoat = 1.0f;
    clearcoatMat->clearcoatRoughness = 0.045f;
    clearcoatMat->envMapIntensity = 1.4f;
    auto torus = make_ref<Mesh>(GeometryFactory::makeTorus(1.0f, 0.22f, 128, 32), clearcoatMat);
    torus->position = {-3.2f, 3.0f, -4.0f};
    torus->rotation.x = glm::radians(65.0f);
    scene.add(torus);

    auto mirrorMat = make_ref<MeshStandardMaterial>();
    mirrorMat->color = {0.86f, 0.90f, 1.0f};
    mirrorMat->roughness = 0.02f;
    mirrorMat->metalness = 1.0f;
    mirrorMat->envMapIntensity = 1.6f;
    auto mirror = make_ref<Mesh>(GeometryFactory::makeSphere(0.95f, 96, 48), mirrorMat);
    mirror->position = {3.4f, 1.1f, -4.0f};
    scene.add(mirror);

    auto grid = make_ref<GridHelper>(18.0f, 18);
    grid->position.y = 0.02f;
    scene.add(grid);

    GLRenderer renderer({1500, 880});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);
    renderer.setOutputColorSpace(ColorSpace::SRGB);

    std::cout << "Run with THREECPP_ENABLE_EXPERIMENTAL_PBR=1. Debug: THREECPP_DEBUG_PBR_IBL_DIFFUSE=1, THREECPP_DEBUG_PBR_IBL_SPECULAR=1, THREECPP_DEBUG_PBR_PMREM_LOD=1.\n";

    while (!window.shouldClose()) {
        window.poll();
        float t = static_cast<float>(window.time());
        scene.environment->environmentRotation = glm::mat3(glm::rotate(glm::mat4(1.0f), 0.25f * std::sin(t * 0.3f), glm::vec3(0, 1, 0)));
        torus->rotation.z = t * 0.25f;
        controls.update();
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
