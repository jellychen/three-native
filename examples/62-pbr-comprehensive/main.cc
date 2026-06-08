#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "core/renderable.h"
#include "light/light.h"
#include "ibl/environment.h"
#include "helpers/geometry-factory.h"
#include "helpers/helpers.h"
#include "controls/orbit-controls.h"

using namespace THREE;

struct SphereRow {
    const char* label;
    std::function<void(std::shared_ptr<MeshPhysicalMaterial>&, float t)> setup;
};

int main() {
    Window window(1920, 1080, "threecpp 62 PBR Comprehensive Test");
    window.poll();
    Scene scene;
    scene.backgroundColor = {0.025f, 0.025f, 0.04f, 1.0f};

    // ── Environment ──────────────────────────────────────────
    auto hdrTex = make_ref<Texture>();
    hdrTex->width = 128; hdrTex->height = 64; hdrTex->channels = 3;
    hdrTex->pixels.resize(static_cast<std::size_t>(128 * 64 * 3));
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 128; ++x) {
            float u = float(x) / 127.0f, v = float(y) / 63.0f;
            float sky = pow(1.0f - v, 1.5f);
            float sun = exp(-pow((u - 0.72f) * 20.0f, 2.0f) - pow((v - 0.15f) * 12.0f, 2.0f));
            float r = (0.35f + 0.65f * sky + sun) * 0.8f;
            float g = (0.40f + 0.60f * sky + sun * 0.95f) * 0.7f;
            float b = (0.55f + 0.45f * sky + sun * 0.85f) * 1.0f;
            auto idx = static_cast<std::size_t>((y * 128 + x) * 3);
            hdrTex->pixels[idx+0] = std::byte(uint8_t(r * 255));
            hdrTex->pixels[idx+1] = std::byte(uint8_t(g * 255));
            hdrTex->pixels[idx+2] = std::byte(uint8_t(b * 255));
        }
    }
    hdrTex->colorSpace = ColorSpace::LinearSRGB;
    hdrTex->mapping = TextureMapping::EquirectangularReflection;
    scene.background = hdrTex;

    PMREMGenerator pmrem({128, 7, true, 64, 64, true});
    auto env = pmrem.fromEquirectangular(hdrTex);
    scene.environment = env;

    // ── Camera ──────────────────────────────────────────────
    PerspectiveCamera camera(45.0f, 1920.0f/1080.0f, 0.1f, 100.0f);
    camera.position = {6.0f, 5.0f, 9.0f};
    camera.lookAt({0.0f, 1.0f, 0.0f});
    OrbitControls controls(camera, window);
    controls.target = {0.0f, 1.0f, 0.0f};

    // ── Lights ──────────────────────────────────────────────
    auto sun = make_ref<DirectionalLight>();
    sun->color = {1.0f, 0.95f, 0.85f};
    sun->intensity = 3.0f;
    sun->position = {5.0f, 8.0f, 4.0f};
    sun->target = {0.0f, 0.0f, 0.0f};
    sun->castShadow = true;
    sun->shadow.enabled = true;
    sun->shadow.mapSize = {2048, 2048};
    sun->shadow.cameraLeft = sun->shadow.cameraBottom = -6.0f;
    sun->shadow.cameraRight = sun->shadow.cameraTop = 6.0f;
    sun->shadow.cameraNear = 0.5f; sun->shadow.cameraFar = 30.0f;
    sun->shadow.bias = 0.003f;
    scene.add(sun);

    auto fill = make_ref<HemisphereLight>();
    fill->skyColor = {0.35f, 0.45f, 0.75f};
    fill->groundColor = {0.08f, 0.05f, 0.03f};
    fill->intensity = 0.6f;
    scene.add(fill);

    auto rim = make_ref<PointLight>();
    rim->color = {1.0f, 0.6f, 0.2f};
    rim->intensity = 1.2f;
    rim->distance = 10.0f;
    rim->position = {-3.0f, 3.5f, 3.0f};
    rim->castShadow = true;
    rim->shadow.enabled = true;
    scene.add(rim);

    // ── Ground plane ──────────────────────────────────────
    auto groundMat = make_ref<MeshPhysicalMaterial>();
    groundMat->color = {0.55f, 0.55f, 0.50f};
    groundMat->roughness = 0.8f; groundMat->metalness = 0.0f;
    auto checker = make_ref<Texture>();
    checker->width = checker->height = 512; checker->channels = 3;
    checker->pixels.resize(512u * 512u * 3);
    for (int y = 0; y < 512; ++y) {
        for (int x = 0; x < 512; ++x) {
            bool b = ((x/32) + (y/32)) % 2 == 0;
            float v = b ? 0.55f : 0.30f;
            auto idx = static_cast<std::size_t>((y * 512 + x) * 3);
            checker->pixels[idx+0] = std::byte(uint8_t(v*255));
            checker->pixels[idx+1] = std::byte(uint8_t(v*255));
            checker->pixels[idx+2] = std::byte(uint8_t(v*255));
        }
    }
    checker->repeat = {4, 4};
    groundMat->map = checker;
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(20, 14, 40, 28), groundMat);
    ground->position = {0.0f, -0.15f, 0.0f};
    ground->rotation = {-1.5708f, 0.0f, 0.0f};
    ground->receiveShadow = true;
    scene.add(ground);

    // ── PBR Test Spheres ──────────────────────────────────
    int cols = 7, rowsDef = 7;
    float colSpacing = 0.85f, rowSpacing = 1.1f;
    float startX = (cols-1) * colSpacing * -0.5f;
    float startZ = (rowsDef-1) * rowSpacing * -0.5f;

    // Row definitions: each row tests one PBR parameter
    glm::vec3 baseColors[7] = {
        {0.85f, 0.30f, 0.20f},  // row0: warm
        {0.20f, 0.55f, 0.85f},  // row1: cool
        {0.90f, 0.75f, 0.20f},  // row2: yellow
        {0.75f, 0.25f, 0.75f},  // row3: purple
        {0.20f, 0.85f, 0.45f},  // row4: green
        {0.90f, 0.45f, 0.15f},  // row5: orange
        {0.50f, 0.80f, 0.95f},  // row6: sky
    };

    for (int row = 0; row < rowsDef; ++row) {
        for (int col = 0; col < cols; ++col) {
            float t = float(col) / float(cols - 1);  // 0..1
            auto mat = make_ref<MeshPhysicalMaterial>();
            mat->color = baseColors[row];
            mat->envMapIntensity = 1.5f;
            if (row == 0) {
                // Color row: hue sweep, fixed metal/rough
                float hue = float(col) / float(cols);
                mat->color = glm::mix(glm::vec3(1,0.2f,0.1f), glm::vec3(0.1f,0.3f,1), hue);
                mat->roughness = 0.3f;
                mat->metalness = 0.4f;
            } else if (row == 1) {
                // Metalness sweep
                mat->metalness = t;
                mat->roughness = 0.25f;
            } else if (row == 2) {
                // Roughness sweep
                mat->roughness = t;
                mat->metalness = 0.5f;
            } else if (row == 3) {
                // Clearcoat sweep
                mat->roughness = 0.35f;
                mat->metalness = 0.2f;
                mat->clearcoat = t;
                mat->clearcoatRoughness = 0.2f;
            } else if (row == 4) {
                // Sheen sweep
                mat->roughness = 0.3f;
                mat->metalness = 0.0f;
                mat->sheen = t;
                mat->sheenColor = glm::vec3(1.0f - t*0.5f, 0.7f - t*0.3f, 1.0f);
                mat->sheenRoughness = glm::mix(0.1f, 0.8f, t);
            } else if (row == 5) {
                // Iridescence sweep
                mat->roughness = 0.2f;
                mat->metalness = 0.0f;
                mat->iridescence = t;
                mat->iridescenceIOR = glm::mix(1.2f, 2.0f, t);
                mat->iridescenceThicknessMinimum = 80.0f + t * 200.0f;
                mat->iridescenceThicknessMaximum = mat->iridescenceThicknessMinimum + 150.0f;
            } else if (row == 6) {
                // Transmission + IOR sweep
                mat->roughness = 0.1f;
                mat->metalness = 0.0f;
                mat->transmission = t;
                mat->thickness = glm::mix(0.5f, 4.0f, t);
                mat->ior = glm::mix(1.2f, 2.0f, t);
                mat->attenuationDistance = 2.0f;
                mat->attenuationColor = {0.85f, 0.9f, 1.0f};
                mat->specularIntensity = 1.0f;
                mat->specularColor = {1.0f, 1.0f, 1.0f};
            }
            // Column 0 shows the base case for reference
            auto sphere = make_ref<Mesh>(GeometryFactory::makeSphere(0.35f, 48, 32), mat);
            sphere->position = {
                startX + float(col) * colSpacing,
                0.4f,
                startZ + float(row) * rowSpacing
            };
            sphere->castShadow = true;
            sphere->receiveShadow = true;
            scene.add(sphere);
        }
    }

    // ── Scene helpers ──────────────────────────────────────
    auto grid = make_ref<GridHelper>(12.0f, 12);
    grid->position = {0.0f, 0.0f, 0.0f};
    scene.add(grid);

    auto axes = make_ref<AxesHelper>(1.5f);
    axes->position = {-6.0f, 0.0f, -4.0f};
    scene.add(axes);

    // ── Renderer ────────────────────────────────────────────
    GLRenderer renderer({1920, 1080, true, {0.025f,0.025f,0.04f,1.0f}});
    renderer.setToneMapping(ToneMapping::ACESFilmic, 1.2f);
    renderer.setOutputColorSpace(ColorSpace::SRGB);

    while (!window.shouldClose()) {
        window.poll();
        controls.update();
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
