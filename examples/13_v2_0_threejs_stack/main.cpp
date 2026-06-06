#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "core/Renderable.hpp"
#include "light/Light.hpp"
#include "helpers/GeometryFactory.hpp"
#include "geometry/FatLineGeometry.hpp"
#include "helpers/Helpers.hpp"
#include "texture/TextureFactory.hpp"
#include "ibl/Environment.hpp"
#include "animation/Animation.hpp"

using namespace threecpp;

int main() {
    Window window(1440, 900, "threecpp v2.0 three.js-like realtime stack");
    window.poll(); // Let macOS display the window before CPU-side setup work.

    Scene scene;
    scene.backgroundColor = {0.025f, 0.028f, 0.035f};

    auto hdr = TextureFactory::makeEquirectangularGradient(256, 128);
    PMREMGenerator pmrem({64, 6, true, 32, 32});
    scene.environment = pmrem.fromEquirectangular(hdr);
    scene.environmentIntensity = 1.0f;

    PerspectiveCamera camera(50.0f, 1440.0f / 900.0f, 0.1f, 300.0f);
    camera.position = {6.5f, 4.2f, 8.5f};
    camera.lookAt({0.0f, 0.8f, 0.0f});

    auto ambient = make_ref<AmbientLight>();
    ambient->color = {0.04f, 0.045f, 0.055f};
    ambient->intensity = 1.0f;
    scene.add(ambient);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {6.0f, 9.0f, 5.0f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->intensity = 5.5f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.mapSizeX = 2048;
    sun->shadow.mapSizeY = 2048;
    sun->shadow.bias = 0.0012f;
    sun->shadow.radius = 1.3f;
    scene.add(sun);

    auto point = make_ref<PointLight>();
    point->position = {-3.5f, 2.8f, 2.8f};
    point->color = {0.4f, 0.65f, 1.0f};
    point->intensity = 18.0f;
    point->distance = 12.0f;
    scene.add(point);

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.52f, 0.51f, 0.48f};
    groundMat->roughness = 0.84f;
    groundMat->metalness = 0.0f;
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(18.0f, 18.0f, 8, 8), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    auto sphere = GeometryFactory::makeUVSphere(0.65f, 64, 32);
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 6; ++col) {
            auto mat = make_ref<MeshStandardMaterial>();
            mat->color = glm::mix(glm::vec3(0.92f, 0.36f, 0.18f), glm::vec3(0.22f, 0.58f, 0.95f), float(col) / 5.0f);
            mat->roughness = glm::clamp(0.08f + float(row) * 0.33f, 0.04f, 1.0f);
            mat->metalness = float(col) / 5.0f;
            mat->envMapIntensity = 1.0f;
            auto mesh = make_ref<Mesh>(sphere, mat);
            mesh->position = {float(col - 2.5f) * 1.35f, 0.68f, float(row - 1) * 1.55f};
            mesh->castShadow = true;
            mesh->receiveShadow = true;
            scene.add(mesh);
        }
    }

    auto glassMat = make_ref<MeshPhysicalMaterial>();
    glassMat->color = {0.75f, 0.92f, 1.0f};
    glassMat->roughness = 0.04f;
    glassMat->metalness = 0.0f;
    glassMat->ior = 1.45f;
    glassMat->transmission = 0.82f;
    glassMat->thickness = 0.35f;
    glassMat->attenuationColor = {0.75f, 0.9f, 1.0f};
    glassMat->attenuationDistance = 2.5f;
    glassMat->clearcoat = 1.0f;
    glassMat->clearcoatRoughness = 0.08f;
    glassMat->transparent = true;
    auto glass = make_ref<Mesh>(GeometryFactory::makeUVSphere(0.9f, 64, 32), glassMat);
    glass->position = {0.0f, 1.05f, -3.0f};
    glass->castShadow = true;
    glass->receiveShadow = true;
    scene.add(glass);

    scene.add(make_ref<GridHelper>(18.0f, 18, glm::vec3(0.15f, 0.16f, 0.18f)));
    scene.add(make_ref<AxesHelper>(1.8f));

    auto fatGeo = FatLineGeometry::fromPolyline({{-4,0.05f,-3.6f},{-2,0.8f,-3.0f},{0,0.05f,-3.6f},{2,0.9f,-3.0f},{4,0.05f,-3.6f}});
    auto fatMat = make_ref<FatLineMaterial>();
    fatMat->color = {1.0f, 0.78f, 0.25f};
    fatMat->linewidth = 7.0f;
    auto curve = make_ref<FatLine>(fatGeo, fatMat);
    scene.add(curve);

    AnimationClip clip;
    clip.name = "glass-bob";
    clip.duration = 3.0f;
    clip.tracks.push_back({"Glass.quaternion", TrackValueType::Quat, Interpolation::Linear, {0.0f, 1.5f, 3.0f}, {0,0,0,1, 0,0.7071f,0,0.7071f, 0,0,0,1}});
    glass->name = "Glass";
    AnimationMixer mixer(&scene);
    auto& action = mixer.clipAction(clip);
    action.loop = LoopMode::Repeat;

    GLRenderer renderer({1440, 900});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);

    double last = window.time();
    while (!window.shouldClose()) {
        window.pollEvents();
        double now = window.time();
        float dt = static_cast<float>(now - last);
        last = now;
        mixer.update(dt);
        float t = static_cast<float>(now);
        camera.position = {std::sin(t * 0.25f) * 8.0f, 4.5f, std::cos(t * 0.25f) * 9.0f};
        camera.lookAt({0.0f, 0.8f, 0.0f});
        renderer.render(scene, camera);
        window.swapBuffers();
    }
}
