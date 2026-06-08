#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/scene.h"
#include "core/camera.h"
#include "core/renderable.h"
#include "geometry/buffer-geometry.h"
#include "helpers/geometry-factory.h"
#include "light/light.h"
#include "texture/texture-factory.h"
#include "ibl/environment.h"

using namespace THREE;

int main() {
    Window window(1280, 720, "threecpp v1.0 - PBR equirect env fallback");

    RendererParameters params;
    params.width = window.framebufferSize().x;
    params.height = window.framebufferSize().y;
    GLRenderer renderer(params);
    Scene scene;
    scene.backgroundColor = {0.025f, 0.028f, 0.035f};

    auto envTexture = TextureFactory::makeCheckerboard(512, 256, 16, {0.95f, 0.92f, 0.86f}, {0.08f, 0.12f, 0.18f});
    envTexture->wrapS = TextureWrap::Repeat;
    auto pmrem = PMREMGenerator();
    scene.environment = pmrem.fromEquirectangular(envTexture);
    scene.environmentIntensity = 1.4f;

    auto camera = PerspectiveCamera(50.0f, window.aspect(), 0.1f, 100.0f);
    camera.position = {0.0f, 1.6f, 6.0f};

    auto ambient = make_ref<AmbientLight>();
    ambient->intensity = 0.05f;
    scene.add(ambient);

    auto dir = make_ref<DirectionalLight>();
    dir->position = {4.0f, 5.0f, 3.0f};
    dir->target = {0.0f, 0.0f, 0.0f};
    dir->intensity = 2.0f;
    scene.add(dir);

    auto sphereGeo = GeometryFactory::makeUVSphere(0.72f, 48, 24);
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 5; ++x) {
            auto mat = make_ref<MeshStandardMaterial>();
            mat->color = {0.95f, 0.72f, 0.44f};
            mat->metalness = float(x) / 4.0f;
            mat->roughness = 0.08f + 0.85f * float(y) / 2.0f;
            mat->envMapIntensity = 1.0f;
            auto mesh = make_ref<Mesh>(sphereGeo, mat);
            mesh->position = {float(x - 2) * 1.55f, float(y - 1) * 1.45f, 0.0f};
            scene.add(mesh);
        }
    }

    auto gridMat = make_ref<LineBasicMaterial>();
    gridMat->color = {0.22f, 0.25f, 0.3f};
    auto grid = make_ref<LineSegments>(BufferGeometry::makeLineGrid(8, 0.5f), gridMat);
    grid->position.y = -1.8f;
    scene.add(grid);

    while (!window.shouldClose()) {
        window.pollEvents();
        auto fb = window.framebufferSize();
        renderer.setSize(fb.x, fb.y);
        camera.aspect = window.aspect();
        camera.updateProjectionMatrix();
        float t = static_cast<float>(glfwGetTime());
        camera.position = {std::sin(t * 0.35f) * 6.0f, 2.0f, std::cos(t * 0.35f) * 6.0f};
        camera.quaternion = glm::quatLookAt(glm::normalize(glm::vec3(0, 0, 0) - camera.position), glm::vec3(0, 1, 0));
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
