#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "core/Renderable.hpp"
#include "controls/OrbitControls.hpp"
#include "helpers/GeometryFactory.hpp"
#include "helpers/Helpers.hpp"
#include "light/Light.hpp"
#include "ibl/Environment.hpp"
#include "texture/TextureFactory.hpp"
#include <iostream>

using namespace threecpp;

static std::shared_ptr<Texture> makeMetallicRoughnessTexture(int w = 256, int h = 256) {
    auto t = std::make_shared<Texture>();
    t->name = "procedural metallicRoughnessTexture: G=roughness, B=metalness";
    t->width = w;
    t->height = h;
    t->channels = 4;
    t->format = TextureFormat::RGBA8;
    t->colorSpace = ColorSpace::LinearSRGB;
    t->wrapS = TextureWrap::Repeat;
    t->wrapT = TextureWrap::Repeat;
    t->pixels.resize(static_cast<std::size_t>(w * h * 4));
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float rough = float(x) / float(std::max(1, w - 1));
            float metal = float(y) / float(std::max(1, h - 1));
            std::size_t i = static_cast<std::size_t>((y * w + x) * 4);
            t->pixels[i + 0] = std::byte(255); // unused in glTF metallicRoughness
            t->pixels[i + 1] = std::byte(std::uint8_t(glm::clamp(rough, 0.0f, 1.0f) * 255.0f));
            t->pixels[i + 2] = std::byte(std::uint8_t(glm::clamp(metal, 0.0f, 1.0f) * 255.0f));
            t->pixels[i + 3] = std::byte(255);
        }
    }
    t->scalarChannel = TextureChannel::G;
    t->markNeedsUpdate();
    return t;
}

static std::shared_ptr<Texture> makeOcclusionUv2Texture(int w = 256, int h = 256) {
    auto t = std::make_shared<Texture>();
    t->name = "procedural aoMap on uv2";
    t->width = w;
    t->height = h;
    t->channels = 4;
    t->format = TextureFormat::RGBA8;
    t->colorSpace = ColorSpace::LinearSRGB;
    t->wrapS = TextureWrap::Repeat;
    t->wrapT = TextureWrap::Repeat;
    t->uvChannel = 1;
    t->pixels.resize(static_cast<std::size_t>(w * h * 4));
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float u = float(x) / float(std::max(1, w - 1));
            float v = float(y) / float(std::max(1, h - 1));
            float radial = glm::clamp(glm::length(glm::vec2(u - 0.5f, v - 0.5f)) * 2.0f, 0.0f, 1.0f);
            float ao = glm::mix(1.0f, 0.35f, 1.0f - radial);
            std::uint8_t q = std::uint8_t(ao * 255.0f);
            std::size_t i = static_cast<std::size_t>((y * w + x) * 4);
            t->pixels[i + 0] = std::byte(q);
            t->pixels[i + 1] = std::byte(q);
            t->pixels[i + 2] = std::byte(q);
            t->pixels[i + 3] = std::byte(255);
        }
    }
    t->markNeedsUpdate();
    return t;
}

static std::shared_ptr<Texture> makeNormalTexture(int w = 256, int h = 256) {
    auto t = std::make_shared<Texture>();
    t->name = "procedural normalMap";
    t->width = w;
    t->height = h;
    t->channels = 4;
    t->format = TextureFormat::RGBA8;
    t->colorSpace = ColorSpace::LinearSRGB;
    t->wrapS = TextureWrap::Repeat;
    t->wrapT = TextureWrap::Repeat;
    t->repeat = {2.0f, 2.0f};
    t->pixels.resize(static_cast<std::size_t>(w * h * 4));
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float u = float(x) / float(w);
            float v = float(y) / float(h);
            glm::vec3 n = glm::normalize(glm::vec3(std::sin(u * glm::two_pi<float>() * 10.0f) * 0.25f, std::cos(v * glm::two_pi<float>() * 10.0f) * 0.25f, 1.0f));
            std::size_t i = static_cast<std::size_t>((y * w + x) * 4);
            t->pixels[i + 0] = std::byte(std::uint8_t((n.x * 0.5f + 0.5f) * 255.0f));
            t->pixels[i + 1] = std::byte(std::uint8_t((n.y * 0.5f + 0.5f) * 255.0f));
            t->pixels[i + 2] = std::byte(std::uint8_t((n.z * 0.5f + 0.5f) * 255.0f));
            t->pixels[i + 3] = std::byte(255);
        }
    }
    t->markNeedsUpdate();
    return t;
}



static std::shared_ptr<Texture> makeBumpTexture(int w = 256, int h = 256) {
    auto t = std::make_shared<Texture>();
    t->name = "procedural bumpMap";
    t->width = w;
    t->height = h;
    t->channels = 4;
    t->format = TextureFormat::RGBA8;
    t->colorSpace = ColorSpace::LinearSRGB;
    t->wrapS = TextureWrap::Repeat;
    t->wrapT = TextureWrap::Repeat;
    t->repeat = {5.0f, 5.0f};
    t->pixels.resize(static_cast<std::size_t>(w * h * 4));
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float u = float(x) / float(w);
            float v = float(y) / float(h);
            float hgt = 0.5f + 0.5f * std::sin(u * glm::two_pi<float>() * 12.0f) * std::sin(v * glm::two_pi<float>() * 12.0f);
            std::uint8_t q = std::uint8_t(glm::clamp(hgt, 0.0f, 1.0f) * 255.0f);
            std::size_t i = static_cast<std::size_t>((y * w + x) * 4);
            t->pixels[i + 0] = std::byte(q);
            t->pixels[i + 1] = std::byte(q);
            t->pixels[i + 2] = std::byte(q);
            t->pixels[i + 3] = std::byte(255);
        }
    }
    t->markNeedsUpdate();
    return t;
}

int main() {
    Window window(1440, 900, "threecpp v4.1 MeshStandardMaterial / PBR parity lab");

    Scene scene;
    scene.backgroundColor = {0.016f, 0.018f, 0.024f};
    scene.environment = PMREMGenerator({64, 6, true, 32, 32}).fromEquirectangular(TextureFactory::makeEquirectangularGradient(256, 128));
    scene.environmentIntensity = 1.25f;

    PerspectiveCamera camera(48.0f, window.aspect(), 0.05f, 200.0f);
    camera.position = {5.0f, 3.3f, 7.0f};
    camera.lookAt({0.0f, 0.6f, 0.0f});
    OrbitControls controls(camera, window);
    controls.target = {0.0f, 0.6f, 0.0f};

    auto hemi = make_ref<HemisphereLight>();
    hemi->intensity = 0.8f;
    hemi->skyColor = {0.52f, 0.66f, 0.92f};
    hemi->groundColor = {0.20f, 0.16f, 0.12f};
    scene.add(hemi);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {4.5f, 7.0f, 4.0f};
    sun->intensity = 4.2f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.mapSize = {2048, 2048};
    sun->shadow.cameraLeft = -7.0f;
    sun->shadow.cameraRight = 7.0f;
    sun->shadow.cameraTop = 7.0f;
    sun->shadow.cameraBottom = -7.0f;
    scene.add(sun);

    scene.add(make_ref<GridHelper>(10.0f, 10));
    scene.add(make_ref<AxesHelper>(1.25f));

    auto sphere = GeometryFactory::makeUVSphere(0.62f, 64, 32);
    auto mr = makeMetallicRoughnessTexture();
    auto ao = makeOcclusionUv2Texture();
    auto normal = makeNormalTexture();

    for (int i = 0; i < 5; ++i) {
        auto mat = make_ref<MeshStandardMaterial>();
        mat->color = glm::mix(glm::vec3(1.0f, 0.58f, 0.28f), glm::vec3(0.60f, 0.78f, 1.0f), float(i) / 4.0f);
        mat->roughness = 1.0f;
        mat->metalness = 1.0f;
        mat->roughnessMap = mr;   // shader samples G
        mat->metalnessMap = mr;   // shader samples B
        mat->aoMap = ao;          // shader samples uv2
        mat->aoMapIntensity = 0.75f;
        mat->normalMap = normal;
        mat->normalScale = {0.25f + 0.2f * float(i), 0.25f + 0.2f * float(i)};
        mat->envMapIntensity = 1.2f;
        auto mesh = make_ref<Mesh>(sphere, mat);
        mesh->position = {float(i - 2) * 1.35f, 0.72f, 0.25f};
        mesh->castShadow = true;
        mesh->receiveShadow = true;
        scene.add(mesh);
    }

    auto alphaMat = make_ref<MeshStandardMaterial>();
    alphaMat->color = {0.55f, 0.88f, 1.0f};
    alphaMat->roughness = 0.08f;
    alphaMat->metalness = 0.0f;
    alphaMat->opacity = 0.45f;
    alphaMat->transparent = true;
    alphaMat->depthWrite = false;
    auto alphaSphere = make_ref<Mesh>(sphere, alphaMat);
    alphaSphere->position = {0.0f, 1.7f, -1.15f};
    scene.add(alphaSphere);


    auto bumpMat = make_ref<MeshStandardMaterial>();
    bumpMat->color = {0.82f, 0.72f, 0.55f};
    bumpMat->roughness = 0.55f;
    bumpMat->metalness = 0.0f;
    bumpMat->bumpMap = makeBumpTexture();
    bumpMat->bumpScale = 0.22f;
    bumpMat->envMapIntensity = 0.7f;
    auto bumpSphere = make_ref<Mesh>(sphere, bumpMat);
    bumpSphere->position = {-2.35f, 1.72f, -1.15f};
    bumpSphere->castShadow = true;
    scene.add(bumpSphere);

    auto flatMat = make_ref<MeshStandardMaterial>();
    flatMat->color = {1.0f, 0.72f, 0.32f};
    flatMat->roughness = 0.72f;
    flatMat->metalness = 0.15f;
    flatMat->flatShading = true;
    auto flatTorus = make_ref<Mesh>(GeometryFactory::makeTorus(0.48f, 0.18f, 12, 32), flatMat);
    flatTorus->position = {2.35f, 1.72f, -1.15f};
    flatTorus->castShadow = true;
    scene.add(flatTorus);

    auto wireMat = make_ref<MeshStandardMaterial>();
    wireMat->color = {0.25f, 0.95f, 0.95f};
    wireMat->roughness = 0.4f;
    wireMat->metalness = 0.2f;
    wireMat->wireframe = true;
    wireMat->toneMapped = true;
    auto wire = make_ref<Mesh>(GeometryFactory::makeUVSphere(0.55f, 24, 12), wireMat);
    wire->position = {0.0f, 2.65f, 0.15f};
    scene.add(wire);

    alphaMat->premultipliedAlpha = true;


    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.48f, 0.46f, 0.42f};
    groundMat->roughness = 0.82f;
    groundMat->metalness = 0.0f;
    groundMat->aoMap = ao;
    groundMat->aoMapIntensity = 0.65f;
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(9.0f, 6.0f, 8, 6), groundMat);
    ground->position = {0.0f, -0.01f, 0.0f};
    ground->receiveShadow = true;
    scene.add(ground);

    GLRenderer renderer({1440, 900});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);
    renderer.setOutputColorSpace(ColorSpace::SRGB);

    std::cout << "v4.1 MeshStandardMaterial parity lab: TBN normalMap, bumpMap, uv2 AO, flatShading, wireframe, premultiplied alpha, tone/output color.\n";

    while (!window.shouldClose()) {
        window.poll();
        if (window.keyPressed(GLFW_KEY_ESCAPE)) window.requestClose();
        double t = window.time();
        alphaSphere->rotation.y = float(t * 0.75);
        controls.update();
        renderer.setSize(window.width(), window.height());
        camera.aspect = window.aspect();
        camera.updateProjectionMatrix();
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
