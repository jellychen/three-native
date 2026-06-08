#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "light/light.h"
#include "helpers/geometry-factory.h"
#include "helpers/helpers.h"
#include "texture/texture-factory.h"
#include "ibl/environment.h"

using namespace THREE;

static std::shared_ptr<Texture> makeStripeTexture(glm::vec3 a, glm::vec3 b, int width = 512, int height = 512) {
    auto t = std::make_shared<Texture>();
    t->width = width;
    t->height = height;
    t->channels = 4;
    t->format = TextureFormat::SRGB8Alpha8;
    t->colorSpace = ColorSpace::SRGB;
    t->wrapS = TextureWrap::Repeat;
    t->wrapT = TextureWrap::Repeat;
    t->pixels.resize(static_cast<std::size_t>(width * height * 4));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            bool stripe = ((x / 32) + (y / 64)) & 1;
            glm::vec3 c = stripe ? a : b;
            std::size_t i = static_cast<std::size_t>((y * width + x) * 4);
            t->pixels[i + 0] = std::byte(std::uint8_t(glm::clamp(c.r, 0.0f, 1.0f) * 255.0f));
            t->pixels[i + 1] = std::byte(std::uint8_t(glm::clamp(c.g, 0.0f, 1.0f) * 255.0f));
            t->pixels[i + 2] = std::byte(std::uint8_t(glm::clamp(c.b, 0.0f, 1.0f) * 255.0f));
            t->pixels[i + 3] = std::byte(255);
        }
    }
    t->repeat = {2.0f, 2.0f};
    t->markNeedsUpdate();
    return t;
}

static std::shared_ptr<Texture> makeNormalRipple(int width = 256, int height = 256) {
    auto t = std::make_shared<Texture>();
    t->width = width;
    t->height = height;
    t->channels = 4;
    t->format = TextureFormat::RGBA8;
    t->colorSpace = ColorSpace::LinearSRGB;
    t->pixels.resize(static_cast<std::size_t>(width * height * 4));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float u = float(x) / float(width);
            float v = float(y) / float(height);
            float sx = std::sin(u * glm::two_pi<float>() * 8.0f) * 0.22f;
            float sy = std::cos(v * glm::two_pi<float>() * 8.0f) * 0.22f;
            glm::vec3 n = glm::normalize(glm::vec3(sx, sy, 1.0f));
            std::size_t i = static_cast<std::size_t>((y * width + x) * 4);
            t->pixels[i + 0] = std::byte(std::uint8_t((n.x * 0.5f + 0.5f) * 255.0f));
            t->pixels[i + 1] = std::byte(std::uint8_t((n.y * 0.5f + 0.5f) * 255.0f));
            t->pixels[i + 2] = std::byte(std::uint8_t((n.z * 0.5f + 0.5f) * 255.0f));
            t->pixels[i + 3] = std::byte(255);
        }
    }
    t->wrapS = TextureWrap::Repeat;
    t->wrapT = TextureWrap::Repeat;
    t->repeat = {3.0f, 3.0f};
    t->markNeedsUpdate();
    return t;
}

static std::shared_ptr<Texture> makeScalarGradient(float left, float right, int width = 256, int height = 256) {
    auto t = std::make_shared<Texture>();
    t->width = width;
    t->height = height;
    t->channels = 4;
    t->format = TextureFormat::RGBA8;
    t->colorSpace = ColorSpace::LinearSRGB;
    t->pixels.resize(static_cast<std::size_t>(width * height * 4));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float v = glm::mix(left, right, float(x) / float(std::max(1, width - 1)));
            std::uint8_t q = std::uint8_t(glm::clamp(v, 0.0f, 1.0f) * 255.0f);
            std::size_t i = static_cast<std::size_t>((y * width + x) * 4);
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
    Window window(1480, 900, "threecpp PBR material gallery - Standard + Physical");
    window.poll(); // Let macOS display the window before CPU-side setup work.

    Scene scene;
    scene.backgroundColor = {0.022f, 0.024f, 0.030f};

    auto envTex = TextureFactory::makeEquirectangularGradient(256, 128, {0.46f, 0.62f, 0.95f}, {1.0f, 0.76f, 0.48f}, {0.12f, 0.10f, 0.09f});
    PMREMGenerator pmrem({64, 6, true, 32, 32});
    scene.environment = pmrem.fromEquirectangular(envTex);
    scene.environmentIntensity = 1.1f;

    PerspectiveCamera camera(48.0f, 1480.0f / 900.0f, 0.05f, 160.0f);
    camera.position = {7.0f, 5.0f, 10.0f};
    camera.lookAt({0.0f, 1.2f, 0.0f});

    auto ambient = make_ref<AmbientLight>();
    ambient->color = {0.04f, 0.045f, 0.055f};
    ambient->intensity = 1.0f;
    scene.add(ambient);

    auto sun = make_ref<DirectionalLight>();
    sun->position = {6.0f, 9.0f, 7.0f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->intensity = 5.6f;
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.mapSizeX = 2048;
    sun->shadow.mapSizeY = 2048;
    sun->shadow.bias = 0.0011f;
    sun->shadow.radius = 1.5f;
    scene.add(sun);

    auto rim = make_ref<PointLight>();
    rim->position = {-5.0f, 3.2f, 4.5f};
    rim->color = {0.35f, 0.55f, 1.0f};
    rim->intensity = 18.0f;
    rim->distance = 16.0f;
    scene.add(rim);

    auto groundMat = make_ref<MeshStandardMaterial>();
    groundMat->color = {0.52f, 0.50f, 0.46f};
    groundMat->roughness = 0.78f;
    groundMat->metalness = 0.0f;
    groundMat->map = TextureFactory::makeCheckerboard(512, 512, 16, {0.58f, 0.57f, 0.52f}, {0.35f, 0.36f, 0.35f});
    groundMat->map->repeat = {5.0f, 5.0f};
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(18.0f, 12.0f, 12, 8), groundMat);
    ground->receiveShadow = true;
    scene.add(ground);

    auto sphere = GeometryFactory::makeUVSphere(0.48f, 64, 32);
    auto torus = GeometryFactory::makeTorus(0.42f, 0.16f, 64, 16);
    auto cylinder = GeometryFactory::makeCylinder(0.42f, 0.42f, 0.95f, 48, 1, true);
    auto rippleNormal = makeNormalRipple();
    auto roughGradient = makeScalarGradient(0.12f, 0.92f);
    auto metalGradient = makeScalarGradient(0.0f, 1.0f);
    auto stripe = makeStripeTexture({0.95f, 0.37f, 0.16f}, {0.16f, 0.25f, 0.95f});

    // Row 1: MeshStandardMaterial roughness/metalness matrix.
    for (int i = 0; i < 6; ++i) {
        auto mat = make_ref<MeshStandardMaterial>();
        mat->color = glm::mix(glm::vec3(0.94f, 0.45f, 0.22f), glm::vec3(0.55f, 0.72f, 1.0f), float(i) / 5.0f);
        mat->roughness = glm::clamp(0.05f + float(i) * 0.17f, 0.03f, 0.95f);
        mat->metalness = 0.0f;
        mat->envMapIntensity = 1.0f;
        auto m = make_ref<Mesh>(sphere, mat);
        m->position = {float(i - 2.5f) * 1.25f, 0.55f, 1.8f};
        m->castShadow = true;
        m->receiveShadow = true;
        scene.add(m);
    }

    // Row 2: metallic gradient.
    for (int i = 0; i < 6; ++i) {
        auto mat = make_ref<MeshStandardMaterial>();
        mat->color = {0.92f, 0.78f, 0.48f};
        mat->roughness = 0.28f;
        mat->metalness = float(i) / 5.0f;
        mat->envMapIntensity = 1.25f;
        auto m = make_ref<Mesh>(sphere, mat);
        m->position = {float(i - 2.5f) * 1.25f, 0.55f, 0.45f};
        m->castShadow = true;
        m->receiveShadow = true;
        scene.add(m);
    }

    // Row 3: textured Standard materials exercising map/normal/roughness/metalness/light slots.
    for (int i = 0; i < 4; ++i) {
        auto mat = make_ref<MeshStandardMaterial>();
        mat->color = {1.0f, 1.0f, 1.0f};
        mat->map = stripe;
        mat->normalMap = rippleNormal;
        mat->normalScale = {0.85f, 0.85f};
        mat->roughnessMap = roughGradient;
        mat->metalnessMap = i >= 2 ? metalGradient : nullptr;
        mat->roughness = i == 0 ? 0.35f : 0.62f;
        mat->metalness = i >= 2 ? 0.6f : 0.0f;
        mat->envMapIntensity = 1.2f;
        std::shared_ptr<BufferGeometry> geo = (i % 2 == 0) ? torus : cylinder;
        auto m = make_ref<Mesh>(geo, mat);
        m->position = {float(i - 1.5f) * 1.45f, 0.7f, -1.05f};
        m->castShadow = true;
        m->receiveShadow = true;
        scene.add(m);
    }

    // Row 4: MeshPhysicalMaterial features: clearcoat, sheen, transmission, iridescence/anisotropy parameter paths.
    const std::vector<glm::vec3> colors = {{0.85f,0.93f,1.0f},{1.0f,0.72f,0.46f},{0.78f,0.55f,1.0f},{0.45f,0.95f,0.82f}};
    for (int i = 0; i < 4; ++i) {
        auto mat = make_ref<MeshPhysicalMaterial>();
        mat->color = colors[std::size_t(i)];
        mat->roughness = i == 0 ? 0.03f : 0.22f + 0.12f * float(i);
        mat->metalness = i == 1 ? 0.6f : 0.0f;
        mat->ior = 1.35f + 0.08f * float(i);
        mat->clearcoat = i >= 1 ? 1.0f : 0.35f;
        mat->clearcoatRoughness = 0.04f + 0.08f * float(i);
        mat->sheen = i == 2 ? 0.9f : 0.0f;
        mat->sheenColor = {0.9f, 0.25f, 0.65f};
        mat->transmission = i == 0 ? 0.85f : 0.0f;
        mat->thickness = i == 0 ? 0.45f : 0.0f;
        mat->attenuationColor = {0.72f, 0.90f, 1.0f};
        mat->attenuationDistance = 2.2f;
        mat->iridescence = i == 3 ? 0.65f : 0.0f;
        mat->anisotropy = i == 3 ? 0.45f : 0.0f;
        mat->transparent = mat->transmission > 0.0f;
        mat->envMapIntensity = 1.4f;
        auto m = make_ref<Mesh>(sphere, mat);
        m->position = {float(i - 1.5f) * 1.45f, 0.55f, -2.65f};
        m->castShadow = true;
        m->receiveShadow = true;
        scene.add(m);
    }

    scene.add(make_ref<GridHelper>(18.0f, 18, glm::vec3(0.12f, 0.13f, 0.15f)));
    scene.add(make_ref<AxesHelper>(1.6f));

    GLRenderer renderer({1480, 900});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);

    while (!window.shouldClose()) {
        window.pollEvents();
        float t = static_cast<float>(window.time());
        camera.position = {std::sin(t * 0.18f) * 7.4f, 5.0f, std::cos(t * 0.18f) * 10.2f};
        camera.lookAt({0.0f, 0.9f, -0.35f});
        renderer.render(scene, camera);
        window.swapBuffers();
    }
}
