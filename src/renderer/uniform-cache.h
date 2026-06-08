#pragma once
#include "common.h"
#include "platform/gl-headers.h"

namespace THREE {

class UniformCache {
    std::unordered_map<GLint, float> floats;
    std::unordered_map<GLint, glm::vec2> vec2s;
    std::unordered_map<GLint, glm::vec3> vec3s;
    std::unordered_map<GLint, glm::vec4> vec4s;
    std::unordered_map<GLint, int> ints;
public:
    void clear() { floats.clear(); vec2s.clear(); vec3s.clear(); vec4s.clear(); ints.clear(); }
    void uniform1f(GLint loc, float v) { if (loc < 0) return; auto it = floats.find(loc); if (it != floats.end() && it->second == v) return; floats[loc] = v; glUniform1f(loc, v); }
    void uniform1i(GLint loc, int v) { if (loc < 0) return; auto it = ints.find(loc); if (it != ints.end() && it->second == v) return; ints[loc] = v; glUniform1i(loc, v); }
    void uniform2f(GLint loc, const glm::vec2& v) { if (loc < 0) return; auto it = vec2s.find(loc); if (it != vec2s.end() && it->second == v) return; vec2s[loc] = v; glUniform2fv(loc, 1, &v[0]); }
    void uniform3f(GLint loc, const glm::vec3& v) { if (loc < 0) return; auto it = vec3s.find(loc); if (it != vec3s.end() && it->second == v) return; vec3s[loc] = v; glUniform3fv(loc, 1, &v[0]); }
    void uniform4f(GLint loc, const glm::vec4& v) { if (loc < 0) return; auto it = vec4s.find(loc); if (it != vec4s.end() && it->second == v) return; vec4s[loc] = v; glUniform4fv(loc, 1, &v[0]); }
};

} // namespace THREE
