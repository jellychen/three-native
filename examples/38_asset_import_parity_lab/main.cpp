#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "loader/AssimpLoader.hpp"
#include "validation/ImportCompatibilityReport.hpp"
#include "validation/MaterialDump.hpp"
#include "controls/OrbitControls.hpp"
#include "helpers/Helpers.hpp"
#include "light/Light.hpp"
#include "ibl/Environment.hpp"
#include "texture/TextureFactory.hpp"
#include <iostream>
#include <iomanip>

using namespace threecpp;

static void normalizeImportedScene(Object3D& root) {
    root.traverse([](Object3D& object) {
        if (auto* mesh = dynamic_cast<Mesh*>(&object)) {
            mesh->castShadow = true;
            mesh->receiveShadow = true;
            if (mesh->material) mesh->material->markNeedsUpdate();
        }
    });
}

static void printImportReport(const ImportCompatibilityReport& report) {
    std::cout << "Import parity summary: " << report.summary() << "\n";
    if (!report.textureIssues.empty()) {
        std::cout << "Texture diagnostics:" << "\n";
        for (const auto& issue : report.textureIssues) {
            std::cout << "  material='" << issue.material << "' slot=" << issue.slot
                      << " source='" << issue.source << "' : " << issue.message << "\n";
        }
    }
}

static void printAnimations(const std::vector<AnimationClip>& clips) {
    std::cout << "Animations: " << clips.size() << "\n";
    for (std::size_t i = 0; i < clips.size(); ++i) {
        std::cout << "  [" << i << "] " << clips[i].name
                  << " duration=" << std::fixed << std::setprecision(3) << clips[i].duration
                  << " tracks=" << clips[i].tracks.size() << "\n";
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: 38_asset_import_parity_lab <asset.glb|asset.gltf|asset.fbx|asset.obj>\n";
        return 1;
    }

    Window window(1440, 900, "threecpp v4.5 asset import parity lab");

    Scene scene;
    scene.backgroundColor = {0.018f, 0.020f, 0.026f};
    scene.environment = PMREMGenerator({64, 6, true, 32, 32}).fromEquirectangular(
        TextureFactory::makeEquirectangularGradient(256, 128));
    scene.environmentIntensity = 1.3f;

    PerspectiveCamera camera(50.0f, window.aspect(), 0.02f, 3000.0f);
    camera.position = {4.5f, 2.8f, 7.0f};
    camera.lookAt({0.0f, 0.8f, 0.0f});

    OrbitControls controls(camera, window);
    controls.target = {0.0f, 0.8f, 0.0f};
    controls.setDistance(8.0f);

    auto ambient = make_ref<AmbientLight>();
    ambient->intensity = 0.55f;
    scene.add(ambient);

    auto hemi = make_ref<HemisphereLight>();
    hemi->intensity = 0.7f;
    hemi->skyColor = {0.50f, 0.62f, 0.85f};
    hemi->groundColor = {0.17f, 0.15f, 0.12f};
    scene.add(hemi);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {5.0f, 8.0f, 5.0f};
    sun->intensity = 4.0f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.mapSize = {2048, 2048};
    sun->shadow.bias = -0.00035f;
    sun->shadow.normalBias = 0.03f;
    sun->shadow.cameraLeft = -10.0f;
    sun->shadow.cameraRight = 10.0f;
    sun->shadow.cameraTop = 10.0f;
    sun->shadow.cameraBottom = -10.0f;
    sun->shadow.cameraNear = 0.1f;
    sun->shadow.cameraFar = 40.0f;
    scene.add(sun);

    auto fill = make_ref<PointLight>();
    fill->position = {-4.0f, 3.0f, 4.0f};
    fill->intensity = 18.0f;
    fill->distance = 14.0f;
    fill->decay = 2.0f;
    scene.add(fill);

    scene.add(make_ref<GridHelper>(14.0f, 14));
    scene.add(make_ref<AxesHelper>(1.5f));

    AssimpLoaderOptions opts;
    opts.loadTextures = true;
    opts.loadEmbeddedTextures = true;
    opts.loadSkinning = true;
    opts.loadMorphTargets = true;
    opts.loadAnimations = true;
    opts.generateTangents = true;
    opts.flipUVs = false;
    opts.validateData = true;
    AssimpLoader loader(opts);

    AssimpLoadResult loaded;
    try {
        loaded = loader.loadResult(argv[1]);
    } catch (const std::exception& e) {
        std::cerr << "Load failed: " << e.what() << "\n";
        return 2;
    }
    if (!loaded.root) {
        std::cerr << "Load failed: empty root\n";
        return 3;
    }

    normalizeImportedScene(*loaded.root);

    ImportCompatibilityInspector inspector;
    auto report = inspector.inspect(*loaded.root, loaded.animations, true);
    printImportReport(report);
    printAnimations(loaded.animations);
    dump_scene_materials(*loaded.root);

    scene.add(loaded.root);

    AnimationMixer mixer(loaded.root.get());
    bool animationEnabled = !loaded.animations.empty();
    std::size_t activeClip = 0;
    if (animationEnabled) {
        auto& action = mixer.clipAction(loaded.animations[activeClip]);
        action.loop = LoopMode::Repeat;
        std::cout << "Playing clip [0]: " << loaded.animations[0].name << "\n";
    }

    GLRenderer renderer({1440, 900});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);
    renderer.setOutputColorSpace(ColorSpace::SRGB);

    std::cout << "Controls: orbit/pan/wheel zoom. SPACE toggle animation, 1/2/3 switch clips, ESC exit.\n";
    double last = window.time();
    bool prevSpace = false, prev1 = false, prev2 = false, prev3 = false;

    auto switchClip = [&](std::size_t index) {
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
        bool k1 = window.keyPressed(GLFW_KEY_1), k2 = window.keyPressed(GLFW_KEY_2), k3 = window.keyPressed(GLFW_KEY_3);
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
