#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "controls/orbit-controls.h"
#include "helpers/geometry-factory.h"
#include "helpers/helpers.h"
#include "light/light.h"
#include <iostream>

using namespace THREE;

static std::shared_ptr<MeshStandardMaterial> makeMat(const glm::vec3& color, float metalness, float roughness) {
    auto m = make_ref<MeshStandardMaterial>();
    m->color = color;
    m->metalness = metalness;
    m->roughness = roughness;
    return m;
}

int main() {
    Window window(1280, 720, "threecpp v5.2 groups / multi-material / drawRange lab");
    GLRenderer renderer({1280, 720});
    renderer.setClearColor({0.025f, 0.027f, 0.032f, 1.0f});

    Scene scene;
    scene.backgroundColor = {0.025f, 0.027f, 0.032f};

    PerspectiveCamera camera(55.0f, 1280.0f / 720.0f, 0.1f, 300.0f);
    camera.position = {7.0f, 5.0f, 9.0f};
    camera.lookAt({0.0f, 0.8f, 0.0f});
    OrbitControls controls(window, camera);
    controls.target = {0.0f, 0.6f, 0.0f};
    controls.distance = 12.0f;

    scene.add(make_ref<GridHelper>(18.0f, 18, glm::vec3(0.20f)));
    scene.add(make_ref<AxesHelper>(2.5f));

    auto boxGeo = BufferGeometry::makeBox(2.0f);
    boxGeo->clearGroups();
    // Box has 6 faces, each face contributes 6 indices. Assign one material per face.
    for (int face = 0; face < 6; ++face) boxGeo->addGroup(face * 6, 6, face);

    std::vector<std::shared_ptr<Material>> faceMaterials;
    faceMaterials.push_back(makeMat({1.0f, 0.18f, 0.10f}, 0.05f, 0.45f));
    faceMaterials.push_back(makeMat({0.15f, 0.60f, 1.00f}, 0.15f, 0.30f));
    faceMaterials.push_back(makeMat({0.35f, 1.00f, 0.30f}, 0.00f, 0.65f));
    faceMaterials.push_back(makeMat({1.00f, 0.85f, 0.22f}, 0.25f, 0.22f));
    faceMaterials.push_back(makeMat({0.95f, 0.35f, 1.00f}, 0.00f, 0.55f));
    faceMaterials.push_back(makeMat({0.85f, 0.85f, 0.90f}, 0.75f, 0.18f));

    auto multiBox = make_ref<Mesh>(boxGeo, faceMaterials.front());
    multiBox->setMaterials(faceMaterials);
    multiBox->position = {-2.3f, 1.2f, 0.0f};
    multiBox->castShadow = true;
    multiBox->receiveShadow = true;
    scene.add(multiBox);

    auto clippedGeo = BufferGeometry::makeBox(2.0f);
    clippedGeo->clearGroups();
    for (int face = 0; face < 6; ++face) clippedGeo->addGroup(face * 6, 6, face % 3);
    // Draw only the middle 4 faces to verify drawRange intersection with groups.
    clippedGeo->setDrawRange(6, 24);
    std::vector<std::shared_ptr<Material>> clippedMaterials;
    clippedMaterials.push_back(makeMat({0.2f, 0.9f, 1.0f}, 0.0f, 0.35f));
    clippedMaterials.push_back(makeMat({1.0f, 0.45f, 0.18f}, 0.0f, 0.55f));
    clippedMaterials.push_back(makeMat({0.8f, 1.0f, 0.35f}, 0.3f, 0.25f));
    auto clippedBox = make_ref<Mesh>(clippedGeo, clippedMaterials.front());
    clippedBox->setMaterials(clippedMaterials);
    clippedBox->position = {2.3f, 1.2f, 0.0f};
    clippedBox->castShadow = true;
    clippedBox->receiveShadow = true;
    scene.add(clippedBox);

    auto sphereGeo = GeometryFactory::makeUVSphere(0.32f, 18, 10);
    sphereGeo->clearGroups();
    // Use first half / second half of indexed sphere as two groups to verify InstancedMesh + groups.
    int half = sphereGeo->indexCount() / 2;
    half -= half % 3;
    sphereGeo->addGroup(0, half, 0);
    sphereGeo->addGroup(half, sphereGeo->indexCount() - half, 1);
    std::vector<std::shared_ptr<Material>> instMats;
    instMats.push_back(makeMat({0.85f, 0.90f, 1.0f}, 0.65f, 0.28f));
    instMats.push_back(makeMat({1.0f, 0.55f, 0.16f}, 0.10f, 0.42f));
    auto inst = make_ref<InstancedMesh>(sphereGeo, instMats.front(), 96);
    inst->setMaterials(instMats);
    inst->castShadow = true;
    inst->receiveShadow = true;
    for (int i = 0; i < inst->count; ++i) {
        float x = float(i % 16) - 7.5f;
        float z = float(i / 16) - 2.5f;
        glm::mat4 m(1.0f);
        m = glm::translate(m, glm::vec3(x * 0.55f, 0.45f, z * 0.55f - 3.7f));
        inst->setMatrixAt(i, m);
        inst->setColorAt(i, {0.75f + 0.25f * std::sin(float(i)), 0.78f, 1.0f, 1.0f});
    }
    scene.add(inst);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {5.0f, 9.0f, 6.0f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->intensity = 2.6f;
    sun->castShadow = true;
    sun->shadow.mapSize = {2048, 2048};
    sun->shadow.cameraLeft = -10.0f;
    sun->shadow.cameraRight = 10.0f;
    sun->shadow.cameraTop = 10.0f;
    sun->shadow.cameraBottom = -10.0f;
    sun->shadow.cameraFar = 40.0f;
    scene.add(sun);

    auto hemi = make_ref<HemisphereLight>();
    hemi->skyColor = {0.48f, 0.58f, 0.82f};
    hemi->groundColor = {0.10f, 0.08f, 0.06f};
    hemi->intensity = 0.55f;
    scene.add(hemi);

    std::cout << "v5.2 groups/multi-material/drawRange lab\n"
              << "left box: 6 groups / 6 materials\n"
              << "right box: drawRange intersects groups\n"
              << "rear spheres: InstancedMesh with 2 groups / 2 materials\n";

    double lastPrint = 0.0;
    while (!window.shouldClose()) {
        window.poll();
        float t = static_cast<float>(window.time());
        multiBox->rotation.y = t * 0.65f;
        clippedBox->rotation.y = -t * 0.55f;
        sun->position = {std::cos(t * 0.35f) * 6.5f, 9.0f, std::sin(t * 0.35f) * 6.5f};
        controls.update();
        renderer.render(scene, camera);
        if (window.time() - lastPrint > 1.0) {
            lastPrint = window.time();
            std::cout << "calls=" << renderer.info.calls
                      << " instancedCalls=" << renderer.info.instancedCalls
                      << " instances=" << renderer.info.instances
                      << " triangles=" << renderer.info.triangles
                      << " programs=" << renderer.info.programs << std::endl;
        }
        window.swapBuffers();
        if (window.keyPressed(GLFW_KEY_ESCAPE)) window.requestClose();
    }
    return 0;
}
