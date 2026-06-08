#pragma once
#include "geometry/buffer-geometry.h"

namespace THREE {

class FatLineGeometry {
public:
    // three.js Line2/LineSegments2 style geometry.
    // Each segment is rendered as two triangles. The vertex shader performs screen-space extrusion
    // from instanceStart/instanceEnd using FatLineMaterial::linewidth and viewport resolution.
    static std::shared_ptr<BufferGeometry> fromPolyline(const std::vector<glm::vec3>& points, float ignoredObjectSpaceWidth = 0.0f, bool closed = false);
};

} // namespace THREE
