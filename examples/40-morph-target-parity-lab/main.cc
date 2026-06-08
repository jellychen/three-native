#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "light/light.h"
#include "helpers/geometry-factory.h"
#include "helpers/helpers.h"
#include "ibl/environment.h"
#include "texture/texture-factory.h"
#include "material/material.h"
#include "animation/animation.h"
#include "morph/morph-target-utils.h"

using namespace THREE;

static std::shared_ptr<BufferGeometry> makeFaceRigPlane(bool relative) {
    const int cols = 50;
    const int rows = 32;
    const float w = 4.6f;
    const float h = 3.0f;
    std::vector<float> pos, normal, uv, color;
    std::vector<std::uint32_t> idx;
    std::array<std::vector<float>, 6> targets;
    for (auto& t : targets) t.reserve((cols + 1) * (rows + 1) * 3);

    for (int y = 0; y <= rows; ++y) {
        float v = float(y) / float(rows);
        float py = (v - 0.5f) * h;
        for (int x = 0; x <= cols; ++x) {
            float u = float(x) / float(cols);
            float px = (u - 0.5f) * w;
            glm::vec3 p(px, py, 0.0f);
            pos.insert(pos.end(), {p.x, p.y, p.z});
            normal.insert(normal.end(), {0, 0, 1});
            uv.insert(uv.end(), {u, v});
            color.insert(color.end(), {u, v, 1.0f - u});

            float center = 1.0f - glm::clamp(glm::length(glm::vec2(px / (w * 0.5f), py / (h * 0.5f))), 0.0f, 1.0f);
            glm::vec3 smile = p + glm::vec3(0.0f, -0.35f * std::cos((u - 0.5f) * glm::pi<float>()), 0.28f * center);
            glm::vec3 blinkL = p + glm::vec3(px < 0.0f ? 0.0f : 0.0f, px < 0.0f ? -0.28f * std::exp(-std::abs(py) * 2.0f) : 0.0f, px < 0.0f ? 0.18f * center : 0.0f);
            glm::vec3 blinkR = p + glm::vec3(0.0f, px > 0.0f ? -0.28f * std::exp(-std::abs(py) * 2.0f) : 0.0f, px > 0.0f ? 0.18f * center : 0.0f);
            glm::vec3 puff = p + glm::vec3(0.0f, 0.0f, 0.65f * center);
            float a = (u - 0.5f) * 1.6f;
            glm::mat2 r(std::cos(a), -std::sin(a), std::sin(a), std::cos(a));
            glm::vec2 q = r * glm::vec2(px, py);
            glm::vec3 twist(q.x, q.y, 0.45f * std::sin(u * glm::two_pi<float>()) * std::sin(v * glm::pi<float>()));
            glm::vec3 jaw = p + glm::vec3(0.0f, v < 0.45f ? -0.65f * (0.45f - v) : 0.0f, v < 0.45f ? 0.25f * center : 0.0f);
            std::array<glm::vec3, 6> vals{smile, blinkL, blinkR, puff, twist, jaw};
            for (int i = 0; i < 6; ++i) {
                glm::vec3 out = relative ? (vals[static_cast<std::size_t>(i)] - p) : vals[static_cast<std::size_t>(i)];
                targets[static_cast<std::size_t>(i)].insert(targets[static_cast<std::size_t>(i)].end(), {out.x, out.y, out.z});
            }
        }
    }
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            std::uint32_t a = std::uint32_t(y * (cols + 1) + x);
            std::uint32_t b = a + 1;
            std::uint32_t c = a + std::uint32_t(cols + 1);
            std::uint32_t d = c + 1;
            idx.insert(idx.end(), {a, c, b, b, c, d});
        }
    }
    auto g = make_ref<BufferGeometry>();
    g->setAttribute("position", BufferAttribute::fromVector(pos, 3, AttributeType::Float32));
    g->setAttribute("normal", BufferAttribute::fromVector(normal, 3, AttributeType::Float32));
    g->setAttribute("uv", BufferAttribute::fromVector(uv, 2, AttributeType::Float32));
    g->setAttribute("color", BufferAttribute::fromVector(color, 3, AttributeType::Float32));
    g->setIndex(idx);
    g->morphTargetsRelative = relative;
    g->morphTextureFallbackPreferred = true;
    std::vector<BufferAttribute> morphPositions;
    for (auto& t : targets) morphPositions.push_back(BufferAttribute::fromVector(t, 3, AttributeType::Float32));
    g->setMorphAttribute("position", std::span<const BufferAttribute>(morphPositions.data(), morphPositions.size()));
    g->computeBoundingSphere();
    return g;
}

int main() {
    Window window(1280, 760, "threecpp v4.7 morph target parity lab");

    Scene scene;
    scene.backgroundColor = {0.025f, 0.027f, 0.034f};
    auto envTex = TextureFactory::makeEquirectangularGradient(256, 128, {0.38f,0.52f,0.90f}, {1.0f,0.82f,0.55f}, {0.04f,0.04f,0.05f});
    PMREMGenerator pmrem({64, 6, true, 32, 32});
    scene.environment = pmrem.fromEquirectangular(envTex);
    scene.environmentIntensity = 1.25f;

    PerspectiveCamera camera(48.0f, 1280.0f / 760.0f, 0.05f, 120.0f);
    camera.position = {0.0f, 2.0f, 7.2f};
    camera.lookAt({0, 0, 0});

    auto ambient = make_ref<AmbientLight>();
    ambient->intensity = 0.45f;
    scene.add(ambient);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {4.0f, 6.0f, 5.0f};
    sun->target = {0, 0, 0};
    sun->intensity = 4.0f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.mapSize = {2048, 2048};
    scene.add(sun);

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.42f,0.42f,0.40f};
    groundMat->roughness = 0.78f;
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(10.0f, 7.0f, 10, 8), groundMat);
    ground->position.y = -1.65f;
    ground->receiveShadow = true;
    scene.add(ground);

    auto mat = make_ref<MeshPhysicalMaterial>();
    mat->color = {0.92f, 0.58f, 0.26f};
    mat->roughness = 0.34f;
    mat->metalness = 0.04f;
    mat->clearcoat = 0.55f;
    mat->clearcoatRoughness = 0.16f;
    mat->envMapIntensity = 1.2f;

    auto gpuMesh = make_ref<Mesh>(makeFaceRigPlane(false), mat);
    gpuMesh->name = "MorphGpuFirstFour";
    gpuMesh->position.x = -2.55f;
    gpuMesh->morphTargetDictionary = {{"Smile",0},{"BlinkL",1},{"BlinkR",2},{"Puff",3},{"Twist",4},{"Jaw",5}};
    gpuMesh->morphTargetInfluences.assign(6, 0.0f);
    gpuMesh->castShadow = true;
    gpuMesh->receiveShadow = true;
    scene.add(gpuMesh);

    auto cpuMat = make_ref<MeshStandardMaterial>();
    cpuMat->color = {0.42f, 0.78f, 0.92f};
    cpuMat->roughness = 0.46f;
    cpuMat->metalness = 0.0f;
    auto cpuMesh = make_ref<Mesh>(makeFaceRigPlane(true), cpuMat);
    cpuMesh->name = "MorphCpuBakeSix";
    cpuMesh->position.x = 2.55f;
    cpuMesh->morphTargetDictionary = gpuMesh->morphTargetDictionary;
    cpuMesh->morphTargetInfluences = {0.25f, 0.45f, 0.45f, 0.2f, 0.35f, 0.55f};
    cpuMesh->morphTargetsUseCpuFallback = true;
    MorphTargetUtils::bakeActiveMorphsToBase(*cpuMesh);
    cpuMesh->castShadow = true;
    cpuMesh->receiveShadow = true;
    scene.add(cpuMesh);

    auto axes = make_ref<AxesHelper>(1.4f);
    axes->position = {-4.6f, -1.52f, 0.0f};
    scene.add(axes);

    GLRenderer renderer({1280, 760});
    renderer.setClearColor({0.02f, 0.02f, 0.025f, 1.0f});

    while (!window.shouldClose()) {
        window.poll();
        float t = static_cast<float>(window.time());
        gpuMesh->setMorphTargetInfluence("Smile", 0.5f + 0.5f * std::sin(t * 1.2f));
        gpuMesh->setMorphTargetInfluence("BlinkL", 0.5f + 0.5f * std::sin(t * 1.7f + 1.0f));
        gpuMesh->setMorphTargetInfluence("BlinkR", 0.5f + 0.5f * std::sin(t * 1.7f + 2.0f));
        gpuMesh->setMorphTargetInfluence("Puff", 0.5f + 0.5f * std::sin(t * 1.05f + 3.0f));
        gpuMesh->setMorphTargetInfluence("Twist", 0.5f + 0.5f * std::sin(t * 0.8f + 4.0f));
        gpuMesh->setMorphTargetInfluence("Jaw", 0.5f + 0.5f * std::sin(t * 1.35f + 5.0f));
        gpuMesh->rotation.y = std::sin(t * 0.35f) * 0.18f;
        cpuMesh->rotation.y = -std::sin(t * 0.35f) * 0.18f;
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
