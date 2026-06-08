#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "light/light.h"
#include "helpers/geometry-factory.h"
#include "geometry/fat-line-geometry.h"
#include "texture/texture-factory.h"
#include "ibl/environment.h"
#include "controls/orbit-controls.h"

using namespace THREE;

static std::shared_ptr<MeshPhysicalMaterial> physicalMat(
    glm::vec3 color,
    float roughness,
    float metalness,
    float clearcoat,
    float transmission,
    float sheen,
    float ior,
    float thickness,
    glm::vec3 attenuationColor)
{
    auto mat = make_ref<MeshPhysicalMaterial>();
    mat->color = color;
    mat->roughness = roughness;
    mat->metalness = metalness;
    mat->ior = ior;
    mat->clearcoat = clearcoat;
    mat->clearcoatRoughness = 0.08f + roughness * 0.25f;
    mat->sheen = sheen;
    mat->sheenColor = glm::mix(glm::vec3(0.35f, 0.22f, 0.9f), color, 0.35f);
    mat->sheenRoughness = 0.55f;
    mat->transmission = transmission;
    mat->thickness = thickness;
    mat->attenuationColor = attenuationColor;
    mat->attenuationDistance = transmission > 0.0f ? 2.2f : std::numeric_limits<float>::infinity();
    mat->specularIntensity = 1.0f;
    mat->specularColor = glm::vec3(1.0f);
    mat->envMapIntensity = 1.65f;
    mat->transparent = transmission > 0.0f;
    mat->depthWrite = transmission <= 0.0f;
    return mat;
}

int main() {
    Window window(1500, 900, "threecpp v4.4 MeshPhysicalMaterial parity lab");
    Scene scene;
    scene.backgroundColor = {0.012f, 0.014f, 0.020f};

    PMREMGenerator pmrem({64, 6, true, 32, 32});
    scene.environment = pmrem.fromEquirectangular(TextureFactory::makeStudioHDRI(256, 128));
    scene.environmentIntensity = 1.35f;

    PerspectiveCamera camera(45.0f, 1500.0f / 900.0f, 0.05f, 300.0f);
    camera.position = {8.5f, 5.4f, 10.0f};
    camera.lookAt({0.0f, 1.0f, 0.0f});
    OrbitControls controls(camera, window);
    controls.target = {0.0f, 1.0f, 0.0f};

    auto ambient = make_ref<AmbientLight>();
    ambient->color = {0.05f, 0.055f, 0.07f};
    ambient->intensity = 0.35f;
    scene.add(ambient);

    auto hemi = make_ref<HemisphereLight>();
    hemi->skyColor = {0.46f, 0.58f, 0.95f};
    hemi->groundColor = {0.26f, 0.22f, 0.18f};
    hemi->intensity = 0.45f;
    scene.add(hemi);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {7.0f, 10.0f, 6.0f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->intensity = 4.0f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.mapSize = {2048, 2048};
    sun->shadow.cameraLeft = -11.0f;
    sun->shadow.cameraRight = 11.0f;
    sun->shadow.cameraBottom = -8.0f;
    sun->shadow.cameraTop = 8.0f;
    sun->shadow.cameraNear = 0.1f;
    sun->shadow.cameraFar = 35.0f;
    sun->shadow.bias = 0.0009f;
    sun->shadow.normalBias = 0.015f;
    sun->shadow.radius = 1.7f;
    scene.add(sun);

    auto point = make_ref<PointLight>();
    point->position = {-3.5f, 3.0f, 3.5f};
    point->color = {1.0f, 0.72f, 0.46f};
    point->intensity = 45.0f;
    point->distance = 13.0f;
    point->decay = 2.0f;
    point->castShadow = true;
    point->shadow.enabled = true;
    point->shadow.mapSize = {1024, 1024};
    point->shadow.cameraNear = 0.1f;
    point->shadow.cameraFar = 18.0f;
    point->shadow.bias = 0.02f;
    scene.add(point);

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.46f, 0.45f, 0.42f};
    groundMat->roughness = 0.78f;
    groundMat->metalness = 0.0f;
    groundMat->map = TextureFactory::makeCheckerboard(512, 512, 16, {0.56f,0.56f,0.53f}, {0.25f,0.26f,0.28f});
    groundMat->map->repeat = {6, 5};
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(18, 12, 16, 10), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    auto sphere = GeometryFactory::makeUVSphere(0.58f, 72, 36);
    struct Row { const char* label; float y; } rows[] = {
        {"transmission", 0.7f},
        {"clearcoat", 2.05f},
        {"sheen/specular", 3.40f},
    };

    for (int i = 0; i < 6; ++i) {
        float u = float(i) / 5.0f;
        float x = (float(i) - 2.5f) * 1.45f;
        auto glass = physicalMat({0.64f, 0.86f, 1.0f}, 0.02f + u * 0.38f, 0.0f, 0.10f, 0.95f - u * 0.2f, 0.0f, 1.15f + u * 0.45f, 0.35f + u * 1.1f, {0.55f + u * 0.4f, 0.85f, 1.0f});
        glass->dispersion = u * 0.28f;
        auto m0 = make_ref<Mesh>(sphere, glass); m0->position = {x, rows[0].y, 0.0f}; m0->castShadow = true; m0->receiveShadow = true; scene.add(m0);

        auto coat = physicalMat(glm::mix(glm::vec3(1.0f,0.36f,0.18f), glm::vec3(0.35f,0.55f,1.0f), u), 0.12f + u * 0.55f, u * 0.55f, 1.0f, 0.0f, 0.0f, 1.5f, 0.0f, {1,1,1});
        coat->clearcoatRoughness = 0.02f + u * 0.5f;
        auto m1 = make_ref<Mesh>(sphere, coat); m1->position = {x, rows[1].y, 0.0f}; m1->castShadow = true; m1->receiveShadow = true; scene.add(m1);

        auto cloth = physicalMat(glm::mix(glm::vec3(0.25f,0.18f,0.85f), glm::vec3(1.0f,0.75f,0.35f), u), 0.35f + u * 0.45f, 0.0f, 0.0f, 0.0f, 0.45f + u * 0.5f, 1.2f + u * 0.7f, 0.0f, {1,1,1});
        cloth->specularIntensity = 0.4f + u * 1.2f;
        cloth->specularColor = glm::mix(glm::vec3(1.0f), glm::vec3(1.0f, 0.55f, 0.35f), u);
        cloth->iridescence = u * 0.85f;
        cloth->anisotropy = (u - 0.5f) * 1.6f;
        cloth->anisotropyRotation = glm::radians(120.0f * u);
        auto m2 = make_ref<Mesh>(sphere, cloth); m2->position = {x, rows[2].y, 0.0f}; m2->castShadow = true; m2->receiveShadow = true; scene.add(m2);
    }

    auto lineMat = make_ref<FatLineMaterial>();
    lineMat->color = {0.9f, 0.95f, 1.0f};
    lineMat->linewidth = 3.0f;
    auto guide = make_ref<FatLine>(FatLineGeometry::fromPolyline({{-4.4f, 4.2f, 0.0f}, {4.4f, 4.2f, 0.0f}}), lineMat);
    scene.add(guide);

    GLRenderer renderer({1500, 900});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.05f);

    while (!window.shouldClose()) {
        window.poll();
        controls.update();
        float t = static_cast<float>(window.time());
        point->position = {-3.6f + std::sin(t * 0.9f) * 2.2f, 3.2f + std::sin(t * 1.3f) * 0.6f, 3.2f + std::cos(t * 0.7f) * 1.5f};
        sun->target = {std::sin(t * 0.35f) * 1.5f, 0.0f, std::cos(t * 0.28f) * 1.0f};
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
