#pragma once
#include "common.h"
#include "platform/gl-headers.h"

namespace THREE {

struct TextureUnitAllocation {
    int unit = -1;
    bool cacheHit = false;
};

struct TextureUnitAllocatorStats {
    int maxUnits = 0;
    int activeUnits = 0;
    int requests = 0;
    int hits = 0;
    int misses = 0;
    int evictions = 0;
};

// WebGLRenderer-style texture unit allocator.
// It keeps stable texture->unit bindings within a frame, avoids redundant
// glActiveTexture/glBindTexture, and exposes diagnostics for large scenes.
class TextureUnitAllocator {
public:
    struct Binding {
        std::uint64_t textureId = 0;
        std::uint64_t textureVersion = 0;
        GLenum target = GL_TEXTURE_2D;
        GLuint glTexture = 0;
        int lastFrameUsed = -1;
    };

    TextureUnitAllocator() { queryMaxUnits(); }

    void beginFrame(int frameIndex) {
        currentFrame = frameIndex;
        stats.activeUnits = 0;
        for (const auto& b : units) if (b.glTexture != 0) ++stats.activeUnits;
    }

    void reset() {
        units.clear();
        textureToUnit.clear();
        currentActiveUnit = -1;
        queryMaxUnits();
        stats = {};
        stats.maxUnits = maxUnits;
    }

    TextureUnitAllocation bind(std::uint64_t textureId,
                               std::uint64_t textureVersion,
                               GLenum target,
                               GLuint glTexture) {
        ++stats.requests;
        if (maxUnits <= 0) queryMaxUnits();
        if (units.empty()) units.resize(static_cast<std::size_t>(std::max(1, maxUnits)));

        const auto key = makeKey(textureId, target);
        auto it = textureToUnit.find(key);
        if (it != textureToUnit.end()) {
            const int unit = it->second;
            if (unit >= 0 && unit < static_cast<int>(units.size())) {
                Binding& b = units[static_cast<std::size_t>(unit)];
                if (b.textureId == textureId && b.textureVersion == textureVersion && b.target == target && b.glTexture == glTexture) {
                    ++stats.hits;
                    b.lastFrameUsed = currentFrame;
                    activateAndBindIfNeeded(unit, target, glTexture);
                    return {unit, true};
                }
            }
        }

        ++stats.misses;
        const int unit = acquireUnit();
        Binding& slot = units[static_cast<std::size_t>(unit)];
        if (slot.glTexture != 0) {
            textureToUnit.erase(makeKey(slot.textureId, slot.target));
            ++stats.evictions;
        }
        slot.textureId = textureId;
        slot.textureVersion = textureVersion;
        slot.target = target;
        slot.glTexture = glTexture;
        slot.lastFrameUsed = currentFrame;
        textureToUnit[key] = unit;
        activateAndBindIfNeeded(unit, target, glTexture, true);
        stats.activeUnits = std::max(stats.activeUnits, unit + 1);
        return {unit, false};
    }

    const TextureUnitAllocatorStats& getStats() const { return stats; }
    int getMaxUnits() const { return maxUnits; }

private:
    int maxUnits = 0;
    int currentFrame = 0;
    int currentActiveUnit = -1;
    std::vector<Binding> units;
    std::unordered_map<std::uint64_t, int> textureToUnit;
    TextureUnitAllocatorStats stats;

    static std::uint64_t makeKey(std::uint64_t textureId, GLenum target) {
        return (textureId << 16u) ^ static_cast<std::uint64_t>(target & 0xffffu);
    }

    void queryMaxUnits() {
        GLint value = 16;
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &value);
        maxUnits = std::max(1, static_cast<int>(value));
        if (units.empty()) units.resize(static_cast<std::size_t>(maxUnits));
        stats.maxUnits = maxUnits;
    }

    int acquireUnit() const {
        for (int i = 0; i < static_cast<int>(units.size()); ++i) {
            if (units[static_cast<std::size_t>(i)].glTexture == 0) return i;
        }
        int best = 0;
        int oldest = units[0].lastFrameUsed;
        for (int i = 1; i < static_cast<int>(units.size()); ++i) {
            if (units[static_cast<std::size_t>(i)].lastFrameUsed < oldest) {
                oldest = units[static_cast<std::size_t>(i)].lastFrameUsed;
                best = i;
            }
        }
        return best;
    }

    void activateAndBindIfNeeded(int unit, GLenum target, GLuint texture, bool force = false) {
        if (currentActiveUnit != unit) {
            glActiveTexture(GL_TEXTURE0 + unit);
            currentActiveUnit = unit;
        }
        if (force) {
            glBindTexture(target, texture);
            return;
        }
        const auto& b = units[static_cast<std::size_t>(unit)];
        if (b.glTexture != texture || b.target != target) glBindTexture(target, texture);
    }
};

} // namespace THREE
