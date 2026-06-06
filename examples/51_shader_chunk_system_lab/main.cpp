#include "shader/ShaderChunk.hpp"
#include "shader/ShaderLib.hpp"
#include <iostream>

using namespace threecpp;

static const char* stageName(ShaderStage s) {
    switch (s) {
        case ShaderStage::Vertex: return "vertex";
        case ShaderStage::Fragment: return "fragment";
        case ShaderStage::Both: return "both";
    }
    return "unknown";
}

int main() {
    std::cout << "threecpp v5.8 shader chunk system lab\n";
    std::cout << "registered chunks:\n";
    for (const auto& chunk : ShaderChunk::registry()) {
        std::cout << "  - " << chunk.name << " [" << stageName(chunk.stage) << "] " << chunk.description << "\n";
    }

    ProgramKey key{};
    key.materialType = MaterialType::MeshPhysical;
    key.useMap = true;
    key.useNormalMap = true;
    key.useRoughnessMap = true;
    key.useMetalnessMap = true;
    key.useAOMap = true;
    key.useIBL = true;
    key.usePMREM = true;
    key.useShadowMap = true;
    key.usePhysical = true;
    key.useClearcoat = true;
    key.useSheen = true;
    key.useTransmission = true;
    key.useTransmissionRenderTarget = true;
    key.useInstancing = true;
    key.useInstanceColor = true;

    ShaderSource src = ShaderLib::build(key);
    std::cout << "\nphysical shader generated:\n";
    std::cout << "  vertex bytes:   " << src.vertex.size() << "\n";
    std::cout << "  fragment bytes: " << src.fragment.size() << "\n";
    std::cout << "  contains <chunk:pbr_math>: " << (src.fragment.find("<chunk:pbr_math>") != std::string::npos ? "yes" : "no") << "\n";
    std::cout << "  contains <chunk:shadow>:   " << (src.fragment.find("<chunk:shadow>") != std::string::npos ? "yes" : "no") << "\n";
    std::cout << "  contains <chunk:ibl>:      " << (src.fragment.find("<chunk:ibl>") != std::string::npos ? "yes" : "no") << "\n";
    std::cout << "  contains <chunk:physical>: " << (src.fragment.find("<chunk:physical>") != std::string::npos ? "yes" : "no") << "\n";
    return 0;
}
