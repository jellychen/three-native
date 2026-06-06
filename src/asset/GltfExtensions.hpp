#pragma once
#include "common.hpp"
#include "texture/Texture.hpp"
#include "material/Material.hpp"
#include <sstream>

namespace threecpp {

enum class GltfExtensionStatus { Unsupported, MetadataOnly, Imported };

struct GltfExtensionInfo {
    std::string name;
    GltfExtensionStatus status = GltfExtensionStatus::Unsupported;
    std::string note;
};

struct GltfTextureTransformInfo {
    bool present = false;
    TextureTransform transform;
    std::string sourceSlot;
};

struct GltfExtensionsReport {
    std::vector<GltfExtensionInfo> extensions;
    int ktx2Textures = 0;
    int basisPayloads = 0;
    int textureTransforms = 0;
    int physicalMaterials = 0;
    int unsupportedCritical = 0;

    void add(std::string name, GltfExtensionStatus status, std::string note = {}) {
        extensions.push_back({std::move(name), status, std::move(note)});
        if (status == GltfExtensionStatus::Unsupported) ++unsupportedCritical;
    }

    std::string summary() const {
        std::ostringstream out;
        out << "extensions=" << extensions.size()
            << " textureTransforms=" << textureTransforms
            << " ktx2=" << ktx2Textures
            << " basisPayloads=" << basisPayloads
            << " physicalMaterials=" << physicalMaterials
            << " unsupported=" << unsupportedCritical;
        return out.str();
    }
};

inline const char* to_string(GltfExtensionStatus s) {
    switch (s) {
        case GltfExtensionStatus::Imported: return "imported";
        case GltfExtensionStatus::MetadataOnly: return "metadata-only";
        case GltfExtensionStatus::Unsupported: return "unsupported";
    }
    return "unknown";
}

inline void apply_gltf_texture_transform(Texture& texture, const GltfTextureTransformInfo& info) {
    if (!info.present) return;
    texture.applyTextureTransform(info.transform);
}

} // namespace threecpp
