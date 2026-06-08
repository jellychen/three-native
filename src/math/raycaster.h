#pragma once
#include "core/camera.h"
#include "core/renderable.h"
#include "geometry/buffer-geometry.h"
#include <vector>
#include <algorithm>
#include <limits>
#include <glm/glm.hpp>

namespace THREE {

struct Ray {
    glm::vec3 origin{0.0f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};

    Ray() = default;
    Ray(const glm::vec3& o, const glm::vec3& d);

    glm::vec3 at(float t) const;

    // Moller-Trumbore ray-triangle intersection
    // Returns true and sets distance, uv if hit
    bool intersectTriangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
                           bool backfaceCulling, float& outDist, float& outU, float& outV) const;

    bool intersectSphere(const glm::vec3& center, float radius) const;

    bool intersectBox(const glm::vec3& min, const glm::vec3& max) const;
};

struct Intersection {
    float distance = std::numeric_limits<float>::max();
    float distanceToRay = 0.0f;
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f};
    int faceIndex = 0;
    int instanceId = -1;
    Object3D* object = nullptr;
    float u = 0.0f, v = 0.0f;
};

class Raycaster {
public:
    Ray ray;
    float near = 0.0f;
    float far = std::numeric_limits<float>::infinity();
    Camera* camera = nullptr;
    uint32_t layers = 1u;
    
    struct Params {
        struct { float threshold = 0.0f; } Mesh;
        struct { float threshold = 1.0f; } Line;
        struct { float threshold = 1.0f; } Points;
        struct { } LOD;
        struct { } Sprite;
    } params;

    Raycaster() = default;
    Raycaster(const glm::vec3& origin, const glm::vec3& direction, float near = 0.0f, float far = std::numeric_limits<float>::infinity());

    void set(const glm::vec3& origin, const glm::vec3& direction);

    void setFromCamera(const glm::vec2& ndc, Camera& cam);

    // Intersect a single object and its children
    std::vector<Intersection> intersectObject(Object3D& object, bool recursive = true);

    // Intersect a list of objects
    std::vector<Intersection> intersectObjects(std::vector<Object3D*>& objects, bool recursive = true);

private:
    void intersectRecursive(Object3D& object, std::vector<Intersection>& results, bool recursive);

    void raycastObject(Object3D& object, std::vector<Intersection>& results);

    void raycastMesh(Mesh& mesh, std::vector<Intersection>& results);
};

} // namespace THREE
