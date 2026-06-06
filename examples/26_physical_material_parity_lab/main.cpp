#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "core/Renderable.hpp"
#include "light/Light.hpp"
#include "helpers/GeometryFactory.hpp"
#include "texture/TextureFactory.hpp"
#include "ibl/Environment.hpp"
#include "controls/OrbitControls.hpp"

using namespace threecpp;

int main() {
    Window window(1420, 860, "threecpp v3.5 MeshPhysicalMaterial parity lab");
    Scene scene;
    scene.backgroundColor = {0.012f, 0.014f, 0.019f};
    PMREMGenerator pmrem({64, 6, true, 32, 32});
    scene.environment = pmrem.fromEquirectangular(TextureFactory::makeStudioHDRI(256, 128));
    scene.environmentIntensity = 1.45f;

    PerspectiveCamera camera(45.0f, 1420.0f / 860.0f, 0.05f, 200.0f);
    camera.position = {7.0f, 4.5f, 8.5f};
    camera.lookAt({0,1,0});
    OrbitControls controls(camera, window);
    controls.target = {0,1,0};

    auto ambient = make_ref<AmbientLight>();
    ambient->intensity = 0.35f;
    ambient->color = {0.05f, 0.055f, 0.065f};
    scene.add(ambient);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {6,9,6};
    sun->target = {0,0,0};
    sun->intensity = 4.0f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.mapSizeX = 2048;
    sun->shadow.mapSizeY = 2048;
    sun->shadow.cameraLeft = -10;
    sun->shadow.cameraRight = 10;
    sun->shadow.cameraBottom = -10;
    sun->shadow.cameraTop = 10;
    sun->shadow.bias = 0.001f;
    sun->shadow.radius = 1.3f;
    scene.add(sun);

    auto spot = make_ref<SpotLight>();
    spot->position = {-4,5,4};
    spot->target = {0,0.6f,0};
    spot->intensity = 55.0f;
    spot->distance = 14.0f;
    spot->angle = glm::radians(24.0f);
    spot->penumbra = 0.5f;
    spot->castShadow = true;
    spot->shadow.enabled = true;
    spot->shadow.bias = 0.0009f;
    spot->shadow.radius = 1.0f;
    scene.add(spot);

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.40f,0.40f,0.38f};
    groundMat->roughness = 0.82f;
    groundMat->map = TextureFactory::makeCheckerboard(512, 512, 18, {0.52f,0.52f,0.49f}, {0.25f,0.26f,0.27f});
    groundMat->map->repeat = {5,5};
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(16, 12, 12, 8), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    auto sphere = GeometryFactory::makeUVSphere(0.62f, 64, 32);
    struct Config { const char* name; float clearcoat, sheen, transmission, iridescence, anisotropy, dispersion, metalness, roughness; glm::vec3 color; };
    std::vector<Config> configs = {
        {"clearcoat", 1.0f,0,0,0,0,0,0,0.13f,{1.0f,0.55f,0.35f}},
        {"sheen", 0,0.95f,0,0,0,0,0,0.55f,{0.55f,0.38f,1.0f}},
        {"transmission", 0.25f,0,0.88f,0,0,0.22f,0,0.04f,{0.72f,0.9f,1.0f}},
        {"iridescence", 0.6f,0,0,0.85f,0,0,0,0.24f,{0.75f,0.66f,1.0f}},
        {"anisotropy", 0.4f,0,0,0,0.75f,0,0.75f,0.28f,{1.0f,0.78f,0.45f}},
        {"combined", 1.0f,0.45f,0.35f,0.45f,0.35f,0.15f,0.0f,0.18f,{0.60f,1.0f,0.86f}}
    };
    for (int i = 0; i < int(configs.size()); ++i) {
        const auto& c = configs[std::size_t(i)];
        auto mat = make_ref<MeshPhysicalMaterial>();
        mat->name = c.name;
        mat->color = c.color;
        mat->roughness = c.roughness;
        mat->metalness = c.metalness;
        mat->ior = 1.45f;
        mat->clearcoat = c.clearcoat;
        mat->clearcoatRoughness = 0.06f;
        mat->sheen = c.sheen;
        mat->sheenColor = {1.0f, 0.25f, 0.75f};
        mat->sheenRoughness = 0.65f;
        mat->transmission = c.transmission;
        mat->thickness = c.transmission > 0.0f ? 0.55f : 0.0f;
        mat->attenuationColor = {0.70f,0.92f,1.0f};
        mat->attenuationDistance = 2.5f;
        mat->iridescence = c.iridescence;
        mat->iridescenceIOR = 1.35f;
        mat->anisotropy = c.anisotropy;
        mat->anisotropyRotation = glm::radians(35.0f * float(i));
        mat->dispersion = c.dispersion;
        mat->transparent = mat->transmission > 0.0f;
        mat->envMapIntensity = 1.6f;
        auto mesh = make_ref<Mesh>(sphere, mat);
        mesh->position = {float(i - 2.5f) * 1.45f, 0.7f, 0.0f};
        mesh->castShadow = true;
        mesh->receiveShadow = true;
        scene.add(mesh);
    }

    GLRenderer renderer({1420, 860});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.05f);
    while (!window.shouldClose()) {
        window.poll();
        controls.update();
        float t = static_cast<float>(window.time());
        spot->target = {std::sin(t) * 1.4f, 0.5f, std::cos(t * 0.8f) * 1.0f};
        renderer.render(scene, camera);
        window.swapBuffers();
    }
}
