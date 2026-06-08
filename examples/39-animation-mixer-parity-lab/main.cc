#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "light/light.h"
#include "helpers/geometry-factory.h"
#include "helpers/helpers.h"
#include "controls/orbit-controls.h"
#include "animation/animation.h"
#include "ibl/environment.h"
#include "texture/texture-factory.h"

using namespace THREE;

static AnimationClip makeMoveClip() {
    AnimationClip clip;
    clip.name = "MoveLoop";
    clip.duration = 3.0f;
    clip.tracks.push_back({
        "AnimatedBox.position", TrackValueType::Vec3, Interpolation::Smooth,
        {0.0f, 1.5f, 3.0f},
        {-2.0f, 0.55f, 0.0f,  2.0f, 0.55f, 0.0f,  -2.0f, 0.55f, 0.0f}
    });
    return clip;
}

static AnimationClip makeRotateClip() {
    AnimationClip clip;
    clip.name = "RotatePingPong";
    clip.duration = 2.0f;
    clip.tracks.push_back({
        "AnimatedBox.quaternion", TrackValueType::Quat, Interpolation::Linear,
        {0.0f, 1.0f, 2.0f},
        {
            0.0f, 0.0f, 0.0f, 1.0f,
            0.0f, 0.7071f, 0.0f, 0.7071f,
            0.0f, 0.0f, 0.0f, 1.0f
        }
    });
    return clip;
}

static AnimationClip makeScaleClip() {
    AnimationClip clip;
    clip.name = "ScaleOnce";
    clip.duration = 1.2f;
    clip.tracks.push_back({
        "AnimatedBox.scale", TrackValueType::Vec3, Interpolation::Smooth,
        {0.0f, 0.6f, 1.2f},
        {1.0f,1.0f,1.0f,  1.8f,0.55f,1.8f,  1.0f,1.0f,1.0f}
    });
    return clip;
}

int main() {
    Window window(1280, 760, "threecpp v4.6 AnimationMixer parity lab");

    Scene scene;
    scene.backgroundColor = {0.026f, 0.028f, 0.034f};
    auto envTex = TextureFactory::makeEquirectangularGradient(256, 128);
    PMREMGenerator pmrem({64, 6, true, 32, 32});
    scene.environment = pmrem.fromEquirectangular(envTex);
    scene.environmentIntensity = 0.9f;

    PerspectiveCamera camera(55.0f, 1280.0f / 760.0f, 0.05f, 150.0f);
    camera.position = {5.0f, 3.2f, 6.5f};
    camera.lookAt({0.0f, 0.8f, 0.0f});
    OrbitControls controls(window, camera);
    controls.target = {0.0f, 0.6f, 0.0f};

    auto hemi = make_ref<HemisphereLight>();
    hemi->skyColor = {0.55f, 0.68f, 1.0f};
    hemi->groundColor = {0.18f, 0.16f, 0.12f};
    hemi->intensity = 1.0f;
    scene.add(hemi);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {4.0f, 7.0f, 4.0f};
    sun->target = {0.0f, 0.4f, 0.0f};
    sun->intensity = 4.5f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.mapSize = {2048, 2048};
    scene.add(sun);

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.45f, 0.46f, 0.43f};
    groundMat->roughness = 0.82f;
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(9.0f, 9.0f, 6, 6), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    auto boxMat = make_ref<MeshStandardMaterial>();
    boxMat->color = {0.9f, 0.42f, 0.22f};
    boxMat->roughness = 0.4f;
    boxMat->metalness = 0.05f;
    boxMat->envMapIntensity = 1.1f;
    auto box = make_ref<Mesh>(BufferGeometry::makeBox(0.9f), boxMat);
    box->name = "AnimatedBox";
    box->position = {-2.0f, 0.55f, 0.0f};
    box->castShadow = true;
    box->receiveShadow = true;
    scene.add(box);

    scene.add(make_ref<GridHelper>(9.0f, 9, glm::vec3(0.14f, 0.15f, 0.17f)));
    scene.add(make_ref<AxesHelper>(1.2f));

    AnimationClip moveClip = makeMoveClip();
    AnimationClip rotateClip = makeRotateClip();
    AnimationClip scaleClip = makeScaleClip();
    AnimationMixer mixer(&scene);
    auto& move = mixer.clipAction(moveClip).setLoop(LoopMode::Repeat).play();
    auto& rotate = mixer.clipAction(rotateClip).setLoop(LoopMode::PingPong).setEffectiveWeight(0.65f).play();
    auto& scale = mixer.clipAction(scaleClip).setLoop(LoopMode::Once).setEffectiveWeight(0.0f);
    scale.clampWhenFinished = false;

    GLRenderer renderer({1280, 760});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);

    double last = window.time();
    double pulseTimer = 0.0;
    while (!window.shouldClose()) {
        window.pollEvents();
        double now = window.time();
        float dt = static_cast<float>(now - last);
        last = now;
        pulseTimer += dt;

        if (pulseTimer > 4.0) {
            pulseTimer = 0.0;
            scale.reset().setEffectiveWeight(1.0f).fadeIn(0.2f).play();
            move.crossFadeFrom(rotate, 0.45f, false);
            rotate.fadeIn(0.45f);
        }

        controls.update();
        mixer.update(dt);
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
