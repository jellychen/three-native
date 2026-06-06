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
        const std::filesystem::path path(argv[1]);
        try {
            if (path.extension() == ".hdr" || path.extension() == ".HDR") {
                auto tex = TextureLoader::loadRGBE(path);
                std::cout << "Loaded RGBE environment: " << path << "\n";
                return tex;
            }
            if (path.extension() == ".pfm" || path.extension() == ".PFM") {
                auto tex = TextureLoader::loadPFMAsRGB16FPlaceholder(path);
                std::cout << "Loaded PFM environment: " << path << "\n";
                return tex;
            }
        } catch (const std::exception& e) {
            std::cerr << "Environment load failed, using procedural studio HDRI: " << e.what() << "\n";
        }
    }
    std::cout << "Using procedural studio HDRI. Pass a .hdr or .pfm path to test external env maps.\n";
    return TextureFactory::makeStudioHDRI(1024, 512);
}

int main(int argc, char** argv) {
    Window window(1500, 900, "threecpp v4.0 PMREM / HDR IBL lab");

    auto hdr = loadEnvironment(argc, argv);
    PMREMGenerator pmrem(PMREMOptions{64, 6, true, 32, 32});

    Scene scene;
    scene.backgroundColor = {0.015f, 0.016f, 0.020f};
    scene.environment = pmrem.fromEquirectangular(hdr);
    scene.environment->backgroundIntensity = 1.0f;
    scene.environment->intensity = 1.0f;
    scene.environmentIntensity = 1.35f;

    PerspectiveCamera camera(45.0f, 1500.0f / 900.0f, 0.05f, 300.0f);
    camera.position = {8.5f, 4.25f, 9.0f};
    camera.lookAt({0.0f, 1.0f, 0.0f});

    OrbitControls controls(camera, window);
    controls.target = {0.0f, 1.0f, 0.0f};

    auto hemi = make_ref<HemisphereLight>();
    hemi->skyColor = {0.35f, 0.46f, 0.72f};
    hemi->groundColor = {0.06f, 0.055f, 0.05f};
    hemi->intensity = 0.35f;
    scene.add(hemi);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {4.5f, 6.0f, 5.0f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->intensity = 1.55f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.mapSize = {2048, 2048};
    sun->shadow.cameraLeft = -9.0f;
    sun->shadow.cameraRight = 9.0f;
    sun->shadow.cameraTop = 7.0f;
    sun->shadow.cameraBottom = -7.0f;
    sun->shadow.cameraFar = 35.0f;
    scene.add(sun);

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.50f, 0.49f, 0.46f};
    groundMat->roughness = 0.82f;
    groundMat->metalness = 0.0f;
    groundMat->map = TextureFactory::makeCheckerboard(512, 512, 24, {0.58f,0.56f,0.52f}, {0.30f,0.31f,0.33f});
    groundMat->map->repeat = {6.0f, 5.0f};
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(18.0f, 14.0f, 16, 16), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    auto sphereGeo = GeometryFactory::makeUVSphere(0.58f, 72, 36);
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 8; ++col) {
            float roughness = glm::clamp(float(col) / 7.0f, 0.035f, 1.0f);
            float metalness = float(row) / 3.0f;
            auto mat = make_ref<MeshStandardMaterial>();
            mat->color = glm::mix(glm::vec3(1.0f, 0.55f, 0.22f), glm::vec3(0.55f, 0.75f, 1.0f), metalness);
            mat->roughness = roughness;
            mat->metalness = metalness;
            mat->envMapIntensity = 1.25f;
            auto mesh = make_ref<Mesh>(sphereGeo, mat);
            mesh->position = {-5.6f + col * 1.6f, 0.72f, -2.7f + row * 1.65f};
            mesh->castShadow = true;
            mesh->receiveShadow = true;
            scene.add(mesh);
        }
    }

    auto chromeMat = make_ref<MeshStandardMaterial>();
    chromeMat->color = {0.92f, 0.94f, 0.98f};
    chromeMat->roughness = 0.04f;
    chromeMat->metalness = 1.0f;
    chromeMat->envMapIntensity = 1.6f;
    auto chrome = make_ref<Mesh>(GeometryFactory::makeTorus(1.15f, 0.22f, 96, 24), chromeMat);
    chrome->position = {0.0f, 2.5f, -4.0f};
    chrome->rotation.x = glm::radians(70.0f);
    chrome->castShadow = true;
    scene.add(chrome);

    auto grid = make_ref<GridHelper>(18.0f, 18);
    grid->position.y = 0.01f;
    scene.add(grid);

    GLRenderer renderer({1500, 900});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);

    std::cout << "PMREM: cubeSize=" << scene.environment->pmremCubeSize
              << " mipLevels=" << scene.environment->pmremMipLevels
              << " hasPMREM=" << scene.environment->hasPMREM << "\n";
    std::cout << "Controls: LMB orbit, MMB/RMB pan, wheel dolly. ESC exits.\n";

    while (!window.shouldClose()) {
        window.poll();
        float t = static_cast<float>(window.time());
        scene.environment->environmentRotation = glm::mat3(glm::rotate(glm::mat4(1.0f), 0.18f * std::sin(t * 0.25f), glm::vec3(0, 1, 0)));
        chrome->rotation.z = t * 0.35f;
        controls.update();
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
