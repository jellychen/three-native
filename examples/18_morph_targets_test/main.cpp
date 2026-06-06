#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "core/Renderable.hpp"
#include "light/Light.hpp"
#include "helpers/GeometryFactory.hpp"
#include "helpers/Helpers.hpp"
#include "ibl/Environment.hpp"
#include "texture/TextureFactory.hpp"
#include "animation/Animation.hpp"

using namespace threecpp;

static std::shared_ptr<BufferGeometry> makeMorphPlane() {
    const int cols = 42;
    const int rows = 28;
    const float w = 4.2f;
    const float h = 2.8f;
    std::vector<float> pos;
    std::vector<float> normal;
    std::vector<float> uv;
    std::vector<std::uint32_t> idx;
    pos.reserve((cols + 1) * (rows + 1) * 3);
    normal.reserve((cols + 1) * (rows + 1) * 3);
    uv.reserve((cols + 1) * (rows + 1) * 2);

    std::vector<float> smile;
    std::vector<float> twist;
    std::vector<float> inflate;
    smile.reserve(pos.capacity());
    twist.reserve(pos.capacity());
    inflate.reserve(pos.capacity());

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

            float center = 1.0f - glm::clamp(glm::length(glm::vec2(px / (w * 0.5f), py / (h * 0.5f))), 0.0f, 1.0f);
            glm::vec3 s = p + glm::vec3(0.0f, -0.42f * std::cos((u - 0.5f) * glm::pi<float>()) + 0.18f * std::sin(v * glm::pi<float>()), 0.35f * center);
            smile.insert(smile.end(), {s.x, s.y, s.z});

            float a = (u - 0.5f) * 1.8f;
            glm::mat2 r(std::cos(a), -std::sin(a), std::sin(a), std::cos(a));
            glm::vec2 q = r * glm::vec2(px, py);
            glm::vec3 t(q.x, q.y, 0.55f * std::sin(u * glm::two_pi<float>()) * std::sin(v * glm::pi<float>()));
            twist.insert(twist.end(), {t.x, t.y, t.z});

            glm::vec3 inf = p + glm::vec3(0.0f, 0.0f, 0.9f * center);
            inflate.insert(inflate.end(), {inf.x, inf.y, inf.z});
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
    g->setIndex(idx);
    g->morphTargetsRelative = false;
    std::vector<BufferAttribute> targets;
    targets.push_back(BufferAttribute::fromVector(smile, 3, AttributeType::Float32));
    targets.push_back(BufferAttribute::fromVector(twist, 3, AttributeType::Float32));
    targets.push_back(BufferAttribute::fromVector(inflate, 3, AttributeType::Float32));
    g->setMorphAttribute("position", std::span<const BufferAttribute>(targets.data(), targets.size()));
    g->computeBoundingSphere();
    return g;
}

int main() {
    Window window(1280, 760, "threecpp v2.4 morph targets / blend shape test");

    Scene scene;
    scene.backgroundColor = {0.025f, 0.027f, 0.034f};
    auto envTex = TextureFactory::makeEquirectangularGradient(256, 128, {0.45f,0.58f,0.95f}, {1.0f,0.80f,0.50f}, {0.06f,0.06f,0.07f});
    PMREMGenerator pmrem({64, 6, true, 32, 32});
    scene.environment = pmrem.fromEquirectangular(envTex);
    scene.environmentIntensity = 1.25f;

    PerspectiveCamera camera(48.0f, 1280.0f / 760.0f, 0.05f, 120.0f);
    camera.position = {0.0f, 2.1f, 6.2f};
    camera.lookAt({0, 0, 0});

    auto ambient = make_ref<AmbientLight>();
    ambient->intensity = 0.8f;
    ambient->color = {0.045f, 0.047f, 0.055f};
    scene.add(ambient);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {4.0f, 6.0f, 5.0f};
    sun->target = {0, 0, 0};
    sun->intensity = 4.2f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    scene.add(sun);

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.42f,0.42f,0.40f};
    groundMat->roughness = 0.72f;
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(9.0f, 6.0f, 8, 6), groundMat);
    ground->position.y = -1.55f;
    ground->receiveShadow = true;
    scene.add(ground);

    auto mat = make_ref<MeshPhysicalMaterial>();
    mat->color = {0.84f, 0.54f, 0.25f};
    mat->roughness = 0.31f;
    mat->metalness = 0.08f;
    mat->clearcoat = 0.75f;
    mat->clearcoatRoughness = 0.12f;
    mat->envMapIntensity = 1.2f;

    auto mesh = make_ref<Mesh>(makeMorphPlane(), mat);
    mesh->name = "MorphPlane";
    mesh->morphTargetDictionary = {{"Smile", 0}, {"Twist", 1}, {"Inflate", 2}};
    mesh->morphTargetInfluences = {0.0f, 0.0f, 0.0f};
    mesh->castShadow = true;
    mesh->receiveShadow = true;
    scene.add(mesh);

    auto axes = make_ref<AxesHelper>(1.6f);
    axes->position = {-3.2f, -1.45f, 0.0f};
    scene.add(axes);

    GLRenderer renderer({1280, 760});
    renderer.setClearColor({0.02f, 0.02f, 0.025f, 1.0f});

    while (!window.shouldClose()) {
        window.poll();
        float t = static_cast<float>(window.time());
        mesh->setMorphTargetInfluence("Smile", 0.5f + 0.5f * std::sin(t * 1.3f));
        mesh->setMorphTargetInfluence("Twist", 0.5f + 0.5f * std::sin(t * 0.83f + 1.1f));
        mesh->setMorphTargetInfluence("Inflate", 0.5f + 0.5f * std::sin(t * 1.7f + 2.4f));
        mesh->rotation.y = std::sin(t * 0.35f) * 0.25f;
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
