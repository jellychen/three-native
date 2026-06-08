#pragma once
#include "common.h"
#include "core/renderable.h"
#include "animation/animation.h"
#include <sstream>
#include <unordered_set>

namespace THREE {

struct GltfValidationStats {
    int objects = 0;
    int meshes = 0;
    int skinnedMeshes = 0;
    int materials = 0;
    int textures = 0;
    int skeletons = 0;
    int bones = 0;
    int morphTargetObjects = 0;
    int morphTargets = 0;
    int animations = 0;
    int animationTracks = 0;
    int lights = 0;
};

struct GltfValidationOptions {
    bool countUniqueMaterials = true;
    bool countUniqueTextures = true;
    bool printPerObject = false;
};

class GltfValidationSuite {
public:
    GltfValidationSuite() = default;

    GltfValidationStats inspect(Object3D& root,
                                const std::vector<AnimationClip>& animations = {},
                                const GltfValidationOptions& options = {}) {
        stats = {};
        materialIds.clear();
        textureIds.clear();
        stats.animations = static_cast<int>(animations.size());
        for (const auto& clip : animations) stats.animationTracks += static_cast<int>(clip.tracks.size());
        root.traverse([&](Object3D& object) {
            ++stats.objects;
            if (object.kind == ObjectKind::Light) ++stats.lights;
            if (auto* mesh = dynamic_cast<Mesh*>(&object)) {
                ++stats.meshes;
                inspectMesh(*mesh, options);
            }
            if (auto* skinned = dynamic_cast<SkinnedMesh*>(&object)) {
                ++stats.skinnedMeshes;
                if (skinned->skeleton) {
                    ++stats.skeletons;
                    stats.bones += static_cast<int>(skinned->skeleton->bones.size());
                }
            }
        });
        if (options.countUniqueMaterials) stats.materials = static_cast<int>(materialIds.size());
        if (options.countUniqueTextures) stats.textures = static_cast<int>(textureIds.size());
        return stats;
    }

    static std::string summary(const GltfValidationStats& s) {
        std::ostringstream out;
        out << "objects=" << s.objects
            << " meshes=" << s.meshes
            << " skinnedMeshes=" << s.skinnedMeshes
            << " materials=" << s.materials
            << " textures=" << s.textures
            << " skeletons=" << s.skeletons
            << " bones=" << s.bones
            << " morphObjects=" << s.morphTargetObjects
            << " morphTargets=" << s.morphTargets
            << " animations=" << s.animations
            << " animationTracks=" << s.animationTracks
            << " lights=" << s.lights;
        return out.str();
    }

private:
    GltfValidationStats stats;
    std::unordered_set<ObjectId> materialIds;
    std::unordered_set<ObjectId> textureIds;

    void inspectMesh(Mesh& mesh, const GltfValidationOptions& options) {
        if (mesh.material) {
            if (options.countUniqueMaterials) materialIds.insert(mesh.material->id);
            collectMaterialTextures(*mesh.material);
        }
        if (mesh.geometry) {
            int morphCount = mesh.geometry->morphTargetCount("position");
            if (morphCount == 0) morphCount = mesh.geometry->morphTargetCount("normal");
            if (morphCount > 0) {
                ++stats.morphTargetObjects;
                stats.morphTargets += morphCount;
            }
        }
    }

    void collect(const std::shared_ptr<Texture>& tex) {
        if (tex) textureIds.insert(tex->id);
    }

    void collectStandard(MeshStandardMaterial& m) {
        collect(m.map); collect(m.alphaMap); collect(m.normalMap); collect(m.bumpMap);
        collect(m.displacementMap); collect(m.roughnessMap); collect(m.metalnessMap);
        collect(m.aoMap); collect(m.lightMap); collect(m.emissiveMap);
    }

    void collectMaterialTextures(Material& mat) {
        if (auto* m = dynamic_cast<MeshBasicMaterial*>(&mat)) {
            collect(m->map); collect(m->alphaMap); collect(m->aoMap); collect(m->lightMap); collect(m->envMap);
        } else if (auto* m = dynamic_cast<MeshPhysicalMaterial*>(&mat)) {
            collectStandard(*m);
            collect(m->transmissionMap); collect(m->thicknessMap); collect(m->specularIntensityMap);
            collect(m->specularColorMap); collect(m->specularMap); collect(m->clearcoatMap);
            collect(m->clearcoatRoughnessMap); collect(m->clearcoatNormalMap); collect(m->sheenColorMap);
            collect(m->sheenRoughnessMap); collect(m->iridescenceMap); collect(m->iridescenceThicknessMap);
            collect(m->anisotropyMap);
        } else if (auto* m = dynamic_cast<MeshStandardMaterial*>(&mat)) {
            collectStandard(*m);
        } else if (auto* m = dynamic_cast<MeshLambertMaterial*>(&mat)) {
            collect(m->map); collect(m->emissiveMap); collect(m->aoMap); collect(m->lightMap);
        } else if (auto* m = dynamic_cast<MeshPhongMaterial*>(&mat)) {
            collect(m->map); collect(m->normalMap); collect(m->specularMap); collect(m->emissiveMap); collect(m->lightMap);
        } else if (auto* m = dynamic_cast<LineBasicMaterial*>(&mat)) {
            collect(m->map);
        } else if (auto* m = dynamic_cast<PointsMaterial*>(&mat)) {
            collect(m->map); collect(m->alphaMap);
        } else if (auto* m = dynamic_cast<SpriteMaterial*>(&mat)) {
            collect(m->map);
        }
    }
};

} // namespace THREE
