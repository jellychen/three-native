#pragma once
#include "geometry/BufferGeometry.hpp"

namespace threecpp::GeometryFactory {

inline std::shared_ptr<BufferGeometry> makePlane(float size = 1.0f) {
    auto g = std::make_shared<BufferGeometry>();
    float s = size * 0.5f;
    std::vector<float> p = {-s,0,-s, s,0,-s, s,0,s, -s,0,s};
    std::vector<float> n = {0,1,0, 0,1,0, 0,1,0, 0,1,0};
    std::vector<float> uv = {0,0, 1,0, 1,1, 0,1};
    g->setAttribute("position", BufferAttribute::fromVector(p, 3, AttributeType::Float32));
    g->setAttribute("normal", BufferAttribute::fromVector(n, 3, AttributeType::Float32));
    g->setAttribute("uv", BufferAttribute::fromVector(uv, 2, AttributeType::Float32));
    g->setAttribute("uv2", BufferAttribute::fromVector(uv, 2, AttributeType::Float32));
    g->indices = {0,1,2, 0,2,3};
    return g;
}

inline std::shared_ptr<BufferGeometry> makeCube(float size = 1.0f) {
    auto g = std::make_shared<BufferGeometry>();
    float s = size * 0.5f;
    std::vector<float> p = {
        -s,-s,s, s,-s,s, s,s,s, -s,s,s,  s,-s,-s, -s,-s,-s, -s,s,-s, s,s,-s,
        -s,s,s, s,s,s, s,s,-s, -s,s,-s, -s,-s,-s, s,-s,-s, s,-s,s, -s,-s,s,
        s,-s,s, s,-s,-s, s,s,-s, s,s,s, -s,-s,-s, -s,-s,s, -s,s,s, -s,s,-s
    };
    std::vector<float> n;
    const glm::vec3 normals[6]={{0,0,1},{0,0,-1},{0,1,0},{0,-1,0},{1,0,0},{-1,0,0}};
    for (auto no: normals) for(int i=0;i<4;i++){ n.push_back(no.x); n.push_back(no.y); n.push_back(no.z); }
    std::vector<float> uv;
    for (int f = 0; f < 6; ++f) uv.insert(uv.end(), {0,0, 1,0, 1,1, 0,1});
    std::vector<uint32_t> idx;
    for(uint32_t f=0; f<6; ++f){ uint32_t o=f*4; idx.insert(idx.end(), {o,o+1,o+2,o,o+2,o+3}); }
    g->setAttribute("position", BufferAttribute::fromVector(p, 3, AttributeType::Float32));
    g->setAttribute("normal", BufferAttribute::fromVector(n, 3, AttributeType::Float32));
    g->setAttribute("uv", BufferAttribute::fromVector(uv, 2, AttributeType::Float32));
    g->setAttribute("uv2", BufferAttribute::fromVector(uv, 2, AttributeType::Float32));
    g->indices = idx;
    return g;
}

inline std::shared_ptr<BufferGeometry> makeUVSphere(float radius = 1.0f, int widthSegments = 48, int heightSegments = 24) {
    auto g = std::make_shared<BufferGeometry>();
    widthSegments = std::max(3, widthSegments);
    heightSegments = std::max(2, heightSegments);
    std::vector<float> p, n, uv;
    std::vector<uint32_t> idx;
    for (int y = 0; y <= heightSegments; ++y) {
        float v = float(y) / float(heightSegments);
        float theta = v * glm::pi<float>();
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);
        for (int x = 0; x <= widthSegments; ++x) {
            float u = float(x) / float(widthSegments);
            float phi = u * glm::two_pi<float>();
            glm::vec3 normal{std::sin(phi) * sinTheta, cosTheta, std::cos(phi) * sinTheta};
            glm::vec3 pos = normal * radius;
            p.insert(p.end(), {pos.x, pos.y, pos.z});
            n.insert(n.end(), {normal.x, normal.y, normal.z});
            uv.insert(uv.end(), {u, 1.0f - v});
        }
    }
    for (int y = 0; y < heightSegments; ++y) {
        for (int x = 0; x < widthSegments; ++x) {
            uint32_t a = uint32_t(y * (widthSegments + 1) + x);
            uint32_t b = a + uint32_t(widthSegments + 1);
            uint32_t c = b + 1;
            uint32_t d = a + 1;
            if (y != 0) idx.insert(idx.end(), {a, b, d});
            if (y != heightSegments - 1) idx.insert(idx.end(), {d, b, c});
        }
    }
    g->setAttribute("position", BufferAttribute::fromVector(p, 3, AttributeType::Float32));
    g->setAttribute("normal", BufferAttribute::fromVector(n, 3, AttributeType::Float32));
    g->setAttribute("uv", BufferAttribute::fromVector(uv, 2, AttributeType::Float32));
    g->setAttribute("uv2", BufferAttribute::fromVector(uv, 2, AttributeType::Float32));
    g->indices = std::move(idx);
    g->computeBoundingSphere();
    return g;
}


// Compatibility alias: three.js-style SphereGeometry naming.
// Existing implementation is UV sphere based.
inline std::shared_ptr<BufferGeometry> makeSphere(float radius = 1.0f, int widthSegments = 48, int heightSegments = 24) {
    return makeUVSphere(radius, widthSegments, heightSegments);
}

inline std::shared_ptr<BufferGeometry> makeGrid(int divisions = 10, float size = 10.0f) {
    auto g = std::make_shared<BufferGeometry>();
    std::vector<float> p;
    float half = size * 0.5f;
    for (int i = 0; i <= divisions; ++i) {
        float t = -half + size * float(i) / float(divisions);
        p.insert(p.end(), {-half,0,t, half,0,t, t,0,-half, t,0,half});
    }
    g->setAttribute("position", BufferAttribute::fromVector(p, 3, AttributeType::Float32));
    return g;
}

} // namespace threecpp::GeometryFactory

namespace threecpp::GeometryFactory {

inline std::shared_ptr<BufferGeometry> makePlaneSegments(float width = 1.0f, float height = 1.0f, int widthSegments = 1, int heightSegments = 1) {
    auto g = std::make_shared<BufferGeometry>();
    widthSegments = std::max(1, widthSegments);
    heightSegments = std::max(1, heightSegments);
    std::vector<float> p, n, uv;
    std::vector<uint32_t> idx;
    for (int y = 0; y <= heightSegments; ++y) {
        float v = float(y) / float(heightSegments);
        float py = (v - 0.5f) * height;
        for (int x = 0; x <= widthSegments; ++x) {
            float u = float(x) / float(widthSegments);
            float px = (u - 0.5f) * width;
            p.insert(p.end(), {px, 0.0f, py});
            n.insert(n.end(), {0.0f, 1.0f, 0.0f});
            uv.insert(uv.end(), {u, 1.0f - v});
        }
    }
    for (int y = 0; y < heightSegments; ++y) {
        for (int x = 0; x < widthSegments; ++x) {
            uint32_t a = uint32_t(y * (widthSegments + 1) + x);
            uint32_t b = a + 1;
            uint32_t c = a + uint32_t(widthSegments + 1);
            uint32_t d = c + 1;
            idx.insert(idx.end(), {a, c, b, b, c, d});
        }
    }
    g->setAttribute("position", BufferAttribute::fromVector(p, 3, AttributeType::Float32));
    g->setAttribute("normal", BufferAttribute::fromVector(n, 3, AttributeType::Float32));
    g->setAttribute("uv", BufferAttribute::fromVector(uv, 2, AttributeType::Float32));
    g->setAttribute("uv2", BufferAttribute::fromVector(uv, 2, AttributeType::Float32));
    g->indices = std::move(idx);
    g->computeBoundingSphere();
    return g;
}

inline std::shared_ptr<BufferGeometry> makeCylinder(float radiusTop = 1.0f, float radiusBottom = 1.0f, float height = 2.0f, int radialSegments = 32, int heightSegments = 1, bool capped = true) {
    auto g = std::make_shared<BufferGeometry>();
    radialSegments = std::max(3, radialSegments);
    heightSegments = std::max(1, heightSegments);
    std::vector<float> p, n, uv;
    std::vector<uint32_t> idx;
    for (int y = 0; y <= heightSegments; ++y) {
        float v = float(y) / float(heightSegments);
        float radius = glm::mix(radiusTop, radiusBottom, v);
        float py = height * (0.5f - v);
        for (int x = 0; x <= radialSegments; ++x) {
            float u = float(x) / float(radialSegments);
            float phi = u * glm::two_pi<float>();
            glm::vec3 normal{std::sin(phi), 0.0f, std::cos(phi)};
            p.insert(p.end(), {normal.x * radius, py, normal.z * radius});
            n.insert(n.end(), {normal.x, normal.y, normal.z});
            uv.insert(uv.end(), {u, 1.0f - v});
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
    auto addCap = [&](bool top) {
        float py = top ? height * 0.5f : -height * 0.5f;
        float radius = top ? radiusTop : radiusBottom;
        uint32_t center = uint32_t(p.size() / 3);
        p.insert(p.end(), {0.0f, py, 0.0f});
        n.insert(n.end(), {0.0f, top ? 1.0f : -1.0f, 0.0f});
        uv.insert(uv.end(), {0.5f, 0.5f});
        for (int x = 0; x <= radialSegments; ++x) {
            float u = float(x) / float(radialSegments);
            float phi = u * glm::two_pi<float>();
            float sx = std::sin(phi), cz = std::cos(phi);
            uint32_t vi = uint32_t(p.size() / 3);
            p.insert(p.end(), {sx * radius, py, cz * radius});
            n.insert(n.end(), {0.0f, top ? 1.0f : -1.0f, 0.0f});
            uv.insert(uv.end(), {sx * 0.5f + 0.5f, cz * 0.5f + 0.5f});
            if (x > 0) {
                if (top) idx.insert(idx.end(), {center, vi - 1, vi});
                else idx.insert(idx.end(), {center, vi, vi - 1});
            }
        }
    };
    if (capped) { addCap(true); addCap(false); }
    g->setAttribute("position", BufferAttribute::fromVector(p, 3, AttributeType::Float32));
    g->setAttribute("normal", BufferAttribute::fromVector(n, 3, AttributeType::Float32));
    g->setAttribute("uv", BufferAttribute::fromVector(uv, 2, AttributeType::Float32));
    g->setAttribute("uv2", BufferAttribute::fromVector(uv, 2, AttributeType::Float32));
    g->indices = std::move(idx);
    g->computeBoundingSphere();
    return g;
}

inline std::shared_ptr<BufferGeometry> makeTorus(float radius = 1.0f, float tube = 0.35f, int radialSegments = 48, int tubularSegments = 16) {
    auto g = std::make_shared<BufferGeometry>();
    radialSegments = std::max(3, radialSegments);
    tubularSegments = std::max(3, tubularSegments);
    std::vector<float> p, n, uv;
    std::vector<uint32_t> idx;
    for (int j = 0; j <= radialSegments; ++j) {
        float u = float(j) / float(radialSegments) * glm::two_pi<float>();
        glm::vec3 center{std::cos(u) * radius, 0.0f, std::sin(u) * radius};
        for (int i = 0; i <= tubularSegments; ++i) {
            float v = float(i) / float(tubularSegments) * glm::two_pi<float>();
            glm::vec3 normal{std::cos(u) * std::cos(v), std::sin(v), std::sin(u) * std::cos(v)};
            glm::vec3 pos = center + normal * tube;
            p.insert(p.end(), {pos.x, pos.y, pos.z});
            n.insert(n.end(), {normal.x, normal.y, normal.z});
            uv.insert(uv.end(), {float(j) / radialSegments, float(i) / tubularSegments});
        }
    }
    for (int j = 0; j < radialSegments; ++j) {
        for (int i = 0; i < tubularSegments; ++i) {
            uint32_t a = uint32_t(j * (tubularSegments + 1) + i);
            uint32_t b = uint32_t((j + 1) * (tubularSegments + 1) + i);
            uint32_t c = b + 1;
            uint32_t d = a + 1;
            idx.insert(idx.end(), {a, b, d, d, b, c});
        }
    }
    g->setAttribute("position", BufferAttribute::fromVector(p, 3, AttributeType::Float32));
    g->setAttribute("normal", BufferAttribute::fromVector(n, 3, AttributeType::Float32));
    g->setAttribute("uv", BufferAttribute::fromVector(uv, 2, AttributeType::Float32));
    g->setAttribute("uv2", BufferAttribute::fromVector(uv, 2, AttributeType::Float32));
    g->indices = std::move(idx);
    g->computeBoundingSphere();
    return g;
}

} // namespace threecpp::GeometryFactory
