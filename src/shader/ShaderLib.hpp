#pragma once
#include "renderer/GLProgram.hpp"

namespace threecpp {

struct ShaderSource { std::string vertex; std::string fragment; };
class ShaderLib {
public:
    static ShaderSource build(const ProgramKey& key);
};

} // namespace threecpp
