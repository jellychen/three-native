#include "common.hpp"
#include "loader/AssimpLoader.hpp"
#include "validation/ImportCompatibilityReport.hpp"
#include "controls/OrbitControls.hpp"
#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "core/Scene.hpp"
#include "core/Camera.hpp"
#include "light/Light.hpp"
#include <iostream>

using namespace threecpp;

static void print_extension_report(const GltfExtensionsReport& r) {
    std::cout << "[gltf extensions] " << r.summary() << "\n";
    for (const auto& e : r.extensions) {
        std::cout << "  - " << e.name << ": " << to_string(e.status);
        if (!e.note.empty()) std::cout << " | " << e.note;
        std::cout << "\n";
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: 48_gltf_extensions_texture_transform <model.glb|model.gltf|model.fbx|model.obj> [--no-render]\n";
        return 1;
    }
    const std::filesystem::path modelPath = argv[1];
    bool render = true;
    for (int i = 2; i < argc; ++i) if (std::string(argv[i]) == "--no-render") render = false;

    AssimpLoaderOptions opts;
    opts.loadTextures = true;
    opts.loadEmbeddedTextures = true;
    opts.loadSkinning = true;
    opts.loadMorphTargets = true;
    opts.loadAnimations = true;
    opts.preserveCompressedTextures = true;
    opts.inspectGltfExtensions = true;

    AssimpLoader loader(opts);
    auto result = loader.loadResult(modelPath);
    if (!result.root) throw std::runtime_error("failed to load model");

    ImportCompatibilityInspector inspector;
    auto report = inspector.inspect(*result.root, result.animations, true);
    std::cout << "[asset] " << modelPath << "\n";
    std::cout << "[format] " << result.format << "\n";
    std::cout << "[import] " << report.summary() << "\n";
    for (const auto& issue : report.textureIssues) {
        std::cout << "[texture issue] material=" << issue.material
                  << " slot=" << issue.slot
                  << " source=" << issue.source
                  << " message=" << issue.message << "\n";
    }
    print_extension_report(result.extensionReport);

    if (!render) return 0;

    Window window(1280, 720, "threecpp v5.5 glTF extensions / texture transform");
    GLRenderer renderer;
    renderer.initialize(window);
    renderer.setClearColor({0.04f, 0.045f, 0.055f}, 1.0f);

    Scene scene;
    scene.backgroundColor = {0.04f, 0.045f, 0.055f};
    scene.add(result.root);

    auto hemi = make_ref<HemisphereLight>();
    hemi->skyColor = {0.7f, 0.78f, 0.95f};
    hemi->groundColor = {0.12f, 0.11f, 0.10f};
    hemi->intensity = 1.0f;
    scene.add(hemi);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {4.0f, 8.0f, 5.0f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->intensity = 3.0f;
    sun->castShadow = true;
    sun->shadow.mapSize = {2048, 2048};
    scene.add(sun);

    PerspectiveCamera camera(60.0f, 1280.0f / 720.0f, 0.05f, 500.0f);
    camera.position = {0.0f, 1.8f, 6.0f};
    camera.lookAt({0.0f, 1.0f, 0.0f});
    OrbitControls controls(camera, window);
    controls.target = {0.0f, 1.0f, 0.0f};

    while (!window.shouldClose()) {
        window.poll();
        controls.update();
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
