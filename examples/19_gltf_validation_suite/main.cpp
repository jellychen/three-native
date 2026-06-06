#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "loader/AssimpLoader.hpp"
#include "validation/GltfValidationSuite.hpp"
#include "controls/OrbitControls.hpp"
#include "helpers/Helpers.hpp"
#include "helpers/GeometryFactory.hpp"
#include "light/Light.hpp"
#include "ibl/Environment.hpp"
#include "texture/TextureFactory.hpp"
#include <iostream>

using namespace threecpp;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: 19_gltf_validation_suite <asset.glb|asset.gltf|asset.fbx|asset.obj>\n";
        return 1;
    }

    Window window(1440, 900, "threecpp v3.3 glTF validation suite");
    Scene scene;
    scene.backgroundColor = {0.018f, 0.020f, 0.026f};
    scene.environment = PMREMGenerator({64, 6, true, 32, 32}).fromEquirectangular(TextureFactory::makeEquirectangularGradient(256, 128));
    scene.environmentIntensity = 1.15f;

    PerspectiveCamera camera(50.0f, window.aspect(), 0.05f, 1000.0f);
    camera.position = {4.5f, 2.7f, 6.0f};
    camera.lookAt({0.0f, 0.8f, 0.0f});
    OrbitControls controls(camera, window);
    controls.target = {0.0f, 0.8f, 0.0f};

    auto ambient = make_ref<AmbientLight>();
    ambient->intensity = 0.9f;
    scene.add(ambient);
    auto sun = make_ref<DirectionalLight>();
    sun->position = {5.0f, 8.0f, 6.0f};
    sun->intensity = 5.0f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    scene.add(sun);
    scene.add(make_ref<GridHelper>(10.0f, 10));
    scene.add(make_ref<AxesHelper>(1.4f));

    AssimpLoaderOptions opts;
    opts.loadTextures = true;
    opts.loadEmbeddedTextures = true;
    opts.loadSkinning = true;
    opts.loadMorphTargets = true;
    opts.loadAnimations = true;
    opts.generateTangents = true;
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

    GltfValidationSuite validator;
    const auto stats = validator.inspect(*loaded.root, loaded.animations);
    std::cout << "threecpp v3.3 validation: " << GltfValidationSuite::summary(stats) << "\n";

    loaded.root->traverse([](Object3D& object) {
        if (auto* mesh = dynamic_cast<Mesh*>(&object)) {
            mesh->castShadow = true;
            mesh->receiveShadow = true;
        }
    });
    scene.add(loaded.root);

    AnimationMixer mixer(loaded.root.get());
    if (!loaded.animations.empty()) {
        auto& action = mixer.clipAction(loaded.animations.front());
        action.loop = LoopMode::Repeat;
        std::cout << "Playing first clip: " << loaded.animations.front().name << "\n";
    }

    GLRenderer renderer({1440, 900});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);
    double last = window.time();
    while (!window.shouldClose()) {
        window.poll();
        double now = window.time();
        float dt = static_cast<float>(now - last);
        last = now;
        mixer.update(dt);
        controls.update();
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
