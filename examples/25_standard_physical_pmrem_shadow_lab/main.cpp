#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "core/Renderable.hpp"
#include "light/Light.hpp"
#include "helpers/GeometryFactory.hpp"
#include "helpers/Helpers.hpp"
#include "texture/TextureFactory.hpp"
#include "ibl/Environment.hpp"
#include "controls/OrbitControls.hpp"

using namespace threecpp;

static std::shared_ptr<MeshStandardMaterial> standardMat(glm::vec3 color, float roughness, float metalness) {
    auto m = make_ref<MeshStandardMaterial>();
    m->color = color;
    m->roughness = roughness;
    m->metalness = metalness;
    m->envMapIntensity = 1.25f;
    return m;
}

int main() {
    Window window(1500, 900, "threecpp v3.5 Standard/Physical + PMREM/HDR IBL + Shadow lab");

    Scene scene;
    scene.backgroundColor = {0.018f, 0.019f, 0.024f};
    auto hdr = TextureFactory::makeStudioHDRI(256, 128);
    PMREMGenerator pmrem({64, 6, true, 32, 32});
    scene.environment = pmrem.fromEquirectangular(hdr);
    scene.environment->skyColor = {0.35f, 0.46f, 0.72f};
    scene.environment->groundColor = {0.08f, 0.07f, 0.06f};
    scene.environment->specularColor = {1.0f, 0.96f, 0.88f};
    scene.environmentIntensity = 1.25f;

    PerspectiveCamera camera(45.0f, 1500.0f / 900.0f, 0.05f, 250.0f);
    camera.position = {8.0f, 4.8f, 9.0f};
    camera.lookAt({0.0f, 1.0f, 0.0f});
    OrbitControls controls(camera, window);
    controls.target = {0.0f, 1.0f, 0.0f};

    auto amb = make_ref<AmbientLight>();
    amb->color = {0.025f, 0.028f, 0.035f};
    amb->intensity = 1.0f;
    scene.add(amb);

    auto hemi = make_ref<HemisphereLight>();
    hemi->color = {0.20f, 0.26f, 0.42f};
    hemi->groundColor = {0.08f, 0.055f, 0.040f};
    hemi->intensity = 0.7f;
    scene.add(hemi);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {8.0f, 11.0f, 8.0f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->intensity = 5.0f;
    sun->color = {1.0f, 0.94f, 0.82f};
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.mapSizeX = 2048;
    sun->shadow.mapSizeY = 2048;
    sun->shadow.cameraLeft = -13.0f;
    sun->shadow.cameraRight = 13.0f;
    sun->shadow.cameraBottom = -13.0f;
    sun->shadow.cameraTop = 13.0f;
    sun->shadow.cameraFar = 70.0f;
    sun->shadow.bias = 0.0010f;
    sun->shadow.radius = 1.5f;
    scene.add(sun);

    auto spot = make_ref<SpotLight>();
    spot->position = {-4.0f, 6.5f, 4.5f};
    spot->target = {0.0f, 0.8f, 0.0f};
    spot->intensity = 65.0f;
    spot->distance = 18.0f;
    spot->angle = glm::radians(25.0f);
    spot->penumbra = 0.45f;
    spot->castShadow = true;
    spot->shadow.enabled = true;
    spot->shadow.mapSizeX = 1024;
    spot->shadow.mapSizeY = 1024;
    spot->shadow.bias = 0.00085f;
    spot->shadow.radius = 1.25f;
    scene.add(spot);

    auto warm = make_ref<PointLight>();
    warm->position = {4.5f, 2.3f, -4.0f};
    warm->color = {1.0f, 0.45f, 0.18f};
    warm->intensity = 65.0f;
    warm->distance = 16.0f;
    warm->decay = 2.0f;
    scene.add(warm);

    auto groundMat = standardMat({0.52f,0.51f,0.48f}, 0.78f, 0.0f);
    groundMat->map = TextureFactory::makeCheckerboard(512, 512, 20, {0.58f,0.57f,0.52f}, {0.33f,0.34f,0.34f});
    groundMat->map->repeat = {6.0f, 4.0f};
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(22.0f, 15.0f, 18, 12), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    auto sphere = GeometryFactory::makeUVSphere(0.55f, 64, 32);
    auto torus = GeometryFactory::makeTorus(0.45f, 0.16f, 64, 16);
    auto checker = TextureFactory::makeCheckerboard(512, 512, 10, {0.9f,0.35f,0.16f}, {0.14f,0.26f,0.95f});
    auto alpha = TextureFactory::makeAlphaCircle();

    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 6; ++x) {
            float fx = float(x) / 5.0f;
            float fy = float(y) / 2.0f;
            std::shared_ptr<Material> mat;
            if (y < 2) {
                auto sm = standardMat(glm::mix(glm::vec3(0.95f,0.35f,0.16f), glm::vec3(0.55f,0.75f,1.0f), fx), 0.08f + fx * 0.82f, fy);
                if (x == 2) sm->map = checker;
                if (x == 3) { sm->alphaMap = alpha; sm->alphaTest = 0.35f; sm->transparent = true; }
                mat = sm;
            } else {
                auto pm = make_ref<MeshPhysicalMaterial>();
                pm->color = glm::mix(glm::vec3(0.75f,0.9f,1.0f), glm::vec3(1.0f,0.68f,0.36f), fx);
                pm->roughness = 0.05f + fx * 0.55f;
                pm->metalness = x == 4 ? 0.45f : 0.0f;
                pm->ior = 1.25f + 0.06f * float(x);
                pm->clearcoat = x >= 1 ? 1.0f : 0.0f;
                pm->clearcoatRoughness = 0.03f + 0.08f * fx;
                pm->sheen = x == 2 ? 0.8f : 0.0f;
                pm->sheenColor = {1.0f, 0.25f, 0.72f};
                pm->transmission = x == 0 ? 0.85f : 0.0f;
                pm->thickness = x == 0 ? 0.45f : 0.0f;
                pm->attenuationColor = {0.72f, 0.9f, 1.0f};
                pm->attenuationDistance = 2.1f;
                pm->iridescence = x == 3 ? 0.75f : 0.0f;
                pm->anisotropy = x == 5 ? 0.55f : 0.0f;
                pm->dispersion = x == 0 ? 0.35f : 0.0f;
                pm->transparent = pm->transmission > 0.0f;
                pm->envMapIntensity = 1.5f;
                mat = pm;
            }
            auto mesh = make_ref<Mesh>((x + y) % 2 ? torus : sphere, mat);
            mesh->position = {float(x - 2.5f) * 1.42f, 0.75f, float(1 - y) * 1.85f};
            mesh->castShadow = true;
            mesh->receiveShadow = true;
            scene.add(mesh);
        }
    }

    auto gridMat = make_ref<LineBasicMaterial>();
    gridMat->color = {0.10f, 0.11f, 0.13f};
    auto grid = make_ref<LineSegments>(GeometryFactory::makeGrid(24, 22.0f), gridMat);
    grid->position.y = 0.01f;
    scene.add(grid);

    GLRenderer renderer({1500, 900});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);
    while (!window.shouldClose()) {
        window.poll();
        controls.update();
        float t = static_cast<float>(window.time());
        spot->target = {std::sin(t * 0.9f) * 1.5f, 0.5f, std::cos(t * 0.9f) * 1.2f};
        warm->position = {std::sin(t * 1.1f) * 5.0f, 2.2f, std::cos(t * 1.1f) * 4.0f};
        renderer.render(scene, camera);
        window.swapBuffers();
    }
}
