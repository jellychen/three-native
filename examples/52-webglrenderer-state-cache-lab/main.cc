#include "renderer/cache/renderer-cache-diagnostics.h"
#include "renderer/cache/texture-unit-allocator.h"
#include "renderer/cache/web-gl-state-cache.h"
#include "renderer/cache/render-list-persistent-cache.h"
#include "renderer/program-cache.h"
#include "renderer/render-list.h"
#include "platform/window.h"
#include "core/scene.h"
#include "core/camera.h"
#include "helpers/geometry-factory.h"
#include "material/material.h"
#include "core/renderable.h"
#include "renderer/gl-renderer.h"
#include "controls/orbit-controls.h"
#include <iostream>

using namespace THREE;

int main() {
    Window window(1280, 720, "threecpp v5.9 WebGLRenderer-style state/cache lab");

    RendererParameters params;
    params.width = 1280;
    params.height = 720;
    params.sortObjects = true;
    GLRenderer renderer(params);
    renderer.setSize(1280, 720);

    Scene scene;
    scene.backgroundColor = {0.018f, 0.02f, 0.025f, 1.0f};

    PerspectiveCamera camera(60.0f, 1280.0f / 720.0f, 0.1f, 200.0f);
    camera.position = {0.0f, 5.5f, 12.0f};
    camera.lookAt({0.0f, 0.0f, 0.0f});
    OrbitControls controls(camera, window);
    controls.target = {0.0f, 0.0f, 0.0f};

    auto sharedGeometry = GeometryFactory::makeUVSphere(0.45f, 24, 16);
    std::vector<std::shared_ptr<MeshStandardMaterial>> materials;
    for (int i = 0; i < 8; ++i) {
        auto mat = make_ref<MeshStandardMaterial>();
        mat->color = glm::vec3(0.35f + 0.08f * float(i), 0.55f, 0.85f - 0.06f * float(i));
        mat->roughness = 0.2f + 0.09f * float(i);
        mat->metalness = (i % 3 == 0) ? 0.7f : 0.05f;
        materials.push_back(mat);
    }

    for (int z = 0; z < 10; ++z) {
        for (int x = 0; x < 10; ++x) {
            auto mesh = make_ref<Mesh>();
            mesh->geometry = sharedGeometry;
            mesh->material = materials[(x + z) % materials.size()];
            mesh->position = {float(x - 5) * 1.15f, 0.0f, float(z - 5) * 1.15f};
            scene.add(mesh);
        }
    }

    auto light = make_ref<DirectionalLight>();
    light->intensity = 4.0f;
    light->position = {5.0f, 8.0f, 5.0f};
    light->target = {0.0f, 0.0f, 0.0f};
    scene.add(light);

    double lastPrint = 0.0;
    int frame = 0;
    while (!window.shouldClose()) {
        window.poll();
        if (window.keyPressed(GLFW_KEY_ESCAPE)) break;
        controls.update();

        renderer.render(scene, camera);
        window.swapBuffers();

        const double now = window.time();
        if (now - lastPrint > 1.0) {
            lastPrint = now;
            RendererCacheDiagnostics diag = renderer.diagnostics();
            std::cout << diag.toString() << std::endl;
            std::cout << "frame calls=" << renderer.info.calls
                      << " triangles=" << renderer.info.triangles
                      << " programs=" << renderer.info.programs << std::endl;
        }
    }
    return 0;
}
