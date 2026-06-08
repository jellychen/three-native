#pragma once
#include "core/renderable.h"
#include "helpers/geometry-factory.h"
#include "core/camera.h"
#include "light/light.h"

namespace THREE {

class AxesHelper : public LineSegments {
public:
    explicit AxesHelper(float size = 1.0f) {
        auto g = std::make_shared<BufferGeometry>();
        std::vector<float> p = {0,0,0, size,0,0, 0,0,0, 0,size,0, 0,0,0, 0,0,size};
        std::vector<float> c = {1,0,0, 1,0,0, 0,1,0, 0,1,0, 0.2f,0.45f,1, 0.2f,0.45f,1};
        g->setAttribute("position", BufferAttribute::fromVector(p, 3, AttributeType::Float32));
        g->setAttribute("color", BufferAttribute::fromVector(c, 3, AttributeType::Float32));
        auto m = std::make_shared<LineBasicMaterial>();
        m->vertexColors = true;
        geometry = g;
        material = m;
        kind = ObjectKind::LineSegments;
    }
};

class GridHelper : public LineSegments {
public:
    GridHelper(float size = 10.0f, int divisions = 10, const glm::vec3& color = glm::vec3(0.38f)) {
        geometry = GeometryFactory::makeGrid(divisions, size);
        auto m = std::make_shared<LineBasicMaterial>();
        m->color = color;
        material = m;
        kind = ObjectKind::LineSegments;
    }
};

class BoxHelper : public LineSegments {
public:
    BoxHelper(const glm::vec3& min = glm::vec3(-0.5f), const glm::vec3& max = glm::vec3(0.5f), const glm::vec3& color = glm::vec3(1.0f, 0.8f, 0.15f)) {
        auto g = std::make_shared<BufferGeometry>();
        const glm::vec3 v[8] = {
            {min.x,min.y,min.z},{max.x,min.y,min.z},{max.x,max.y,min.z},{min.x,max.y,min.z},
            {min.x,min.y,max.z},{max.x,min.y,max.z},{max.x,max.y,max.z},{min.x,max.y,max.z}
        };
        const int e[24] = {0,1,1,2,2,3,3,0, 4,5,5,6,6,7,7,4, 0,4,1,5,2,6,3,7};
        std::vector<float> p;
        p.reserve(72);
        for (int i : e) p.insert(p.end(), {v[i].x, v[i].y, v[i].z});
        g->setAttribute("position", BufferAttribute::fromVector(p, 3, AttributeType::Float32));
        auto m = std::make_shared<LineBasicMaterial>();
        m->color = color;
        geometry = g;
        material = m;
        kind = ObjectKind::LineSegments;
    }
};

class SkeletonHelper : public LineSegments {
public:
    explicit SkeletonHelper(Object3D* root = nullptr) : root_(root) {
        auto m = std::make_shared<LineBasicMaterial>();
        m->color = glm::vec3(0.35f, 0.75f, 1.0f);
        material = m;
        geometry = std::make_shared<BufferGeometry>();
        kind = ObjectKind::LineSegments;
        if (root_) update();
    }

    void setRoot(Object3D* root) {
        root_ = root;
        update();
    }

    // three.js-style helper refresh entry point. Call this after skeleton animation
    // updates and before rendering if the bone matrices have changed.
    void update() {
        if (!root_) {
            geometry->clear();
            return;
        }
        updateFromRoot(*root_);
    }

    void updateFromRoot(Object3D& root) {
        std::vector<float> p;
        root.updateMatrixWorld(true);
        root.traverse([&](Object3D& o) {
            Object3D* parent = o.parentObject();
            if (o.kind != ObjectKind::Bone || !parent || parent->kind != ObjectKind::Bone) return;
            glm::vec3 a = glm::vec3(parent->matrixWorld[3]);
            glm::vec3 b = glm::vec3(o.matrixWorld[3]);
            p.insert(p.end(), {a.x,a.y,a.z, b.x,b.y,b.z});
        });
        geometry->clear();
        geometry->setAttribute("position", BufferAttribute::fromVector(p, 3, AttributeType::Float32));
    }

private:
    Object3D* root_ = nullptr;
};


class CameraHelper : public LineSegments {
public:
    explicit CameraHelper(const Camera& camera, const glm::vec3& color = glm::vec3(1.0f, 0.85f, 0.2f)) {
        auto g = std::make_shared<BufferGeometry>();
        std::vector<float> p;
        auto emit = [&](glm::vec3 a, glm::vec3 b) { p.insert(p.end(), {a.x,a.y,a.z,b.x,b.y,b.z}); };
        glm::mat4 inv = glm::inverse(camera.projectionMatrix);
        auto unproject = [&](float x, float y, float z) {
            glm::vec4 q = inv * glm::vec4(x, y, z, 1.0f);
            return glm::vec3(q) / std::max(q.w, 1e-6f);
        };
        glm::vec3 ntl = unproject(-1,  1, -1), ntr = unproject( 1,  1, -1), nbr = unproject( 1, -1, -1), nbl = unproject(-1, -1, -1);
        glm::vec3 ftl = unproject(-1,  1,  1), ftr = unproject( 1,  1,  1), fbr = unproject( 1, -1,  1), fbl = unproject(-1, -1,  1);
        emit(ntl,ntr); emit(ntr,nbr); emit(nbr,nbl); emit(nbl,ntl);
        emit(ftl,ftr); emit(ftr,fbr); emit(fbr,fbl); emit(fbl,ftl);
        emit(ntl,ftl); emit(ntr,ftr); emit(nbr,fbr); emit(nbl,fbl);
        emit(glm::vec3(0), ntl); emit(glm::vec3(0), ntr); emit(glm::vec3(0), nbr); emit(glm::vec3(0), nbl);
        g->setAttribute("position", BufferAttribute::fromVector(p, 3, AttributeType::Float32));
        geometry = g;
        auto m = std::make_shared<LineBasicMaterial>();
        m->color = color;
        material = m;
        kind = ObjectKind::LineSegments;
    }
};

class DirectionalLightHelper : public LineSegments {
public:
    explicit DirectionalLightHelper(const DirectionalLight& light, float size = 1.0f) {
        auto g = std::make_shared<BufferGeometry>();
        std::vector<float> p = {-size,0,0, size,0,0, 0,-size,0, 0,size,0, 0,0,0, 0,0,-size * 2.0f};
        g->setAttribute("position", BufferAttribute::fromVector(p, 3, AttributeType::Float32));
        geometry = g;
        auto m = std::make_shared<LineBasicMaterial>();
        m->color = light.color;
        material = m;
        kind = ObjectKind::LineSegments;
    }
};

class PointLightHelper : public LineSegments {
public:
    explicit PointLightHelper(const PointLight& light, float sphereSize = 1.0f, int segments = 24) {
        auto g = std::make_shared<BufferGeometry>();
        std::vector<float> p;
        auto emitCircle = [&](int axis) {
            for (int i = 0; i < segments; ++i) {
                float a = glm::two_pi<float>() * float(i) / float(segments);
                float b = glm::two_pi<float>() * float(i + 1) / float(segments);
                glm::vec3 p0(0), p1(0);
                if (axis == 0) { p0 = {0, std::cos(a)*sphereSize, std::sin(a)*sphereSize}; p1 = {0, std::cos(b)*sphereSize, std::sin(b)*sphereSize}; }
                if (axis == 1) { p0 = {std::cos(a)*sphereSize, 0, std::sin(a)*sphereSize}; p1 = {std::cos(b)*sphereSize, 0, std::sin(b)*sphereSize}; }
                if (axis == 2) { p0 = {std::cos(a)*sphereSize, std::sin(a)*sphereSize, 0}; p1 = {std::cos(b)*sphereSize, std::sin(b)*sphereSize, 0}; }
                p.insert(p.end(), {p0.x,p0.y,p0.z,p1.x,p1.y,p1.z});
            }
        };
        emitCircle(0); emitCircle(1); emitCircle(2);
        g->setAttribute("position", BufferAttribute::fromVector(p, 3, AttributeType::Float32));
        geometry = g;
        auto m = std::make_shared<LineBasicMaterial>();
        m->color = light.color;
        material = m;
        kind = ObjectKind::LineSegments;
    }
};

class SpotLightHelper : public LineSegments {
public:
    explicit SpotLightHelper(const SpotLight& light, float length = 4.0f, int segments = 32)
        : helperLength(length), helperSegments(segments) {
        rebuild(light);
        update(light);
    }

    void rebuild(const SpotLight& light) {
        auto g = std::make_shared<BufferGeometry>();
        std::vector<float> p;
        float r = std::tan(light.angle) * helperLength;
        glm::vec3 apex(0), center(0,0,-helperLength);
        for (int i = 0; i < helperSegments; ++i) {
            float a = glm::two_pi<float>() * float(i) / float(helperSegments);
            float b = glm::two_pi<float>() * float(i + 1) / float(helperSegments);
            glm::vec3 p0(std::cos(a)*r, std::sin(a)*r, -helperLength);
            glm::vec3 p1(std::cos(b)*r, std::sin(b)*r, -helperLength);
            p.insert(p.end(), {p0.x,p0.y,p0.z,p1.x,p1.y,p1.z});
            if (i % 8 == 0) p.insert(p.end(), {apex.x,apex.y,apex.z,p0.x,p0.y,p0.z});
        }
        p.insert(p.end(), {apex.x,apex.y,apex.z,center.x,center.y,center.z});
        g->setAttribute("position", BufferAttribute::fromVector(p, 3, AttributeType::Float32));
        geometry = g;
        auto m = std::dynamic_pointer_cast<LineBasicMaterial>(material);
        if (!m) {
            m = std::make_shared<LineBasicMaterial>();
            material = m;
        }
        m->color = light.color;
        kind = ObjectKind::LineSegments;
    }

    void update(const SpotLight& light) {
        position = light.position;
        // The helper cone is authored along local -Z, matching three.js SpotLightHelper.
        // lookAt() also orients local -Z toward the target, so this keeps the helper
        // aligned with the actual SpotLight shadow/view direction.
        lookAt(light.target);
    }

private:
    float helperLength = 4.0f;
    int helperSegments = 32;
};

class VertexNormalsHelper : public LineSegments {
protected:
    VertexNormalsHelper() = default;
public:
    explicit VertexNormalsHelper(const RenderableObject& object, float size = 0.2f, const glm::vec3& color = glm::vec3(0.2f, 0.6f, 1.0f)) {
        build(object, "normal", size, color);
    }
protected:
    void build(const RenderableObject& object, const std::string& attrName, float size, const glm::vec3& color) {
        auto g = std::make_shared<BufferGeometry>();
        std::vector<float> p;
        if (object.geometry) {
            const auto* pos = object.geometry->getAttribute("position");
            const auto* vec = object.geometry->getAttribute(attrName);
            if (pos && vec && pos->type == AttributeType::Float32 && vec->type == AttributeType::Float32) {
                auto ps = pos->asSpan<float>();
                auto ns = vec->asSpan<float>();
                int count = std::min(pos->count, vec->count);
                for (int i = 0; i < count; ++i) {
                    glm::vec3 a(ps[i*pos->itemSize], ps[i*pos->itemSize+1], ps[i*pos->itemSize+2]);
                    glm::vec3 n(ns[i*vec->itemSize], ns[i*vec->itemSize+1], ns[i*vec->itemSize+2]);
                    if (glm::length(n) > 1e-6f) n = glm::normalize(n);
                    glm::vec3 b = a + n * size;
                    p.insert(p.end(), {a.x,a.y,a.z,b.x,b.y,b.z});
                }
            }
        }
        g->setAttribute("position", BufferAttribute::fromVector(p, 3, AttributeType::Float32));
        geometry = g;
        auto m = std::make_shared<LineBasicMaterial>();
        m->color = color;
        material = m;
        kind = ObjectKind::LineSegments;
    }
};

class VertexTangentsHelper : public VertexNormalsHelper {
public:
    explicit VertexTangentsHelper(const RenderableObject& object, float size = 0.2f, const glm::vec3& color = glm::vec3(1.0f, 0.55f, 0.15f))
        : VertexNormalsHelper() {
        build(object, "tangent", size, color);
    }
};

} // namespace THREE
