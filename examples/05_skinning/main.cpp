#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "core/Renderable.hpp"
#include "helpers/GeometryFactory.hpp"

using namespace threecpp;

int main() {
    Window window(1280, 720, "threecpp 05 skinning scaffold");
    Scene scene;
    PerspectiveCamera camera(55.0f, 1280.0f / 720.0f, 0.1f, 100.0f);
    camera.position = {3, 2, 6};
    camera.quaternion = glm::quatLookAt(glm::normalize(glm::vec3(0, 0.8f, 0) - camera.position), glm::vec3(0, 1, 0));

    auto geo = GeometryFactory::makeCube(1.0f);
    std::vector<float> skinIndex(geo->vertexCount() * 4, 0.0f);
    std::vector<float> skinWeight(geo->vertexCount() * 4, 0.0f);
    for (int i = 0; i < geo->vertexCount(); ++i) skinWeight[i * 4] = 1.0f;
    geo->setAttribute("skinIndex", BufferAttribute::fromVector(skinIndex, 4, AttributeType::Float32));
    geo->setAttribute("skinWeight", BufferAttribute::fromVector(skinWeight, 4, AttributeType::Float32));

    auto mat = make_ref<MeshStandardMaterial>();
    mat->color = {0.45f, 0.7f, 1.0f};
    mat->roughness = 0.45f;
    auto skinned = make_ref<SkinnedMesh>(geo, mat);

    auto bone = make_ref<Bone>();
    bone->boneIndex = 0;
    skinned->add(bone);
    auto skeleton = make_ref<Skeleton>();
    skeleton->bones.push_back(bone.get());
    skeleton->calculateInverses();
    skinned->bind(skeleton);
    scene.add(skinned);

    auto ambient = make_ref<AmbientLight>();
    ambient->intensity = 1.0f;
    scene.add(ambient);

    GLRenderer renderer({1280, 720});
    while (!window.shouldClose()) {
        window.pollEvents();
        float t = static_cast<float>(glfwGetTime());
        bone->quaternion = glm::angleAxis(std::sin(t) * 0.6f, glm::vec3(0, 1, 0));
        renderer.render(scene, camera);
        window.swapBuffers();
    }
}
