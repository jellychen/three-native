#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "controls/OrbitControls.hpp"
#include "helpers/GeometryFactory.hpp"
#include "helpers/Helpers.hpp"
#include "performance/PerformanceCache.hpp"
#include <iostream>

using namespace threecpp;

static glm::vec3 palette(int i) {
    float h = float(i % 37) / 37.0f;
    return {0.35f + 0.45f * std::sin(h * glm::two_pi<float>() + 0.0f) * 0.5f + 0.25f,
            0.35f + 0.45f * std::sin(h * glm::two_pi<float>() + 2.1f) * 0.5f + 0.25f,
            0.35f + 0.45f * std::sin(h * glm::two_pi<float>() + 4.2f) * 0.5f + 0.25f};
}

int main() {
    Window window(1280, 720, "threecpp v5.0 performance cache / large scene lab");
    GLRenderer renderer({1280, 720});
    renderer.setClearColor({0.018f, 0.020f, 0.026f, 1.0f});

    Scene scene;
    scene.backgroundColor = {0.018f, 0.020f, 0.026f};

    PerspectiveCamera camera(55.0f, 1280.0f / 720.0f, 0.1f, 500.0f);
    camera.position = {32.0f, 24.0f, 36.0f};
    camera.lookAt({0.0f, 0.0f, 0.0f});
    OrbitControls controls(window, camera);
    controls.target = {0.0f, 0.0f, 0.0f};
    controls.distance = 54.0f;

    scene.add(make_ref<GridHelper>(72.0f, 72, glm::vec3(0.18f)));
    scene.add(make_ref<AxesHelper>(4.0f));

    auto sharedGeo = GeometryFactory::makeUVSphere(0.42f, 16, 8);
    std::vector<std::shared_ptr<MeshStandardMaterial>> materials;
    for (int i = 0; i < 24; ++i) {
        auto m = make_ref<MeshStandardMaterial>();
        m->color = palette(i);
        m->roughness = 0.22f + 0.72f * float(i % 6) / 5.0f;
        m->metalness = float((i / 6) % 4) / 3.0f;
        materials.push_back(m);
    }

    constexpr int grid = 34;
    constexpr float spacing = 1.55f;
    int index = 0;
    for (int z = 0; z < grid; ++z) {
        for (int x = 0; x < grid; ++x) {
            auto mesh = make_ref<Mesh>(sharedGeo, materials[static_cast<std::size_t>((x + z * 7) % materials.size())]);
            mesh->position = {(x - grid * 0.5f) * spacing, 0.45f, (z - grid * 0.5f) * spacing};
            mesh->scale = glm::vec3(0.85f + 0.25f * std::sin(float(index) * 0.31f));
            mesh->castShadow = true;
            mesh->receiveShadow = true;
            mesh->renderOrder = float((x + z) % 3);
            scene.add(mesh);
            ++index;
        }
    }

    // Transparent and transmissive queues stress sorting and the new queue split.
    for (int i = 0; i < 48; ++i) {
        auto m = make_ref<MeshPhysicalMaterial>();
        m->color = {0.65f, 0.85f, 1.0f};
        m->roughness = 0.05f + 0.5f * float(i % 8) / 7.0f;
        m->metalness = 0.0f;
        m->transmission = 0.55f;
        m->thickness = 0.45f;
        m->transparent = true;
        m->opacity = 0.58f;
        m->depthWrite = false;
        auto mesh = make_ref<Mesh>(sharedGeo, m);
        float a = float(i) / 48.0f * glm::two_pi<float>();
        mesh->position = {std::cos(a) * 18.0f, 2.6f + std::sin(float(i) * 1.7f) * 1.1f, std::sin(a) * 18.0f};
        mesh->scale = glm::vec3(1.8f);
        scene.add(mesh);
    }

    auto sun = make_ref<DirectionalLight>();
    sun->position = {14.0f, 28.0f, 18.0f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->intensity = 2.2f;
    sun->castShadow = true;
    sun->shadow.mapSize = {2048, 2048};
    scene.add(sun);

    auto point = make_ref<PointLight>();
    point->position = {-10.0f, 8.0f, 10.0f};
    point->intensity = 12.0f;
    point->distance = 55.0f;
    point->decay = 2.0f;
    point->color = {0.45f, 0.65f, 1.0f};
    scene.add(point);

    auto hemi = make_ref<HemisphereLight>();
    hemi->skyColor = {0.38f, 0.48f, 0.78f};
    hemi->groundColor = {0.12f, 0.10f, 0.08f};
    hemi->intensity = 0.45f;
    scene.add(hemi);

    auto profile = analyze_large_scene(scene);
    std::cout << "v5.0 large scene profile: objects=" << profile.objectCount
              << " meshes=" << profile.meshCount
              << " uniqueGeometries=" << profile.geometryCount
              << " uniqueMaterials=" << profile.materialCount
              << " transparent=" << profile.transparentCount
              << " transmissive=" << profile.transmissiveCount << std::endl;

    RenderListCache listCache;
    double lastPrint = 0.0;
    while (!window.shouldClose()) {
        window.poll();
        float t = static_cast<float>(window.time());
        point->position = {std::cos(t * 0.7f) * 16.0f, 7.0f + std::sin(t * 1.1f) * 2.0f, std::sin(t * 0.7f) * 16.0f};
        sun->position = {std::cos(t * 0.18f) * 22.0f, 28.0f, std::sin(t * 0.18f) * 22.0f};
        controls.update();
        renderer.render(scene, camera);
        std::uint64_t sig = scene_cache_signature(scene);
        if (!listCache.find(sig)) {
            // The real renderer owns RenderList; this cache object is intentionally
            // demonstrative and gives tools a stable place to store scene signatures.
            RenderList empty;
            listCache.store(sig, empty);
        }
        if (window.time() - lastPrint > 2.0) {
            lastPrint = window.time();
            std::cout << "frame calls=" << renderer.info.calls
                      << " tris=" << renderer.info.triangles
                      << " programs=" << renderer.info.programs
                      << " renderListSignatures=" << listCache.size() << std::endl;
        }
        window.swapBuffers();
    }
    return 0;
}
