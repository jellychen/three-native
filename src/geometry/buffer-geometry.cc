#include "geometry/buffer-geometry.h"
#include <random>

namespace THREE {

bool BufferGeometry::hasAttribute(std::string_view name) const { return attributes.find(std::string(name)) != attributes.end(); }
BufferAttribute* BufferGeometry::getAttribute(std::string_view name) { auto it = attributes.find(std::string(name)); return it == attributes.end() ? nullptr : &it->second; }
const BufferAttribute* BufferGeometry::getAttribute(std::string_view name) const { auto it = attributes.find(std::string(name)); return it == attributes.end() ? nullptr : &it->second; }
void BufferGeometry::markNeedsUpdate() { ++version; morphTargetsNeedUpdate = true; }
void BufferGeometry::clear() { attributes.clear(); morphAttributes.clear(); indices.clear(); groups.clear(); drawRange = {}; boundingBox = {}; boundingSphere = {}; markNeedsUpdate(); }
void BufferGeometry::setAttribute(std::string name, BufferAttribute attr) { attributes[std::move(name)] = std::move(attr); markNeedsUpdate(); }
void BufferGeometry::setMorphAttribute(std::string name, std::span<const BufferAttribute> attrs) { morphAttributes[std::move(name)] = std::vector<BufferAttribute>(attrs.begin(), attrs.end()); markNeedsUpdate(); }
void BufferGeometry::setMorphAttribute(std::string name, std::initializer_list<BufferAttribute> attrs) { setMorphAttribute(std::move(name), std::span<const BufferAttribute>(attrs.begin(), attrs.size())); }
const std::vector<BufferAttribute>* BufferGeometry::getMorphAttributes(std::string_view name) const { auto it = morphAttributes.find(std::string(name)); return it == morphAttributes.end() ? nullptr : &it->second; }
int BufferGeometry::morphTargetCount(std::string_view name) const { auto attrs = getMorphAttributes(name); return attrs ? static_cast<int>(attrs->size()) : 0; }
void BufferGeometry::setIndex(std::span<const std::uint32_t> idx) { indices.assign(idx.begin(), idx.end()); markNeedsUpdate(); }
void BufferGeometry::setIndex(std::initializer_list<std::uint32_t> idx) { setIndex(std::span<const std::uint32_t>(idx.begin(), idx.size())); }

void BufferGeometry::addGroup(int start, int count, int materialIndex) {
    if (count <= 0) return;
    groups.push_back({std::max(0, start), count, std::max(0, materialIndex)});
    markNeedsUpdate();
}

void BufferGeometry::clearGroups() {
    groups.clear();
    markNeedsUpdate();
}

void BufferGeometry::setDrawRange(int start, int count) {
    drawRange.start = std::max(0, start);
    drawRange.count = count < 0 ? 0 : count;
    markNeedsUpdate();
}

int BufferGeometry::vertexCount() const {
    if (const auto* p = getAttribute("position")) return p->count;
    return 0;
}

void BufferGeometry::computeBoundingBox() {
    const auto* position = getAttribute("position");
    if (!position || position->type != AttributeType::Float32 || position->itemSize < 3 || position->count == 0) return;
    auto span = position->asSpan<float>();
    glm::vec3 mn(std::numeric_limits<float>::max());
    glm::vec3 mx(std::numeric_limits<float>::lowest());
    for (int i = 0; i < position->count; ++i) {
        glm::vec3 p(span[i * position->itemSize + 0], span[i * position->itemSize + 1], span[i * position->itemSize + 2]);
        mn = glm::min(mn, p);
        mx = glm::max(mx, p);
    }
    boundingBox = {mn, mx, true};
}


void BufferGeometry::computeLineDistances(bool segments) {
    const auto* position = getAttribute("position");
    if (!position || position->type != AttributeType::Float32 || position->itemSize < 3 || position->count == 0) return;
    auto span = position->asSpan<float>();
    std::vector<float> d(static_cast<std::size_t>(position->count), 0.0f);
    auto pAt = [&](int i) {
        return glm::vec3(span[i * position->itemSize + 0], span[i * position->itemSize + 1], span[i * position->itemSize + 2]);
    };
    if (segments) {
        for (int i = 0; i + 1 < position->count; i += 2) {
            d[static_cast<std::size_t>(i)] = 0.0f;
            d[static_cast<std::size_t>(i + 1)] = glm::length(pAt(i + 1) - pAt(i));
        }
    } else {
        float acc = 0.0f;
        for (int i = 1; i < position->count; ++i) {
            acc += glm::length(pAt(i) - pAt(i - 1));
            d[static_cast<std::size_t>(i)] = acc;
        }
    }
    setAttribute("lineDistance", BufferAttribute::fromVector(d, 1, AttributeType::Float32));
}

void BufferGeometry::computeBoundingSphere() {
    computeBoundingBox();
    if (!boundingBox.valid) return;
    glm::vec3 center = (boundingBox.min + boundingBox.max) * 0.5f;
    const auto* position = getAttribute("position");
    auto span = position->asSpan<float>();
    float r2 = 0.0f;
    for (int i = 0; i < position->count; ++i) {
        glm::vec3 p(span[i * position->itemSize + 0], span[i * position->itemSize + 1], span[i * position->itemSize + 2]);
        r2 = std::max(r2, glm::dot(p - center, p - center));
    }
    boundingSphere = {center, std::sqrt(r2), true};
}

std::shared_ptr<BufferGeometry> BufferGeometry::makeBox(float size) {
    float s = size * 0.5f;
    std::vector<float> positions = {
        -s,-s, s,  s,-s, s,  s, s, s, -s, s, s,
        -s,-s,-s, -s, s,-s,  s, s,-s,  s,-s,-s,
        -s, s,-s, -s, s, s,  s, s, s,  s, s,-s,
        -s,-s,-s,  s,-s,-s,  s,-s, s, -s,-s, s,
         s,-s,-s,  s, s,-s,  s, s, s,  s,-s, s,
        -s,-s,-s, -s,-s, s, -s, s, s, -s, s,-s
    };
    std::vector<float> normals = {
        0,0,1,0,0,1,0,0,1,0,0,1, 0,0,-1,0,0,-1,0,0,-1,0,0,-1,
        0,1,0,0,1,0,0,1,0,0,1,0, 0,-1,0,0,-1,0,0,-1,0,0,-1,0,
        1,0,0,1,0,0,1,0,0,1,0,0, -1,0,0,-1,0,0,-1,0,0,-1,0,0
    };
    std::vector<float> uv = {
        0,0,1,0,1,1,0,1, 0,0,1,0,1,1,0,1, 0,0,1,0,1,1,0,1,
        0,0,1,0,1,1,0,1, 0,0,1,0,1,1,0,1, 0,0,1,0,1,1,0,1
    };
    std::vector<std::uint32_t> idx;
    for (std::uint32_t f = 0; f < 6; ++f) {
        std::uint32_t b = f * 4;
        idx.insert(idx.end(), {b, b+1, b+2, b, b+2, b+3});
    }
    auto g = make_ref<BufferGeometry>();
    g->setAttribute("position", BufferAttribute::fromVector(positions, 3, AttributeType::Float32));
    g->setAttribute("normal", BufferAttribute::fromVector(normals, 3, AttributeType::Float32));
    g->setAttribute("uv", BufferAttribute::fromVector(uv, 2, AttributeType::Float32));
    g->setIndex(std::span<const std::uint32_t>(idx.data(), idx.size()));
    g->computeBoundingSphere();
    return g;
}

std::shared_ptr<BufferGeometry> BufferGeometry::makeLineGrid(int halfExtent, float step) {
    std::vector<float> p;
    for (int i = -halfExtent; i <= halfExtent; ++i) {
        float v = i * step;
        p.insert(p.end(), {-halfExtent*step, 0, v, halfExtent*step, 0, v});
        p.insert(p.end(), {v, 0, -halfExtent*step, v, 0, halfExtent*step});
    }
    auto g = make_ref<BufferGeometry>();
    g->setAttribute("position", BufferAttribute::fromVector(p, 3, AttributeType::Float32));
    g->computeBoundingSphere();
    return g;
}

std::shared_ptr<BufferGeometry> BufferGeometry::makeRandomPoints(int count, float radius) {
    std::mt19937 rng(1234);
    std::uniform_real_distribution<float> dist(-radius, radius);
    std::vector<float> p; p.reserve(count * 3);
    std::vector<float> c; c.reserve(count * 3);
    for (int i = 0; i < count; ++i) {
        p.insert(p.end(), {dist(rng), dist(rng), dist(rng)});
        c.insert(c.end(), {0.5f + 0.5f * dist(rng) / radius, 0.5f + 0.5f * dist(rng) / radius, 1.0f});
    }
    auto g = make_ref<BufferGeometry>();
    g->setAttribute("position", BufferAttribute::fromVector(p, 3, AttributeType::Float32));
    g->setAttribute("color", BufferAttribute::fromVector(c, 3, AttributeType::Float32));
    g->computeBoundingSphere();
    return g;
}

} // namespace THREE
