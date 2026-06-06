#pragma once
#include "renderer/GLProgram.hpp"
#include "shader/ShaderLib.hpp"

namespace threecpp {

struct ProgramCacheStats {
    int livePrograms = 0;
    int hits = 0;
    int misses = 0;
};

class ProgramCache {
    std::unordered_map<ProgramKey, std::unique_ptr<GLProgram>, ProgramKeyHash> programs;
    mutable ProgramCacheStats stats;
public:
    GLProgram& get(const ProgramKey& key) {
        auto it = programs.find(key);
        if (it != programs.end()) { ++stats.hits; return *it->second; }
        ShaderSource src = ShaderLib::build(key);
        auto program = std::make_unique<GLProgram>(key, src.vertex, src.fragment);
        auto [inserted, _] = programs.emplace(key, std::move(program));
        ++stats.misses;
        stats.livePrograms = static_cast<int>(programs.size());
        return *inserted->second;
    }
    void clear() { programs.clear(); stats.livePrograms = 0; }
    int size() const { return static_cast<int>(programs.size()); }
    ProgramCacheStats getStats() const { auto s = stats; s.livePrograms = static_cast<int>(programs.size()); return s; }
};

} // namespace threecpp
