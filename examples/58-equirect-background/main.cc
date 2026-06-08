#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "helpers/geometry-factory.h"
#include "light/light.h"

using namespace THREE;

int main() {
    Window window(1280, 720, "threecpp 58 equirect background");
    Scene scene;
    PerspectiveCamera camera(55.0f, 1280.0f / 720.0f, 0.1f, 100.0f);
    camera.position = {0.0f, 1.0f, 3.0f};
    camera.lookAt({0.0f, 0.0f, 0.0f});

    // Generate a procedural sky gradient as equirect background
    auto bgTex = make_ref<Texture>();
    bgTex->width = 512;
    bgTex->height = 256;
    bgTex->channels = 3;
    bgTex->pixels.resize(static_cast<std::size_t>(512 * 256 * 3));
    for (int y = 0; y < 256; ++y) {
        for (int x = 0; x < 512; ++x) {
            float u = static_cast<float>(x) / 511.0f;
            float v = static_cast<float>(y) / 255.0f;
            float sky = 1.0f - v * 0.8f;
            float horizon = 1.0f - std::abs(v - 0.3f) * 2.0f;
            horizon = std::max(0.0f, std::min(1.0f, horizon));
            float r = (0.5f + 0.3f * u) * sky + 0.9f * horizon * (1.0f - sky);
            float g = (0.6f + 0.3f * u) * sky + 0.7f * horizon * (1.0f - sky);
            float b = (0.9f + 0.1f * u) * sky + 1.0f * horizon * (1.0f - sky);
            std::size_t idx = static_cast<std::size_t>((y * 512 + x) * 3);
            bgTex->pixels[idx + 0] = std::byte(static_cast<uint8_t>(r * 255.0f));
            bgTex->pixels[idx + 1] = std::byte(static_cast<uint8_t>(g * 255.0f));
            bgTex->pixels[idx + 2] = std::byte(static_cast<uint8_t>(b * 255.0f));
        }
    }
    bgTex->colorSpace = ColorSpace::LinearSRGB;
    bgTex->mapping = TextureMapping::EquirectangularReflection;
    scene.background = bgTex;

    // A simple reflective sphere
    auto sphereMat = make_ref<MeshStandardMaterial>();
    sphereMat->color = {0.8f, 0.3f, 0.2f};
    sphereMat->roughness = 0.2f;
    sphereMat->metalness = 0.8f;
    sphereMat->envMapIntensity = 1.0f;
    auto sphere = make_ref<Mesh>(GeometryFactory::makeSphere(0.8f, 64, 48), sphereMat);
    sphere->position = {0.0f, 0.0f, 0.0f};
    scene.add(sphere);

    // Ambient light
    auto ambient = make_ref<AmbientLight>();
    ambient->intensity = 0.3f;
    scene.add(ambient);

    GLRenderer renderer({1280, 720, true, {0.0f, 0.0f, 0.0f, 1.0f}});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);
    while (!window.shouldClose()) {
        window.pollEvents();
        sphere->quaternion = glm::angleAxis(static_cast<float>(glfwGetTime()) * 0.3f, glm::vec3(0.0f, 1.0f, 0.0f));
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
