#include "geometry/FatLineGeometry.hpp"

namespace threecpp {

std::shared_ptr<BufferGeometry> FatLineGeometry::fromPolyline(const std::vector<glm::vec3>& points, float, bool closed) {
    auto g = make_ref<BufferGeometry>();
    if (points.size() < 2) return g;

    std::vector<float> control;       // loc0: x chooses start/end, y is side -1/+1
    std::vector<float> instanceStart; // loc1
    std::vector<float> instanceEnd;   // loc2
    std::vector<float> counters;      // loc6, reserved for dashed lines later
    std::vector<std::uint32_t> indices;

    auto emit = [&](const glm::vec3& a, const glm::vec3& b, float counter0, float counter1) {
        const std::uint32_t base = static_cast<std::uint32_t>(control.size() / 3);
        const float ctrl[12] = {
            -1.0f, -1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f,
             1.0f, -1.0f, 0.0f,
             1.0f,  1.0f, 0.0f,
        };
        control.insert(control.end(), std::begin(ctrl), std::end(ctrl));
        for (int i = 0; i < 4; ++i) {
            instanceStart.insert(instanceStart.end(), {a.x, a.y, a.z});
            instanceEnd.insert(instanceEnd.end(), {b.x, b.y, b.z});
        }
        counters.insert(counters.end(), {counter0, counter0, counter1, counter1});
        indices.insert(indices.end(), {base + 0, base + 2, base + 1, base + 2, base + 3, base + 1});
    };

    float accumulated = 0.0f;
    for (std::size_t i = 0; i + 1 < points.size(); ++i) {
        float next = accumulated + glm::length(points[i + 1] - points[i]);
        emit(points[i], points[i + 1], accumulated, next);
        accumulated = next;
    }
    if (closed) {
        float next = accumulated + glm::length(points.front() - points.back());
        emit(points.back(), points.front(), accumulated, next);
    }

    g->setAttribute("position", BufferAttribute::fromVector(control, 3, AttributeType::Float32));
    g->setAttribute("instanceStart", BufferAttribute::fromVector(instanceStart, 3, AttributeType::Float32));
    g->setAttribute("instanceEnd", BufferAttribute::fromVector(instanceEnd, 3, AttributeType::Float32));
    g->setAttribute("lineDistance", BufferAttribute::fromVector(counters, 1, AttributeType::Float32));
    g->setIndex(std::span<const std::uint32_t>(indices.data(), indices.size()));
    g->computeBoundingSphere();
    return g;
}

} // namespace threecpp
