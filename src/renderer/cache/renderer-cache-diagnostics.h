#pragma once
#include "renderer/program-cache.h"
#include "renderer/gl-binding-states.h"
#include "renderer/cache/texture-unit-allocator.h"
#include "renderer/cache/web-gl-state-cache.h"
#include "renderer/cache/render-list-persistent-cache.h"

namespace THREE {

struct RendererCacheDiagnostics {
    ProgramCacheStats programCache;
    TextureUnitAllocatorStats textureUnits;
    WebGLStateCacheStats stateCache;
    RenderListPersistentCacheStats renderListCache;
    int geometryCacheEntries = 0;
    int materialVersionedPrograms = 0;

    std::string toString() const {
        std::ostringstream ss;
        ss << "Renderer cache diagnostics\n";
        ss << "  programs: live=" << programCache.livePrograms << " hits=" << programCache.hits << " misses=" << programCache.misses << "\n";
        ss << "  texture units: max=" << textureUnits.maxUnits << " active=" << textureUnits.activeUnits << " hits=" << textureUnits.hits << " misses=" << textureUnits.misses << " evictions=" << textureUnits.evictions << "\n";
        ss << "  state changes=" << stateCache.stateChanges << " redundant(program=" << stateCache.redundantProgram << ", blend=" << stateCache.redundantBlend << ", depthTest=" << stateCache.redundantDepthTest << ")\n";
        ss << "  render list: entries=" << renderListCache.liveEntries << " hits=" << renderListCache.hits << " misses=" << renderListCache.misses << " invalidations=" << renderListCache.invalidations << "\n";
        ss << "  geometry cache entries=" << geometryCacheEntries << "\n";
        return ss.str();
    }
};

} // namespace THREE
