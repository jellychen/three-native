#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "helpers/geometry-factory.h"
#include "helpers/helpers.h"
#include "light/light.h"
#include "texture/texture-factory.h"
#include "ibl/environment.h"
#include "controls/orbit-controls.h"
#include <iostream>

using namespace THREE;

int main() {
    Window window(1500, 900, "threecpp v3.6 PMREM roughness/mip validation lab");
    auto hdr = TextureFactory::makeStudioHDRI(1024, 512);
    PMREMGenerator pmrem({64, 6, true, 32, 32});

    Scene scene;
    scene.environment = pmrem.fromEquirectangular(hdr);
    scene.environmentIntensity = 1.5f;
    scene.backgroundColor = {0.012f, 0.013f, 0.016f};

    std::cout << "PMREM generated: cubeSize=" << scene.environment->pmremCubeSize
              << " mipLevels=" << scene.environment->pmremMipLevels
              << " hasPMREM=" << scene.environment->hasPMREM << "\n";

    PerspectiveCamera camera(42.0f, 1500.0f / 900.0f, 0.05f, 200.0f);
    camera.position = {7.0f, 4.0f, 8.0f};
    camera.lookAt({0.0f, 1.0f, 0.0f});
    OrbitControls controls(camera, window);
    controls.target = {0.0f, 1.0f, 0.0f};

    auto hemi = make_ref<HemisphereLight>();
    hemi->color = {0.22f, 0.28f, 0.42f};
    hemi->groundColor = {0.06f, 0.05f, 0.04f};
    hemi->intensity = 0.45f;
    scene.add(hemi);

    auto light = make_ref<DirectionalLight>();
    light->position = {4.0f, 8.0f, 5.0f};
    light->target = {0, 0, 0};
    light->intensity = 1.8f;
    scene.add(light);

    auto sphereGeo = GeometryFactory::makeUVSphere(0.7f, 72, 36);
    for (int i = 0; i < 9; ++i) {
        float r = float(i) / 8.0f;
        auto mat = make_ref<MeshPhysicalMaterial>();
        mat->color = {0.95f, 0.86f, 0.72f};
        mat->metalness = 1.0f;
        mat->roughness = glm::clamp(r, 0.035f, 1.0f);
        mat->clearcoat = i >= 6 ? 0.5f : 0.0f;
        mat->clearcoatRoughness = r;
        mat->envMapIntensity = 1.6f;
        auto mesh = make_ref<Mesh>(sphereGeo, mat);
        mesh->position = {-6.0f + i * 1.5f, 1.0f, 0.0f};
        scene.add(mesh);
    }

    auto grid = make_ref<GridHelper>(18.0f, 18);
    scene.add(grid);

    GLRenderer renderer({1500, 900});
    while (!window.shouldClose()) {
        window.poll();
        controls.update();
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
