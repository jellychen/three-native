#include "platform/window.h"
#include "renderer/gl-renderer.h"
#include "controls/orbit-controls.h"
#include "helpers/helpers.h"
#include "geometry/fat-line-geometry.h"
#include "helpers/geometry-factory.h"

using namespace THREE;

int main() {
    Window window(1280, 720, "threecpp v4.8 line / points / fatline / helpers parity lab");
    GLRenderer renderer({1280, 720});
    renderer.setClearColor({0.02f, 0.025f, 0.035f, 1.0f});

    Scene scene;
    scene.backgroundColor = {0.02f, 0.025f, 0.035f};

    PerspectiveCamera camera(55.0f, 1280.0f / 720.0f, 0.1f, 200.0f);
    camera.position = {7.0f, 5.0f, 9.0f};
    camera.lookAt({0.0f, 0.8f, 0.0f});
    OrbitControls controls(window, camera);
    controls.target = {0.0f, 0.8f, 0.0f};

    scene.add(make_ref<GridHelper>(16.0f, 16, glm::vec3(0.28f)));
    scene.add(make_ref<AxesHelper>(2.5f));
    scene.add(make_ref<CameraHelper>(camera));

    auto pointsGeo = BufferGeometry::makeRandomPoints(1200, 2.4f);
    auto pointsMat = make_ref<PointsMaterial>();
    pointsMat->color = {0.65f, 0.85f, 1.0f};
    pointsMat->size = 8.0f;
    pointsMat->scale = 420.0f;
    pointsMat->sizeAttenuation = true;
    auto points = make_ref<Points>(pointsGeo, pointsMat);
    points->position = {-4.0f, 2.2f, 0.0f};
    scene.add(points);

    auto pointsMat2 = make_ref<PointsMaterial>();
    pointsMat2->color = {1.0f, 0.72f, 0.35f};
    pointsMat2->size = 5.0f;
    pointsMat2->sizeAttenuation = false;
    auto points2 = make_ref<Points>(BufferGeometry::makeRandomPoints(450, 1.8f), pointsMat2);
    points2->position = {-4.0f, 1.8f, -4.0f};
    scene.add(points2);

    std::vector<glm::vec3> curve;
    for (int i = 0; i < 80; ++i) {
        float t = float(i) / 79.0f;
        float x = -3.2f + t * 6.4f;
        curve.push_back({x, 1.2f + std::sin(t * glm::two_pi<float>() * 2.0f) * 0.6f, -2.6f + std::cos(t * glm::two_pi<float>() * 1.5f) * 0.35f});
    }
    auto fatMat = make_ref<FatLineMaterial>();
    fatMat->color = {0.1f, 0.9f, 0.95f};
    fatMat->linewidth = 8.0f;
    fatMat->dashed = false;
    auto line2 = make_ref<Line2>(FatLineGeometry::fromPolyline(curve), fatMat);
    scene.add(line2);

    auto dashedMat = make_ref<FatLineMaterial>();
    dashedMat->color = {1.0f, 0.65f, 0.15f};
    dashedMat->linewidth = 5.0f;
    dashedMat->dashed = true;
    dashedMat->dashSize = 0.35f;
    dashedMat->gapSize = 0.22f;
    auto dashed = make_ref<LineSegments2>(FatLineGeometry::fromPolyline({{-3,0.65f,2.5f},{-1.2f,1.6f,2.5f},{0.6f,0.4f,2.5f},{2.8f,1.4f,2.5f}}), dashedMat);
    scene.add(dashed);

    auto sphereGeo = GeometryFactory::makeSphere(1.0f, 32, 16);
    auto sphereMat = make_ref<MeshStandardMaterial>();
    sphereMat->color = {0.8f, 0.8f, 0.9f};
    sphereMat->roughness = 0.45f;
    sphereMat->metalness = 0.1f;
    auto sphere = make_ref<Mesh>(sphereGeo, sphereMat);
    sphere->position = {2.8f, 1.1f, 0.0f};
    scene.add(sphere);
    scene.add(make_ref<VertexNormalsHelper>(*sphere, 0.25f));
    scene.children.back()->position = sphere->position;

    auto dir = make_ref<DirectionalLight>();
    dir->position = {3.5f, 5.5f, 4.0f};
    dir->target = {0,0,0};
    dir->color = {1.0f, 0.95f, 0.88f};
    dir->intensity = 2.0f;
    scene.add(dir);
    auto dirHelper = make_ref<DirectionalLightHelper>(*dir, 0.8f);
    dirHelper->position = dir->position;
    dirHelper->lookAt(dir->target);
    scene.add(dirHelper);

    auto point = make_ref<PointLight>();
    point->position = {-1.7f, 2.8f, 2.8f};
    point->color = {0.35f, 0.6f, 1.0f};
    point->intensity = 8.0f;
    point->distance = 10.0f;
    scene.add(point);
    auto pHelper = make_ref<PointLightHelper>(*point, 0.35f);
    pHelper->position = point->position;
    scene.add(pHelper);

    auto spot = make_ref<SpotLight>();
    spot->position = {3.0f, 5.0f, 3.0f};
    spot->target = {0.0f, 0.5f, 0.0f};
    spot->angle = glm::radians(22.0f);
    spot->penumbra = 0.35f;
    spot->color = {1.0f, 0.55f, 0.35f};
    spot->intensity = 5.0f;
    scene.add(spot);
    auto sHelper = make_ref<SpotLightHelper>(*spot, 3.5f);
    sHelper->position = spot->position;
    sHelper->lookAt(spot->target);
    scene.add(sHelper);

    while (!window.shouldClose()) {
        window.poll();
        float t = static_cast<float>(window.time());
        points->rotation.y = t * 0.15f;
        line2->rotation.y = std::sin(t * 0.4f) * 0.2f;
        controls.update();
        renderer.render(scene, camera);
        window.swapBuffers();
    }
    return 0;
}
