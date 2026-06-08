#include "math/raycaster.h"
#include "common.h"

namespace THREE {

Raycaster::Raycaster(const glm::vec3& origin, const glm::vec3& direction, float near, float far)
    : ray(origin, direction), near(near), far(far) {}


// Ray methods
glm::vec3 Ray::at(float t) const {
    return origin + direction * t;
}

bool Ray::intersectTriangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
                             bool backfaceCulling, float& outDist, float& outU, float& outV) const {
    const float eps = 1e-6f;
    glm::vec3 edge1 = b - a;
    glm::vec3 edge2 = c - a;
    glm::vec3 h = glm::cross(direction, edge2);
    float det = glm::dot(edge1, h);
    if (backfaceCulling) {
        if (det < eps) return false;
    } else {
        if (det > -eps && det < eps) return false;
    }
    float invDet = 1.0f / det;
    glm::vec3 s = origin - a;
    float u = glm::dot(s, h) * invDet;
    if (u < 0.0f || u > 1.0f) return false;
    glm::vec3 q = glm::cross(s, edge1);
    float v = glm::dot(direction, q) * invDet;
    if (v < 0.0f || u + v > 1.0f) return false;
    float t = glm::dot(edge2, q) * invDet;
    if (t < eps) return false;
    outDist = t;
    outU = u;
    outV = v;
    return true;
}

bool Ray::intersectSphere(const glm::vec3& center, float radius) const {
    glm::vec3 oc = origin - center;
    float a = glm::dot(direction, direction);
    float b = 2.0f * glm::dot(oc, direction);
    float c = glm::dot(oc, oc) - radius * radius;
    float disc = b * b - 4.0f * a * c;
    return disc >= 0.0f;
}

bool Ray::intersectBox(const glm::vec3& min, const glm::vec3& max) const {
    float tmin = -std::numeric_limits<float>::infinity();
    float tmax = std::numeric_limits<float>::infinity();
    for (int i = 0; i < 3; ++i) {
        float invD = 1.0f / direction[i];
        float t0 = (min[i] - origin[i]) * invD;
        float t1 = (max[i] - origin[i]) * invD;
        if (invD < 0.0f) std::swap(t0, t1);
        tmin = std::max(tmin, t0);
        tmax = std::min(tmax, t1);
        if (tmax < tmin) return false;
    }
    return tmax >= 0.0f;
}

// Raycaster methods
void Raycaster::set(const glm::vec3& origin, const glm::vec3& direction) {
    ray.origin = origin;
    ray.direction = glm::normalize(direction);
}

void Raycaster::setFromCamera(const glm::vec2& ndc, Camera& cam) {
    camera = &cam;
    PerspectiveCamera* pc = dynamic_cast<PerspectiveCamera*>(&cam);
    if (pc) {
        glm::vec4 clipPos(ndc.x, ndc.y, 1.0f, 1.0f);
        glm::mat4 invProj = glm::inverse(pc->projectionMatrix);
        glm::vec4 viewPos = invProj * clipPos;
        viewPos /= viewPos.w;
        ray.origin = glm::vec3(pc->matrixWorld[3]);
        glm::mat4 invView = glm::inverse(pc->matrixWorldInverse);
        glm::vec4 worldPos4 = invView * viewPos;
        glm::vec3 worldPos = glm::vec3(worldPos4) / worldPos4.w;
        ray.direction = glm::normalize(worldPos - ray.origin);
    } else {
        OrthographicCamera* oc = dynamic_cast<OrthographicCamera*>(&cam);
        if (oc) {
            glm::vec4 clipPos(ndc.x, ndc.y, 0.5f, 1.0f);
            glm::mat4 invProj = glm::inverse(oc->projectionMatrix);
            glm::vec4 viewPos = invProj * clipPos;
            viewPos /= viewPos.w;
            glm::mat4 invView = glm::inverse(oc->matrixWorldInverse);
            glm::vec4 worldPos4 = invView * viewPos;
            glm::vec3 worldPos = glm::vec3(worldPos4) / worldPos4.w;
            ray.origin = worldPos;
            ray.direction = glm::normalize(glm::vec3(invView * glm::vec4(0, 0, -1, 0)));
        }
    }
}

std::vector<Intersection> Raycaster::intersectObject(Object3D& object, bool recursive) {
    std::vector<Intersection> results;
    intersectRecursive(object, results, recursive);
    std::sort(results.begin(), results.end(), [](const Intersection& a, const Intersection& b) {
        return a.distance < b.distance;
    });
    return results;
}

std::vector<Intersection> Raycaster::intersectObjects(std::vector<Object3D*>& objects, bool recursive) {
    std::vector<Intersection> results;
    for (auto* obj : objects) {
        if (obj) intersectRecursive(*obj, results, recursive);
    }
    std::sort(results.begin(), results.end(), [](const Intersection& a, const Intersection& b) {
        return a.distance < b.distance;
    });
    return results;
}

void Raycaster::intersectRecursive(Object3D& object, std::vector<Intersection>& results, bool recursive) {
    if ((layers & object.layers) == 0) return;
    raycastObject(object, results);
    if (recursive) {
        for (const auto& child : object.children) {
            if (child) intersectRecursive(*child, results, true);
        }
    }
}

void Raycaster::raycastObject(Object3D& object, std::vector<Intersection>& results) {
    Mesh* mesh = dynamic_cast<Mesh*>(&object);
    if (mesh && mesh->geometry) {
        raycastMesh(*mesh, results);
        return;
    }
}

void Raycaster::raycastMesh(Mesh& mesh, std::vector<Intersection>& results) {
    auto& geo = *mesh.geometry;
    auto posAttr = geo.getAttribute("position");
    if (!posAttr) return;
    if (posAttr->itemSize < 3) return;

    glm::mat4 worldMatrix = mesh.matrixWorld;
    glm::mat4 invWorld = glm::inverse(worldMatrix);

    glm::vec3 localOrigin = glm::vec3(invWorld * glm::vec4(ray.origin, 1.0f));
    glm::vec3 localDir = glm::normalize(glm::mat3(invWorld) * ray.direction);
    Ray localRay(localOrigin, localDir);

    const float* verts = reinterpret_cast<const float*>(posAttr->data.data());
    int vertCount = posAttr->count;
    int stride = posAttr->itemSize;

    const auto& indices = geo.indices;
    bool indexed = !indices.empty();

    int drawStart = geo.drawRange.start;
    int drawLimit = geo.drawRange.count;
    int triCount = indexed ? (static_cast<int>(indices.size()) / 3) : (vertCount / 3 - drawStart / 3);
    if (drawLimit < std::numeric_limits<int>::max()) {
        triCount = std::min(triCount, drawLimit / 3);
    }

    int maxTri = indexed ? static_cast<int>(indices.size()) / 3 : vertCount / 3;
    int startTri = drawStart / 3;
    int endTri = std::min(startTri + triCount, maxTri);

    for (int ti = startTri; ti < endTri; ++ti) {
        int i0, i1, i2;
        if (indexed) {
            i0 = static_cast<int>(indices[static_cast<std::size_t>(ti) * 3]);
            i1 = static_cast<int>(indices[static_cast<std::size_t>(ti) * 3 + 1]);
            i2 = static_cast<int>(indices[static_cast<std::size_t>(ti) * 3 + 2]);
        } else {
            i0 = ti * 3;
            i1 = ti * 3 + 1;
            i2 = ti * 3 + 2;
        }

        glm::vec3 a(verts[i0 * stride], verts[i0 * stride + 1], verts[i0 * stride + 2]);
        glm::vec3 b(verts[i1 * stride], verts[i1 * stride + 1], verts[i1 * stride + 2]);
        glm::vec3 c(verts[i2 * stride], verts[i2 * stride + 1], verts[i2 * stride + 2]);

        float dist = 0, u = 0, v = 0;
        bool backfaceCulling = true;
        if (mesh.material) {
            backfaceCulling = (mesh.material->side == Side::FrontSide);
        }
        if (localRay.intersectTriangle(a, b, c, backfaceCulling, dist, u, v)) {
            if (dist < near || dist > far) continue;
            glm::vec3 localPt = localRay.at(dist);
            glm::vec3 worldPt = glm::vec3(worldMatrix * glm::vec4(localPt, 1.0f));
            glm::vec3 edge1 = b - a;
            glm::vec3 edge2 = c - a;
            glm::vec3 localNorm = glm::normalize(glm::cross(edge1, edge2));
            glm::vec3 worldNorm = glm::normalize(glm::mat3(worldMatrix) * localNorm);
            float worldDist = glm::distance(worldPt, ray.origin);

            Intersection hit;
            hit.distance = worldDist;
            hit.point = worldPt;
            hit.normal = worldNorm;
            hit.faceIndex = ti;
            hit.object = &mesh;
            hit.u = u;
            hit.v = v;
            results.push_back(hit);
        }
    }
}

} // namespace THREE
