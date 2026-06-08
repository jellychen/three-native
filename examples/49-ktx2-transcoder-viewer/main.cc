#include "common.h"
#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/scene.h"
#include "core/camera.h"
#include "controls/orbit-controls.h"
#include "helpers/geometry-factory.h"
#include "material/material.h"
#include "texture/texture-loader.h"
#include "texture/ktx2-transcoder.h"
#include "light/light.h"
#if THREECPP_ENABLE_ASSIMP
#include "loader/assimp-loader.h"
#include "validation/import-compatibility-report.h"
#endif
#include <iostream>

using namespace THREE;

static bool hasExt(const std::filesystem::path& p, const std::string& e) {
    auto x = p.extension().string();
    std::transform(x.begin(), x.end(), x.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return x == e;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: 49_ktx2_transcoder_viewer <texture.ktx2|model.glb|model.gltf|model.fbx|model.obj> [--no-render]\n";
        return 1;
    }
    std::filesystem::path asset = argv[1];
    bool render = true;
    for (int i = 2; i < argc; ++i) if (std::string(argv[i]) == "--no-render") render = false;

    if (hasExt(asset, ".ktx2")) {
        auto tex = TextureLoader::loadKTX2(asset, true);
        auto caps = CompressedTextureCapabilities::queryGL();
        auto tr = KTX2Transcoder::transcode(*tex, caps, {});
        std::cout << "[ktx2] " << asset << " size=" << tex->width << "x" << tex->height
                  << " levels=" << tex->compressedLevels.size()
                  << " supercompression=" << tex->supercompressionScheme
                  << " chosen=" << tr.gpuFormatName
                  << " compressedUpload=" << tr.usedCompressedUpload
                  << " rgbaFallback=" << tr.usedRGBAFallback
                  << " message=" << tr.message << "\n";
        if (!render) return 0;

        Window window(1280, 720, "threecpp v5.6 KTX2 transcoder viewer");
        GLRenderer renderer;
        renderer.initialize(window);
        renderer.setClearColor({0.025f, 0.025f, 0.03f}, 1.0f);

        Scene scene;
        scene.backgroundColor = {0.025f, 0.025f, 0.03f};
        auto mat = make_ref<MeshBasicMaterial>();
        mat->map = tex;
        mat->color = {1.0f, 1.0f, 1.0f};
        auto quad = make_ref<Mesh>(GeometryFactory::makePlane(3.0f), mat);
        quad->rotation.x = -glm::half_pi<float>();
        scene.add(quad);

        PerspectiveCamera camera(50.0f, 1280.0f / 720.0f, 0.05f, 100.0f);
        camera.position = {0.0f, 2.2f, 4.0f};
        camera.lookAt({0.0f, 0.0f, 0.0f});
        OrbitControls controls(camera, window);
        controls.target = {0.0f, 0.0f, 0.0f};
        while (!window.shouldClose()) {
            window.poll();
            controls.update();
            renderer.render(scene, camera);
            window.swapBuffers();
        }
        return 0;
    }

#if THREECPP_ENABLE_ASSIMP
    AssimpLoaderOptions opts;
    opts.loadTextures = true;
    opts.loadEmbeddedTextures = true;
    opts.preserveCompressedTextures = true;
    opts.inspectGltfExtensions = true;
    opts.loadAnimations = true;
    opts.loadSkinning = true;
    opts.loadMorphTargets = true;
    AssimpLoader loader(opts);
    auto result = loader.loadResult(asset);
    if (!result.root) throw std::runtime_error("model load failed");

    ImportCompatibilityInspector inspector;
    auto report = inspector.inspect(*result.root, result.animations, true);
    std::cout << "[model] " << asset << "\n";
    std::cout << "[import] " << report.summary() << "\n";
    for (const auto& issue : report.textureIssues) {
        std::cout << "[texture] slot=" << issue.slot << " source=" << issue.source << " message=" << issue.message << "\n";
    }
    std::cout << "[extensions] " << result.extensionReport.summary() << "\n";
    if (!render) return 0;

    Window window(1280, 720, "threecpp v5.6 KTX2/BasisU model viewer");
    GLRenderer renderer;
    renderer.initialize(window);
    renderer.setClearColor({0.035f, 0.04f, 0.05f}, 1.0f);

    Scene scene;
    scene.backgroundColor = {0.035f, 0.04f, 0.05f};
    scene.add(result.root);
    auto hemi = make_ref<HemisphereLight>();
    hemi->skyColor = {0.8f, 0.85f, 1.0f};
    hemi->groundColor = {0.12f, 0.11f, 0.10f};
    hemi->intensity = 1.5f;
    scene.add(hemi);
    auto sun = make_ref<DirectionalLight>();
    sun->position = {4.0f, 8.0f, 5.0f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->intensity = 3.0f;
    sun->castShadow = true;
    scene.add(sun);

    PerspectiveCamera camera(55.0f, 1280.0f / 720.0f, 0.05f, 500.0f);
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
#else
    std::cerr << "Model loading requires --enable_assimp=true. KTX2 standalone textures work without Assimp.\n";
    return 2;
#endif
}
