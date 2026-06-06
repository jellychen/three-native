#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "loader/AssimpLoader.hpp"
#include "light/Light.hpp"
#include "helpers/Helpers.hpp"
#include "helpers/GeometryFactory.hpp"
#include "ibl/Environment.hpp"
#include "texture/TextureFactory.hpp"
#include "animation/Animation.hpp"
#include "controls/OrbitControls.hpp"
#include <iostream>

using namespace threecpp;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: 17_assimp_multi_format_viewer <model.fbx|model.glb|model.gltf|model.obj>\n";
        std::cerr << "Supported: ";
        for (auto& e : AssimpLoader::supportedExtensions()) std::cerr << e << " ";
        std::cerr << "\n";
        return 1;
    }

    const std::filesystem::path modelPath = argv[1];
    Window window(1440, 900, "threecpp v2.3 Assimp multi-format viewer");

    Scene scene;
    scene.backgroundColor = {0.025f, 0.028f, 0.033f};

    PMREMGenerator pmrem({64, 6, true, 32, 32});
    scene.environment = pmrem.fromEquirectangular(TextureFactory::makeEquirectangularGradient(256, 128));
    scene.environmentIntensity = 1.0f;

    PerspectiveCamera camera(50.0f, window.aspect(), 0.05f, 1000.0f);
    camera.position = {3.5f, 2.2f, 5.0f};
    camera.lookAt({0.0f, 0.8f, 0.0f});

    OrbitControls controls(camera, window);
    controls.target = {0.0f, 0.8f, 0.0f};
    controls.minDistance = 0.2f;
    controls.maxDistance = 200.0f;
    controls.zoomSpeed = 0.15f;
    controls.rotateSpeed = 0.006f;

    auto ambient = make_ref<AmbientLight>();
    ambient->intensity = 0.8f;
    ambient->color = {0.08f, 0.085f, 0.095f};
    scene.add(ambient);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {5.0f, 8.0f, 4.0f};
    sun->target = {0.0f, 0.6f, 0.0f};
    sun->intensity = 5.5f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.mapSizeX = 2048;
    sun->shadow.mapSizeY = 2048;
    sun->shadow.bias = 0.0012f;
    scene.add(sun);

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.42f, 0.42f, 0.40f};
    groundMat->roughness = 0.88f;
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(18.0f, 18.0f, 10, 10), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);
    scene.add(make_ref<GridHelper>(18.0f, 18, glm::vec3(0.14f, 0.15f, 0.17f)));
    scene.add(make_ref<AxesHelper>(1.6f));

    AssimpLoaderOptions opts;
    opts.loadTextures = true;
    opts.loadEmbeddedTextures = true;
    opts.loadSkinning = true;
    opts.loadAnimations = true;
    opts.generateTangents = true;
    AssimpLoader loader(opts);

    AssimpLoadResult imported;
    try {
        imported = loader.loadResult(modelPath);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 2;
    }

    std::cout << "Loaded: " << modelPath << "\n";
    std::cout << "Format: " << imported.format << " meshes=" << imported.hasMeshes
              << " skins=" << imported.hasSkins << " animations=" << imported.animations.size() << "\n";

    auto root = imported.root;
    root->position = {0.0f, 0.0f, 0.0f};
    scene.add(root);

    root->traverse([](Object3D& o) {
        if (auto* mesh = dynamic_cast<Mesh*>(&o)) {
            mesh->castShadow = true;
            mesh->receiveShadow = true;
        }
    });

    AnimationMixer mixer(root.get());
    if (!imported.animations.empty()) {
        auto& action = mixer.clipAction(imported.animations.front());
        action.loop = LoopMode::Repeat;
        std::cout << "Playing animation: " << imported.animations.front().name << " duration=" << imported.animations.front().duration << "\n";
    }

    GLRenderer renderer({1440, 900});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);

    double last = window.time();
    while (!window.shouldClose()) {
        window.poll();
        const double now = window.time();
        const float dt = static_cast<float>(now - last);
        last = now;
        mixer.update(dt);
        root->updateMatrixWorld(true);

        controls.update();
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
