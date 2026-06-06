#pragma once
#include "common.hpp"
#include "core/Renderable.hpp"
#include "animation/Animation.hpp"
#include <sstream>
#include <unordered_set>
#include <map>

namespace threecpp {

struct ImportTextureIssue {
    std::string material;
    std::string slot;
    std::string source;
    std::string message;
};

struct ImportCompatibilityStats {
    int nodes = 0;
    int meshes = 0;
    int skinnedMeshes = 0;
    int materials = 0;
    int standardMaterials = 0;
    int physicalMaterials = 0;
    int textures = 0;
    int embeddedTextures = 0;
    int unresolvedExternalTextures = 0;
    int uv2Meshes = 0;
    int tangentMeshes = 0;
    int vertexColorMeshes = 0;
    int morphMeshes = 0;
    int morphTargets = 0;
    int skeletons = 0;
    int bones = 0;
    int animations = 0;
    int animationTracks = 0;
    int transparentMaterials = 0;
    int doubleSidedMaterials = 0;
    int alphaTestMaterials = 0;
    int transmissionMaterials = 0;
    int clearcoatMaterials = 0;
    int sheenMaterials = 0;
    int iorMaterials = 0;
    int compressedTextures = 0;
    int ktx2Textures = 0;
    int basisTextures = 0;
    int textureTransforms = 0;
    int transcodeReadyTextures = 0;
    int rgbaFallbackTextures = 0;
};

struct ImportCompatibilityReport {
    ImportCompatibilityStats stats;
    std::vector<ImportTextureIssue> textureIssues;
    std::map<std::string, int> materialTypes;

    std::string summary() const {
        std::ostringstream out;
        out << "nodes=" << stats.nodes
            << " meshes=" << stats.meshes
            << " skinnedMeshes=" << stats.skinnedMeshes
            << " materials=" << stats.materials
            << " standard=" << stats.standardMaterials
            << " physical=" << stats.physicalMaterials
            << " textures=" << stats.textures
            << " embeddedTextures=" << stats.embeddedTextures
            << " unresolvedExternalTextures=" << stats.unresolvedExternalTextures
            << " uv2Meshes=" << stats.uv2Meshes
            << " tangentMeshes=" << stats.tangentMeshes
            << " vertexColorMeshes=" << stats.vertexColorMeshes
            << " morphMeshes=" << stats.morphMeshes
            << " morphTargets=" << stats.morphTargets
            << " skeletons=" << stats.skeletons
            << " bones=" << stats.bones
            << " animations=" << stats.animations
            << " animationTracks=" << stats.animationTracks
            << " transparentMaterials=" << stats.transparentMaterials
            << " doubleSided=" << stats.doubleSidedMaterials
            << " alphaTest=" << stats.alphaTestMaterials
            << " transmission=" << stats.transmissionMaterials
            << " clearcoat=" << stats.clearcoatMaterials
            << " sheen=" << stats.sheenMaterials
            << " compressedTextures=" << stats.compressedTextures
            << " ktx2=" << stats.ktx2Textures
            << " basis=" << stats.basisTextures
            << " textureTransforms=" << stats.textureTransforms
            << " transcodeReady=" << stats.transcodeReadyTextures
            << " rgbaFallback=" << stats.rgbaFallbackTextures;
        return out.str();
    }
};

class ImportCompatibilityInspector {
public:
    ImportCompatibilityReport inspect(Object3D& root,
                                      const std::vector<AnimationClip>& animations = {},
                                      bool checkExternalFiles = true) {
        report = {};
        materialIds.clear();
        textureIds.clear();
        report.stats.animations = static_cast<int>(animations.size());
        for (const auto& clip : animations) report.stats.animationTracks += static_cast<int>(clip.tracks.size());
        root.traverse([&](Object3D& o) {
            ++report.stats.nodes;
            if (auto* mesh = dynamic_cast<Mesh*>(&o)) inspectMesh(*mesh, checkExternalFiles);
            if (auto* skinned = dynamic_cast<SkinnedMesh*>(&o)) {
                ++report.stats.skinnedMeshes;
                if (skinned->skeleton) {
                    ++report.stats.skeletons;
                    report.stats.bones += static_cast<int>(skinned->skeleton->bones.size());
                }
            }
        });
        report.stats.materials = static_cast<int>(materialIds.size());
        report.stats.textures = static_cast<int>(textureIds.size());
        return report;
    }

private:
    ImportCompatibilityReport report;
    std::unordered_set<ObjectId> materialIds;
    std::unordered_set<ObjectId> textureIds;

    void inspectMesh(Mesh& mesh, bool checkExternalFiles) {
        ++report.stats.meshes;
        if (mesh.geometry) {
            if (mesh.geometry->hasAttribute("uv2")) ++report.stats.uv2Meshes;
            if (mesh.geometry->hasAttribute("tangent")) ++report.stats.tangentMeshes;
            if (mesh.geometry->hasAttribute("color")) ++report.stats.vertexColorMeshes;
            int morphCount = mesh.geometry->morphTargetCount("position");
            if (morphCount == 0) morphCount = mesh.geometry->morphTargetCount("normal");
            if (morphCount > 0) {
                ++report.stats.morphMeshes;
                report.stats.morphTargets += morphCount;
            }
        }
        if (mesh.material) inspectMaterial(*mesh.material, checkExternalFiles);
    }

    void inspectTexture(const std::string& material, const std::string& slot, const std::shared_ptr<Texture>& tex, bool checkExternalFiles) {
        if (!tex) return;
        textureIds.insert(tex->id);
        if (tex->embedded) ++report.stats.embeddedTextures;
        if (tex->compressed) ++report.stats.compressedTextures;
        if (tex->compressedContainer == CompressedTextureContainer::KTX2) ++report.stats.ktx2Textures;
        if (tex->compressedContainer == CompressedTextureContainer::BasisUniversal) ++report.stats.basisTextures;
        if (tex->hasTextureTransform) ++report.stats.textureTransforms;
        if (tex->compressedUploadReady) ++report.stats.transcodeReadyTextures;
        if (tex->transcodedToRGBA) ++report.stats.rgbaFallbackTextures;
        if (!tex->embedded && checkExternalFiles && !tex->sourcePath.empty() && tex->sourcePath.rfind("embedded://", 0) != 0) {
            if (!std::filesystem::exists(tex->sourcePath)) {
                ++report.stats.unresolvedExternalTextures;
                report.textureIssues.push_back({material, slot, tex->sourcePath, "external texture file not found"});
            }
        }
        if ((slot == "map" || slot == "emissiveMap") && tex->colorSpace != ColorSpace::SRGB) {
            report.textureIssues.push_back({material, slot, tex->sourcePath, "color texture should usually be sRGB"});
        }
        if (tex->compressed) {
            std::string msg = tex->transcodeMessage.empty() ? "compressed texture has KTX2/BasisU payload; GL upload will transcode or use RGBA fallback" : tex->transcodeMessage;
            report.textureIssues.push_back({material, slot, tex->sourcePath, msg});
        }
        if ((slot != "map" && slot != "emissiveMap") && tex->colorSpace != ColorSpace::LinearSRGB) {
            report.textureIssues.push_back({material, slot, tex->sourcePath, "data/scalar texture should usually be linear"});
        }
    }

    void inspectStandard(MeshStandardMaterial& m, bool checkExternalFiles) {
        inspectTexture(m.name, "map", m.map, checkExternalFiles);
        inspectTexture(m.name, "alphaMap", m.alphaMap, checkExternalFiles);
        inspectTexture(m.name, "normalMap", m.normalMap, checkExternalFiles);
        inspectTexture(m.name, "bumpMap", m.bumpMap, checkExternalFiles);
        inspectTexture(m.name, "displacementMap", m.displacementMap, checkExternalFiles);
        inspectTexture(m.name, "roughnessMap", m.roughnessMap, checkExternalFiles);
        inspectTexture(m.name, "metalnessMap", m.metalnessMap, checkExternalFiles);
        inspectTexture(m.name, "aoMap", m.aoMap, checkExternalFiles);
        inspectTexture(m.name, "lightMap", m.lightMap, checkExternalFiles);
        inspectTexture(m.name, "emissiveMap", m.emissiveMap, checkExternalFiles);
    }

    void inspectMaterial(Material& mat, bool checkExternalFiles) {
        materialIds.insert(mat.id);
        if (mat.transparent) ++report.stats.transparentMaterials;
        if (mat.side == Side::DoubleSide) ++report.stats.doubleSidedMaterials;
        if (mat.alphaTest > 0.0f) ++report.stats.alphaTestMaterials;
        report.materialTypes[mat.name.empty() ? "<unnamed>" : mat.name]++;
        if (auto* p = dynamic_cast<MeshPhysicalMaterial*>(&mat)) {
            ++report.stats.physicalMaterials;
            inspectStandard(*p, checkExternalFiles);
            if (p->transmission > 0.0f || p->transmissionMap) ++report.stats.transmissionMaterials;
            if (p->clearcoat > 0.0f || p->clearcoatMap) ++report.stats.clearcoatMaterials;
            if (p->sheen > 0.0f || p->sheenColorMap) ++report.stats.sheenMaterials;
            if (std::abs(p->ior - 1.5f) > 1e-5f) ++report.stats.iorMaterials;
            inspectTexture(p->name, "transmissionMap", p->transmissionMap, checkExternalFiles);
            inspectTexture(p->name, "thicknessMap", p->thicknessMap, checkExternalFiles);
            inspectTexture(p->name, "clearcoatMap", p->clearcoatMap, checkExternalFiles);
            inspectTexture(p->name, "clearcoatRoughnessMap", p->clearcoatRoughnessMap, checkExternalFiles);
            inspectTexture(p->name, "clearcoatNormalMap", p->clearcoatNormalMap, checkExternalFiles);
            inspectTexture(p->name, "sheenColorMap", p->sheenColorMap, checkExternalFiles);
            inspectTexture(p->name, "sheenRoughnessMap", p->sheenRoughnessMap, checkExternalFiles);
            inspectTexture(p->name, "specularColorMap", p->specularColorMap, checkExternalFiles);
            inspectTexture(p->name, "specularIntensityMap", p->specularIntensityMap, checkExternalFiles);
        } else if (auto* s = dynamic_cast<MeshStandardMaterial*>(&mat)) {
            ++report.stats.standardMaterials;
            inspectStandard(*s, checkExternalFiles);
        }
    }
};

} // namespace threecpp
