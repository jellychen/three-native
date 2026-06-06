#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "loader/AssimpLoader.hpp"
#include "validation/GltfValidationSuite.hpp"
#include "controls/OrbitControls.hpp"
#include "helpers/Helpers.hpp"
#include "light/Light.hpp"
#include "ibl/Environment.hpp"
#include "texture/TextureFactory.hpp"
#include <iostream>
#include <iomanip>

using namespace threecpp;

static void configureImportedScene(Object3D& root) {
    root.traverse([](Object3D& object) {
        if (auto* mesh = dynamic_cast<Mesh*>(&object)) {
            mesh->castShadow = true;
            mesh->receiveShadow = true;
            if (mesh->material) {
                mesh->material->needsUpdate = true;
            }
        }
    });
}

static void printClipList(const std::vector<AnimationClip>& clips) {
    if (clips.empty()) {
        std::cout << "No animation clips found.\n";
        return;
    }
    std::cout << "Animation clips:\n";
    for (size_t i = 0; i < clips.size(); ++i) {
        std::cout << "  [" << i << "] " << clips[i].name
                  << " duration=" << std::fixed << std::setprecision(3) << clips[i].duration
                  << " tracks=" << clips[i].tracks.size() << "\n";
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: 30_gltf_material_animation_validation <asset.glb|asset.gltf|asset.fbx|asset.obj>\n";
        return 1;
    }

    Window window(1440, 900, "threecpp v3.7 glTF material + animation validation");

    Scene scene;
    scene.backgroundColor = {0.016f, 0.018f, 0.024f};
    scene.environment = PMREMGenerator({64, 6, true, 32, 32}).fromEquirectangular(
        TextureFactory::makeEquirectangularGradient(256, 128));
    scene.environmentIntensity = 1.25f;

    PerspectiveCamera camera(50.0f, window.aspect(), 0.03f, 2000.0f);
    camera.position = {4.0f, 2.5f, 6.0f};
    camera.lookAt({0.0f, 0.75f, 0.0f});

    OrbitControls controls(camera, window);
    controls.target = {0.0f, 0.75f, 0.0f};

    auto ambient = make_ref<AmbientLight>();
    ambient->intensity = 0.75f;
    scene.add(ambient);

    auto hemi = make_ref<HemisphereLight>();
    hemi->intensity = 0.65f;
    hemi->skyColor = {0.50f, 0.62f, 0.82f};
    hemi->groundColor = {0.18f, 0.16f, 0.13f};
    scene.add(hemi);

    auto key = make_ref<DirectionalLight>();
    key->position = {5.0f, 8.0f, 6.0f};
    key->intensity = 4.0f;
    key->castShadow = true;
    key->shadow.enabled = true;
    key->shadow.mapSize = {2048, 2048};
    key->shadow.bias = -0.00035f;
    key->shadow.cameraLeft = -8.0f;
    key->shadow.cameraRight = 8.0f;
    key->shadow.cameraTop = 8.0f;
    key->shadow.cameraBottom = -8.0f;
    key->shadow.cameraNear = 0.1f;
    key->shadow.cameraFar = 35.0f;
    scene.add(key);

    auto fill = make_ref<PointLight>();
    fill->position = {-3.0f, 2.5f, 4.0f};
    fill->intensity = 16.0f;
    fill->distance = 12.0f;
    fill->decay = 2.0f;
    scene.add(fill);

    scene.add(make_ref<GridHelper>(12.0f, 12));
    scene.add(make_ref<AxesHelper>(1.5f));

    AssimpLoaderOptions opts;
    opts.loadTextures = true;
    opts.loadEmbeddedTextures = true;
    opts.loadSkinning = true;
    opts.loadMorphTargets = true;
    opts.loadAnimations = true;
    opts.generateTangents = true;
    opts.flipUVs = false;
    AssimpLoader loader(opts);

    AssimpLoadResult loaded;
    try {
        loaded = loader.loadResult(argv[1]);
    } catch (const std::exception& e) {
        std::cerr << "Load failed: " << e.what() << "\n";
        return 2;
    }
    if (!loaded.root) {
        std::cerr << "Load failed: no root object\n";
        return 3;
    }

    configureImportedScene(*loaded.root);

    GltfValidationSuite validator;
    GltfValidationOptions validationOptions;
    validationOptions.printPerObject = false;
    const auto stats = validator.inspect(*loaded.root, loaded.animations, validationOptions);
    std::cout << "Validation summary: " << GltfValidationSuite::summary(stats) << "\n";
    printClipList(loaded.animations);

    scene.add(loaded.root);

    AnimationMixer mixer(loaded.root.get());
    size_t activeClip = 0;
    bool animationEnabled = !loaded.animations.empty();
    if (animationEnabled) {
        auto& action = mixer.clipAction(loaded.animations[activeClip]);
        action.loop = LoopMode::Repeat;
        std::cout << "Playing clip [0]: " << loaded.animations[0].name << "\n";
    }

    GLRenderer renderer({1440, 900});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);

    std::cout << "Controls: mouse orbit/pan/wheel zoom, ESC close.\n";
    std::cout << "Keys: SPACE toggle animation, 1/2/3 switch clips when present.\n";

    double last = window.time();
    bool prevSpace = false;
    bool prev1 = false, prev2 = false, prev3 = false;

    auto switchClip = [&](size_t index) {
        if (index >= loaded.animations.size()) return;
        activeClip = index;
        mixer = AnimationMixer(loaded.root.get());
        auto& action = mixer.clipAction(loaded.animations[activeClip]);
        action.loop = LoopMode::Repeat;
        animationEnabled = true;
        std::cout << "Playing clip [" << activeClip << "]: " << loaded.animations[activeClip].name << "\n";
    };

    while (!window.shouldClose()) {
        window.poll();
        if (window.keyPressed(GLFW_KEY_ESCAPE)) window.requestClose();

        bool space = window.keyPressed(GLFW_KEY_SPACE);
        if (space && !prevSpace) {
            animationEnabled = !animationEnabled;
            std::cout << "Animation " << (animationEnabled ? "enabled" : "paused") << "\n";
        }
        prevSpace = space;

        bool k1 = window.keyPressed(GLFW_KEY_1);
        bool k2 = window.keyPressed(GLFW_KEY_2);
        bool k3 = window.keyPressed(GLFW_KEY_3);
        if (k1 && !prev1) switchClip(0);
        if (k2 && !prev2) switchClip(1);
        if (k3 && !prev3) switchClip(2);
        prev1 = k1; prev2 = k2; prev3 = k3;

        double now = window.time();
        float dt = static_cast<float>(now - last);
        last = now;

        if (animationEnabled) mixer.update(dt);
        controls.update();

        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
