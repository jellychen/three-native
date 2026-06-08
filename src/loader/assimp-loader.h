#pragma once
#include "core/scene.h"
#include "core/renderable.h"
#include "animation/animation.h"
#include "asset/gltf-extensions.h"
#include <assimp/scene.h>

namespace THREE {

struct AssimpLoaderOptions {
    bool triangulate = true;
    bool generateNormals = true;
    bool generateTangents = true;
    bool flipUVs = false;
    bool preferBasicMaterial = false;
    bool loadTextures = true;
    bool loadEmbeddedTextures = true;
    bool loadSkinning = true;
    bool loadMorphTargets = true;
    bool loadAnimations = true;
    bool optimizeMeshes = true;
    bool validateData = true;
    bool preserveCompressedTextures = true;
    bool inspectGltfExtensions = true;
};

struct AssimpLoadResult {
    std::shared_ptr<Object3D> root;
    std::vector<AnimationClip> animations;
    std::string format;
    bool hasMeshes = false;
    bool hasSkins = false;
    bool hasAnimations = false;
    GltfExtensionsReport extensionReport;
};

class AssimpLoader {
    AssimpLoaderOptions options;
    std::filesystem::path baseDir;
    std::unordered_map<std::string, Object3D*> nodeObjects;
    std::unordered_map<std::string, Bone*> boneObjects;
    std::vector<std::string> boneNodeNames;
    std::vector<AnimationClip> importedAnimations;
    std::string importedFormat;
    GltfExtensionsReport extensionReport;

    struct GltfMaterialOverride {
        bool valid = false;
        bool hasTransmission = false;
        float transmission = 0.0f;
        bool hasIor = false;
        float ior = 1.5f;
        bool hasVolume = false;
        float thickness = 0.0f;
        float attenuationDistance = std::numeric_limits<float>::infinity();
        glm::vec3 attenuationColor{1.0f};
        std::shared_ptr<Texture> thicknessMap;
        bool hasDispersion = false;
        float dispersion = 0.0f;
    };
    std::vector<GltfMaterialOverride> gltfMaterialOverrides;

    struct PendingSkin {
        SkinnedMesh* mesh = nullptr;
        std::vector<std::string> boneNames;
        std::vector<glm::mat4> boneInverses;
    };
    std::vector<PendingSkin> pendingSkins;

public:
    explicit AssimpLoader(AssimpLoaderOptions opts = {}) : options(opts) {}

    static bool isSupportedExtension(std::string_view ext);
    static std::vector<std::string> supportedExtensions();

    std::shared_ptr<Object3D> load(const std::filesystem::path& file);
    AssimpLoadResult loadResult(const std::filesystem::path& file);

private:
    void resetImportState();
    void collectBoneNodeNames(const aiScene* scene);
    std::shared_ptr<Object3D> convertNode(const aiScene* scene, aiNode* node);
    std::shared_ptr<Mesh> convertMesh(const aiScene* scene, aiMesh* mesh);
    std::shared_ptr<Material> convertMaterial(const aiScene* scene, aiMaterial* mat, unsigned materialIndex = 0);
    void parseGltfMaterialOverrides(const std::filesystem::path& file);
    std::shared_ptr<Texture> loadMaterialTexture(const aiScene* scene, aiMaterial* mat, aiTextureType type, unsigned index = 0);
    void resolvePendingSkins();
    void convertAnimations(const aiScene* scene);
    void inspectSceneExtensions(const aiScene* scene);
    void inspectMaterialExtensions(aiMaterial* mat);
    void updateTextureExtensionMetadata(Texture& tex, const std::string& rawPath);
};

} // namespace THREE
