#pragma once
#include "renderer/gl-program.h"

namespace THREE {

struct ShaderSource { std::string vertex; std::string fragment; };
class ShaderLib {
public:
    static ShaderSource build(const ProgramKey& key);
};

} // namespace THREE
