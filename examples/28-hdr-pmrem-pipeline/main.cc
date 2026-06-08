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

int main(int argc, char** argv) {
    Window window(1500, 900, "threecpp v3.6 HDR/RGBE + CPU PMREM pipeline");

    std::shared_ptr<Texture> hdr;
    if (argc > 1) {
        try {
            hdr = TextureLoader::loadRGBE(argv[1]);
            std::cout << "Loaded RGBE HDR: " << argv[1] << "\n";
        } catch (const std::exception& e) {
            std::cerr << "HDR load failed, falling back to procedural studio HDRI: " << e.what() << "\n";
        }
    }
    if (!hdr) hdr = TextureFactory::makeStudioHDRI(1024, 512);

    PMREMGenerator pmrem({64, 6, true, 32, 32});
    Scene scene;
    scene.backgroundColor = {0.015f, 0.016f, 0.020f};
    scene.environment = pmrem.fromEquirectangular(hdr);
    scene.environment->backgroundIntensity = 1.0f;
    scene.environmentIntensity = 1.35f;

    PerspectiveCamera camera(45.0f, 1500.0f / 900.0f, 0.05f, 300.0f);
    camera.position = {8.0f, 4.0f, 9.0f};
    camera.lookAt({0.0f, 1.0f, 0.0f});
    OrbitControls controls(camera, window);
    controls.target = {0.0f, 1.0f, 0.0f};

    auto amb = make_ref<AmbientLight>();
    amb->intensity = 0.15f;
    amb->color = {0.08f, 0.09f, 0.11f};
    scene.add(amb);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {5.0f, 8.0f, 5.0f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->intensity = 2.25f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.cameraLeft = -8.0f;
    sun->shadow.cameraRight = 8.0f;
    sun->shadow.cameraBottom = -8.0f;
    sun->shadow.cameraTop = 8.0f;
    sun->shadow.cameraFar = 35.0f;
    sun->shadow.mapSizeX = 2048;
    sun->shadow.mapSizeY = 2048;
    scene.add(sun);

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.52f, 0.51f, 0.48f};
    groundMat->roughness = 0.82f;
    groundMat->metalness = 0.0f;
    groundMat->map = TextureFactory::makeCheckerboard(512, 512, 24, {0.62f,0.60f,0.55f}, {0.33f,0.34f,0.36f});
    groundMat->map->repeat = {5.0f, 5.0f};
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(18.0f, 14.0f, 12, 12), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    auto sphereGeo = GeometryFactory::makeUVSphere(0.62f, 72, 36);
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 7; ++col) {
            float roughness = float(col) / 6.0f;
            float metalness = float(row) / 2.0f;
            auto mat = make_ref<MeshStandardMaterial>();
            mat->color = glm::mix(glm::vec3(1.0f, 0.52f, 0.22f), glm::vec3(0.55f, 0.75f, 1.0f), float(row) / 2.0f);
            mat->roughness = glm::clamp(roughness, 0.04f, 1.0f);
            mat->metalness = metalness;
            mat->envMapIntensity = 1.35f;
            auto mesh = make_ref<Mesh>(sphereGeo, mat);
            mesh->position = {-4.8f + col * 1.6f, 0.75f, -1.8f + row * 1.8f};
            mesh->castShadow = true;
            mesh->receiveShadow = true;
            scene.add(mesh);
        }
    }

    GLRenderer renderer({1500, 900});
    while (!window.shouldClose()) {
        window.poll();
        controls.update();
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
