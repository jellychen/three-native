#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "core/Renderable.hpp"
#include "light/Light.hpp"
#include "helpers/GeometryFactory.hpp"
#include "helpers/Helpers.hpp"
#include "texture/TextureFactory.hpp"
#include "ibl/Environment.hpp"
#include "animation/Animation.hpp"
#include "animation/Skeleton.hpp"

using namespace threecpp;

static std::shared_ptr<BufferGeometry> makeSkinnedTentacle(float radius = 0.28f, float height = 3.2f, int radialSegments = 24, int heightSegments = 24) {
    auto g = std::make_shared<BufferGeometry>();
    std::vector<float> p;
    std::vector<float> n;
    std::vector<float> uv;
    std::vector<float> skinIndex;
    std::vector<float> skinWeight;
    std::vector<uint32_t> idx;

    for (int y = 0; y <= heightSegments; ++y) {
        float v = float(y) / float(heightSegments);
        float py = v * height;
        float taper = glm::mix(1.0f, 0.65f, v);
        for (int x = 0; x <= radialSegments; ++x) {
            float u = float(x) / float(radialSegments);
            float a = u * glm::two_pi<float>();
            float sx = std::cos(a);
            float sz = std::sin(a);
            p.insert(p.end(), {sx * radius * taper, py, sz * radius * taper});
            n.insert(n.end(), {sx, 0.0f, sz});
            uv.insert(uv.end(), {u, v});

            // 3-bone vertical chain. Vertices blend across bone intervals just like
            // imported glTF skinIndex / skinWeight data would do.
            float boneSpace = v * 2.0f;
            int b0 = int(glm::floor(glm::clamp(boneSpace, 0.0f, 1.999f)));
            int b1 = glm::min(b0 + 1, 2);
            float w1 = glm::fract(boneSpace);
            float w0 = 1.0f - w1;
            skinIndex.insert(skinIndex.end(), {float(b0), float(b1), 0.0f, 0.0f});
            skinWeight.insert(skinWeight.end(), {w0, w1, 0.0f, 0.0f});
        }
    }

    for (int y = 0; y < heightSegments; ++y) {
        for (int x = 0; x < radialSegments; ++x) {
            uint32_t a = uint32_t(y * (radialSegments + 1) + x);
            uint32_t b = a + 1;
            uint32_t c = a + uint32_t(radialSegments + 1);
            uint32_t d = c + 1;
            idx.insert(idx.end(), {a, c, b, b, c, d});
        }
    }

    g->setAttribute("position", BufferAttribute::fromVector(p, 3, AttributeType::Float32));
    g->setAttribute("normal", BufferAttribute::fromVector(n, 3, AttributeType::Float32));
    g->setAttribute("uv", BufferAttribute::fromVector(uv, 2, AttributeType::Float32));
    g->setAttribute("skinIndex", BufferAttribute::fromVector(skinIndex, 4, AttributeType::Float32));
    g->setAttribute("skinWeight", BufferAttribute::fromVector(skinWeight, 4, AttributeType::Float32));
    g->setIndex(std::span<const std::uint32_t>(idx.data(), idx.size()));
    g->computeBoundingSphere();
    return g;
}

static AnimationClip makeBoneClip() {
    AnimationClip clip;
    clip.name = "procedural-three-bone-skinning";
    clip.duration = 4.0f;

    clip.tracks.push_back({
        "Bone_Mid.quaternion",
        TrackValueType::Quat,
        Interpolation::Linear,
        {0.0f, 1.0f, 2.0f, 3.0f, 4.0f},
        {
             0.0f, 0.0f, 0.0f, 1.0f,
             0.0f, 0.0f, 0.45f, 0.893f,
             0.0f, 0.0f, 0.0f, 1.0f,
             0.0f, 0.0f,-0.45f, 0.893f,
             0.0f, 0.0f, 0.0f, 1.0f
        }
    });

    clip.tracks.push_back({
        "Bone_Tip.quaternion",
        TrackValueType::Quat,
        Interpolation::Linear,
        {0.0f, 1.0f, 2.0f, 3.0f, 4.0f},
        {
             0.0f, 0.0f, 0.0f, 1.0f,
             0.28f, 0.0f, 0.0f, 0.960f,
             0.0f, 0.0f, 0.0f, 1.0f,
            -0.28f, 0.0f, 0.0f, 0.960f,
             0.0f, 0.0f, 0.0f, 1.0f
        }
    });
    return clip;
}

int main() {
    Window window(1280, 760, "threecpp skinning animation test - GPU skinning + PBR");
    window.poll(); // Let macOS display the window before CPU-side setup work.

    Scene scene;
    scene.backgroundColor = {0.028f, 0.030f, 0.038f};

    auto envTex = TextureFactory::makeEquirectangularGradient(256, 128);
    PMREMGenerator pmrem({64, 6, true, 32, 32});
    scene.environment = pmrem.fromEquirectangular(envTex);
    scene.environmentIntensity = 1.0f;

    PerspectiveCamera camera(50.0f, 1280.0f / 760.0f, 0.05f, 120.0f);
    camera.position = {5.0f, 3.6f, 7.0f};
    camera.lookAt({0.0f, 1.7f, 0.0f});

    auto ambient = make_ref<AmbientLight>();
    ambient->color = {0.06f, 0.065f, 0.075f};
    ambient->intensity = 1.1f;
    scene.add(ambient);

    auto key = make_ref<DirectionalLight>();
    key->position = {5.0f, 8.0f, 4.5f};
    key->target = {0.0f, 1.2f, 0.0f};
    key->intensity = 5.0f;
    key->castShadow = true;
    key->shadow.enabled = true;
    key->shadow.bias = 0.0012f;
    key->shadow.radius = 1.6f;
    key->shadow.mapSizeX = 2048;
    key->shadow.mapSizeY = 2048;
    scene.add(key);

    auto fill = make_ref<PointLight>();
    fill->position = {-3.5f, 2.5f, 3.2f};
    fill->color = {0.55f, 0.72f, 1.0f};
    fill->intensity = 12.0f;
    fill->distance = 12.0f;
    scene.add(fill);

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.48f, 0.48f, 0.45f};
    groundMat->roughness = 0.82f;
    groundMat->metalness = 0.0f;
    groundMat->map = TextureFactory::makeCheckerboard(512, 512, 16, {0.62f, 0.62f, 0.58f}, {0.38f, 0.39f, 0.38f});
    groundMat->map->repeat = {4.0f, 4.0f};
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(10.0f, 10.0f, 8, 8), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    auto skinMat = make_ref<MeshStandardMaterial>();
    skinMat->color = {0.98f, 0.58f, 0.32f};
    skinMat->roughness = 0.48f;
    skinMat->metalness = 0.05f;
    skinMat->normalScale = {0.75f, 0.75f};
    skinMat->envMapIntensity = 1.2f;
    auto skinned = make_ref<SkinnedMesh>(makeSkinnedTentacle(), skinMat);
    skinned->name = "Skinned_Tentacle";
    skinned->position = {0.0f, 0.02f, 0.0f};
    skinned->castShadow = true;
    skinned->receiveShadow = true;

    auto boneRoot = make_ref<Bone>();
    boneRoot->name = "Bone_Root";
    boneRoot->boneIndex = 0;
    boneRoot->position = {0.0f, 0.0f, 0.0f};

    auto boneMid = make_ref<Bone>();
    boneMid->name = "Bone_Mid";
    boneMid->boneIndex = 1;
    boneMid->position = {0.0f, 1.6f, 0.0f};

    auto boneTip = make_ref<Bone>();
    boneTip->name = "Bone_Tip";
    boneTip->boneIndex = 2;
    boneTip->position = {0.0f, 1.6f, 0.0f};

    boneMid->add(boneTip);
    boneRoot->add(boneMid);
    skinned->add(boneRoot);

    auto skeleton = std::make_shared<Skeleton>();
    skeleton->bones = {boneRoot.get(), boneMid.get(), boneTip.get()};
    scene.add(skinned);
    scene.updateMatrixWorld(true);
    skeleton->calculateInverses();
    skinned->bind(skeleton);

    // Visual helpers make the skin/bone relation easy to inspect.
    scene.add(make_ref<GridHelper>(10.0f, 10, glm::vec3(0.15f, 0.16f, 0.18f)));
    scene.add(make_ref<AxesHelper>(1.4f));
    auto skelHelper = make_ref<SkeletonHelper>(skinned.get());
    scene.add(skelHelper);

    AnimationClip clip = makeBoneClip();
    AnimationMixer mixer(skinned.get());
    auto& action = mixer.clipAction(clip);
    action.loop = LoopMode::Repeat;

    GLRenderer renderer({1280, 760});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);

    double last = window.time();
    while (!window.shouldClose()) {
        window.pollEvents();
        double now = window.time();
        float dt = static_cast<float>(now - last);
        last = now;

        mixer.update(dt);
        skelHelper->update();

        float t = static_cast<float>(now);
        camera.position = {std::sin(t * 0.22f) * 5.8f, 3.4f, std::cos(t * 0.22f) * 7.0f};
        camera.lookAt({0.0f, 1.65f, 0.0f});

        renderer.render(scene, camera);
        window.swapBuffers();
    }
}
