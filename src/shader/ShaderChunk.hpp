#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "renderer/GLProgram.hpp"

namespace threecpp {

enum class ShaderStage {
    Vertex,
    Fragment,
    Both
};

struct ShaderChunkInfo {
    std::string_view name;
    ShaderStage stage;
    std::string_view description;
};

class ShaderChunk {
public:
    static std::vector<ShaderChunkInfo> registry();

    // GLSL version and precision header.
    static std::string versionHeader();

    // ProgramKey -> #define block. This replaces scattered ad-hoc define
    // creation and is the first step toward a three.js-style shader library.
    static std::string defines(const ProgramKey& key);

    // Shared chunks. These are deliberately small and stable so the monolithic
    // ShaderLib can be migrated section by section without changing rendering
    // behavior in one large risky patch.
    static std::string common();
    static std::string colorSpace();
    static std::string toneMapping();
    static std::string packing();
    static std::string pbrMath();
    static std::string ibl();
    static std::string shadow();
    static std::string fog();
    static std::string normalPerturb();
    static std::string physical();
    static std::string fragmentCore();

    // Source comments make shader compile errors much easier to map back to the
    // generated chunks while keeping GLSL compatible with desktop GL and GLES.
    static std::string begin(std::string_view name);
    static std::string end(std::string_view name);
};

class ShaderBuilder {
public:
    explicit ShaderBuilder(const ProgramKey& key);

    ShaderBuilder& addChunk(std::string_view name, const std::string& source);
    ShaderBuilder& addRaw(const std::string& source);
    std::string str() const;

private:
    std::string source_;
};

} // namespace threecpp
