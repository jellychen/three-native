#include "loader/AssimpLoader.hpp"
#include "texture/TextureLoader.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <cstdlib>
#include <cctype>
#include <set>
#include <map>
#include <fstream>
#if THREECPP_ENABLE_STB_IMAGE
#include <stb_image.h>
#endif

namespace threecpp {
namespace {

static glm::mat4 to_glm(const aiMatrix4x4& m) {
    // aiMatrix4x4 is row-major in field naming. GLM constructor takes columns.
    return glm::mat4(
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4);
}

static glm::vec3 to_glm(const aiVector3D& v) { return {v.x, v.y, v.z}; }
static glm::quat to_glm(const aiQuaternion& q) { return glm::normalize(glm::quat(q.w, q.x, q.y, q.z)); }

static std::string lower_ext(std::filesystem::path p) {
    std::string e = p.extension().string();
    std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    if (!e.empty() && e[0] == '.') e.erase(e.begin());
    return e;
}

static bool get_float(aiMaterial* mat, const char* key, unsigned type, unsigned idx, float& out) {
    return mat && mat->Get(key, type, idx, out) == AI_SUCCESS;
}

static bool get_string(aiMaterial* mat, const char* key, unsigned type, unsigned idx, aiString& out) {
    return mat && mat->Get(key, type, idx, out) == AI_SUCCESS;
}

static bool get_any_float(aiMaterial* mat, float& out, std::initializer_list<const char*> keys) {
    for (const char* k : keys) {
        if (get_float(mat, k, 0, 0, out)) return true;
    }
    return false;
}


static bool get_any_color(aiMaterial* mat, aiColor4D& out, std::initializer_list<const char*> keys) {
    for (const char* k : keys) {
        if (aiGetMaterialColor(mat, k, 0, 0, &out) == AI_SUCCESS) return true;
    }
    return false;
}

static void set_texture_transform_from_gltf_keys(Texture& t, aiMaterial* mat, const char* prefix) {
    if (!mat || !prefix) return;
    float ox = 0.0f, oy = 0.0f, sx = 1.0f, sy = 1.0f, rot = 0.0f;
    bool changed = false;
    std::string base(prefix);
    changed |= get_float(mat, (base + ".offset.x").c_str(), 0, 0, ox);
    changed |= get_float(mat, (base + ".offset.y").c_str(), 0, 0, oy);
    changed |= get_float(mat, (base + ".scale.x").c_str(), 0, 0, sx);
    changed |= get_float(mat, (base + ".scale.y").c_str(), 0, 0, sy);
    changed |= get_float(mat, (base + ".rotation").c_str(), 0, 0, rot);
    if (changed) {
        TextureTransform tr;
        tr.enabled = true;
        tr.offset = {ox, oy};
        tr.scale = {sx == 0.0f ? 1.0f : sx, sy == 0.0f ? 1.0f : sy};
        tr.rotation = rot;
        tr.texCoord = t.uvChannel;
        t.applyTextureTransform(tr);
    }
}

static TextureWrap to_wrap(aiTextureMapMode mode) {
    switch (mode) {
        case aiTextureMapMode_Clamp: return TextureWrap::ClampToEdge;
        case aiTextureMapMode_Mirror: return TextureWrap::MirroredRepeat;
        case aiTextureMapMode_Wrap:
        default: return TextureWrap::Repeat;
    }
}

static void apply_texture_transform(Texture& t, aiMaterial* mat, aiTextureType type, unsigned index) {
    aiUVTransform uv{};
    if (mat && mat->Get(AI_MATKEY_UVTRANSFORM(type, index), uv) == AI_SUCCESS) {
        TextureTransform tr;
        tr.enabled = true;
        tr.offset = {uv.mTranslation.x, uv.mTranslation.y};
        tr.scale = {uv.mScaling.x == 0.0f ? 1.0f : uv.mScaling.x, uv.mScaling.y == 0.0f ? 1.0f : uv.mScaling.y};
        tr.rotation = uv.mRotation;
        tr.texCoord = t.uvChannel;
        t.applyTextureTransform(tr);
    }
}

static bool looks_like_ktx2_or_basis(const std::string& raw) {
    std::string lower = raw;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return lower.find(".ktx2") != std::string::npos || lower.find("image/ktx2") != std::string::npos || lower.find("basis") != std::string::npos;
}

static bool decode_image_bytes(Texture& t, const std::byte* bytes, std::size_t byteCount, const std::string& sourceLabel) {
#if THREECPP_ENABLE_STB_IMAGE
    if (!bytes || byteCount == 0) return false;
    int w = 0, h = 0, comp = 0;
    stbi_uc* decoded = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(bytes), static_cast<int>(byteCount), &w, &h, &comp, 4);
    if (!decoded || w <= 0 || h <= 0) {
        if (decoded) stbi_image_free(decoded);
        return false;
    }
    t.width = w;
    t.height = h;
    t.channels = 4;
    t.pixels.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u);
    std::memcpy(t.pixels.data(), decoded, t.pixels.size());
    stbi_image_free(decoded);
    t.transcodeMessage = "decoded image payload via stb_image: " + sourceLabel;
    t.markNeedsUpdate();
    return true;
#else
    (void)t; (void)bytes; (void)byteCount; (void)sourceLabel;
    return false;
#endif
}

static bool decode_image_file(Texture& t, const std::filesystem::path& file) {
#if THREECPP_ENABLE_STB_IMAGE
    std::ifstream in(file, std::ios::binary);
    if (!in) return false;
    std::vector<char> raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (raw.empty()) return false;
    return decode_image_bytes(t, reinterpret_cast<const std::byte*>(raw.data()), raw.size(), file.string());
#else
    (void)t; (void)file;
    return false;
#endif
}

static std::string normalize_texture_lookup_key(std::string raw) {
    std::replace(raw.begin(), raw.end(), '\\', '/');
    // Assimp/FBX may preserve URL-like or Windows absolute paths.  Embedded
    // FBX media should match by either full normalized path or basename.
    std::filesystem::path fs(raw);
    std::string key = fs.filename().empty() ? raw : fs.filename().string();
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return key;
}

static bool embedded_texture_name_matches(const aiTexture* embedded, const std::string& rawPath, unsigned embeddedIndex) {
    if (!embedded) return false;
    const std::string rawKey = normalize_texture_lookup_key(rawPath);
    if (rawKey.empty()) return false;

    std::vector<std::string> candidates;
    candidates.push_back("*" + std::to_string(embeddedIndex));
    if (embedded->mFilename.length > 0) candidates.emplace_back(embedded->mFilename.C_Str());
    // Some Assimp FBX builds leave mFilename empty but keep only a format hint.
    // The explicit *N path above still covers numeric embedded references.
    for (auto& c : candidates) {
        if (c.empty()) continue;
        std::string full = c;
        std::replace(full.begin(), full.end(), '\\', '/');
        std::transform(full.begin(), full.end(), full.begin(), [](unsigned char ch){ return static_cast<char>(std::tolower(ch)); });
        if (full == rawKey || normalize_texture_lookup_key(c) == rawKey) return true;
    }
    return false;
}

static const aiTexture* find_embedded_texture(const aiScene* scene, const std::string& rawPath, int* outIndex = nullptr) {
    if (!scene || rawPath.empty() || scene->mNumTextures == 0) return nullptr;
    if (rawPath[0] == '*') {
        int idx = std::atoi(rawPath.c_str() + 1);
        if (idx >= 0 && static_cast<unsigned>(idx) < scene->mNumTextures) {
            if (outIndex) *outIndex = idx;
            return scene->mTextures[idx];
        }
    }
    for (unsigned i = 0; i < scene->mNumTextures; ++i) {
        if (embedded_texture_name_matches(scene->mTextures[i], rawPath, i)) {
            if (outIndex) *outIndex = static_cast<int>(i);
            return scene->mTextures[i];
        }
    }
    return nullptr;
}

static bool fill_texture_from_embedded(Texture& t, const aiTexture* embedded, int embeddedIndex, const std::string& rawPath) {
    if (!embedded) return false;
    t.embedded = true;
    t.sourcePath = "embedded://" + std::to_string(std::max(embeddedIndex, 0));
    t.name = embedded->mFilename.length > 0 ? embedded->mFilename.C_Str() : t.sourcePath;
    t.mimeType = embedded->achFormatHint;

    if (embedded->mHeight == 0) {
        // Compressed embedded FBX/GLB media: PNG/JPEG/TGA/etc bytes.  Keep KTX2
        // compressed, otherwise decode through stb_image when enabled.
        const std::size_t byteCount = static_cast<std::size_t>(embedded->mWidth);
        const std::byte* bytes = reinterpret_cast<const std::byte*>(embedded->pcData);
        std::string hint = t.mimeType.empty() ? rawPath : t.mimeType;
        if (hint.empty()) hint = t.name;
        if (looks_like_ktx2_or_basis(hint)) {
            t.pixels.resize(byteCount);
            std::memcpy(t.pixels.data(), bytes, t.pixels.size());
            t.width = static_cast<int>(embedded->mWidth);
            t.height = 1;
            t.channels = 0;
            t.markNeedsUpdate();
            return true;
        }
        if (decode_image_bytes(t, bytes, byteCount, hint)) return true;
        t.pixels.clear();
        t.width = 0;
        t.height = 0;
        t.channels = 0;
        t.transcodeMessage = "embedded image payload could not be decoded; enable stb package or use an external texture file beside the FBX";
        return true;
    }

    // Uncompressed embedded aiTexel array.
    t.width = static_cast<int>(embedded->mWidth);
    t.height = static_cast<int>(embedded->mHeight);
    t.channels = 4;
    t.pixels.resize(static_cast<std::size_t>(embedded->mWidth) * embedded->mHeight * 4u);
    std::memcpy(t.pixels.data(), embedded->pcData, t.pixels.size());
    t.markNeedsUpdate();
    return true;
}


static void mark_color_texture(Texture& t) {
    t.colorSpace = ColorSpace::SRGB;
    t.format = TextureFormat::SRGB8Alpha8;
}

static void mark_scalar_texture(Texture& t, TextureChannel channel = TextureChannel::R) {
    t.colorSpace = ColorSpace::LinearSRGB;
    t.format = TextureFormat::RGBA8;
    t.scalarChannel = channel;
}



static std::size_t find_matching_brace(const std::string& s, std::size_t openPos) {
    int depth = 0;
    bool inStr = false;
    bool esc = false;
    for (std::size_t i = openPos; i < s.size(); ++i) {
        char c = s[i];
        if (inStr) {
            if (esc) esc = false;
            else if (c == '\\') esc = true;
            else if (c == '"') inStr = false;
            continue;
        }
        if (c == '"') { inStr = true; continue; }
        if (c == '{' || c == '[') ++depth;
        if (c == '}' || c == ']') {
            --depth;
            if (depth == 0) return i;
        }
    }
    return std::string::npos;
}

static std::string json_member_block(const std::string& s, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto k = s.find(needle);
    if (k == std::string::npos) return {};
    auto colon = s.find(':', k + needle.size());
    if (colon == std::string::npos) return {};
    auto b = s.find_first_of("{[", colon + 1);
    if (b == std::string::npos) return {};
    auto e = find_matching_brace(s, b);
    if (e == std::string::npos) return {};
    return s.substr(b, e - b + 1);
}

static std::vector<std::string> json_array_objects(const std::string& s, const std::string& key) {
    std::vector<std::string> out;
    std::string arr = json_member_block(s, key);
    if (arr.empty() || arr.front() != '[') return out;
    bool inStr = false, esc = false;
    int depth = 0;
    std::size_t start = std::string::npos;
    for (std::size_t i = 1; i + 1 < arr.size(); ++i) {
        char c = arr[i];
        if (inStr) {
            if (esc) esc = false;
            else if (c == '\\') esc = true;
            else if (c == '"') inStr = false;
            continue;
        }
        if (c == '"') { inStr = true; continue; }
        if (c == '{') {
            if (depth == 0) start = i;
            ++depth;
        } else if (c == '}') {
            --depth;
            if (depth == 0 && start != std::string::npos) {
                out.push_back(arr.substr(start, i - start + 1));
                start = std::string::npos;
            }
        }
    }
    return out;
}

static bool json_number(const std::string& s, const std::string& key, float& out) {
    std::string needle = "\"" + key + "\"";
    auto k = s.find(needle);
    if (k == std::string::npos) return false;
    auto colon = s.find(':', k + needle.size());
    if (colon == std::string::npos) return false;
    auto b = s.find_first_of("-+.0123456789", colon + 1);
    if (b == std::string::npos) return false;
    char* end = nullptr;
    out = std::strtof(s.c_str() + b, &end);
    return end && end != s.c_str() + b;
}

static bool json_int(const std::string& s, const std::string& key, int& out) {
    float f = 0.0f;
    if (!json_number(s, key, f)) return false;
    out = static_cast<int>(f);
    return true;
}

static bool json_vec3(const std::string& s, const std::string& key, glm::vec3& out) {
    std::string needle = "\"" + key + "\"";
    auto k = s.find(needle);
    if (k == std::string::npos) return false;
    auto colon = s.find(':', k + needle.size());
    auto lb = s.find('[', colon);
    auto rb = s.find(']', lb);
    if (lb == std::string::npos || rb == std::string::npos) return false;
    std::string a = s.substr(lb + 1, rb - lb - 1);
    const char* c = a.c_str();
    char* e = nullptr;
    float x = std::strtof(c, &e); if (e == c) return false;
    c = e; while (*c && (*c == ',' || std::isspace(static_cast<unsigned char>(*c)))) ++c;
    float y = std::strtof(c, &e); if (e == c) return false;
    c = e; while (*c && (*c == ',' || std::isspace(static_cast<unsigned char>(*c)))) ++c;
    float z = std::strtof(c, &e); if (e == c) return false;
    out = {x, y, z};
    return true;
}

static std::string json_string_value(const std::string& s, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto k = s.find(needle);
    if (k == std::string::npos) return {};
    auto colon = s.find(':', k + needle.size());
    if (colon == std::string::npos) return {};
    auto q1 = s.find('"', colon + 1);
    if (q1 == std::string::npos) return {};
    auto q2 = s.find('"', q1 + 1);
    if (q2 == std::string::npos) return {};
    return s.substr(q1 + 1, q2 - q1 - 1);
}

static void append_indices_for_face(const aiFace& face, std::vector<std::uint32_t>& indices) {
    for (unsigned j = 0; j < face.mNumIndices; ++j) indices.push_back(face.mIndices[j]);
}

} // namespace

bool AssimpLoader::isSupportedExtension(std::string_view ext) {
    std::string e(ext);
    std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    if (!e.empty() && e[0] == '.') e.erase(e.begin());
    static const std::set<std::string> supported = {"fbx", "glb", "gltf", "obj", "dae", "blend", "3ds", "stl", "ply"};
    return supported.count(e) != 0;
}

std::vector<std::string> AssimpLoader::supportedExtensions() {
    return {"fbx", "glb", "gltf", "obj", "dae", "3ds", "stl", "ply"};
}

void AssimpLoader::resetImportState() {
    nodeObjects.clear();
    boneObjects.clear();
    boneNodeNames.clear();
    pendingSkins.clear();
    importedAnimations.clear();
    gltfMaterialOverrides.clear();
    importedFormat.clear();
    extensionReport = {};
}

std::shared_ptr<Object3D> AssimpLoader::load(const std::filesystem::path& file) {
    return loadResult(file).root;
}

AssimpLoadResult AssimpLoader::loadResult(const std::filesystem::path& file) {
    resetImportState();
    baseDir = file.parent_path();
    importedFormat = lower_ext(file);
    if (!isSupportedExtension(importedFormat)) {
        throw std::runtime_error("AssimpLoader unsupported extension: " + importedFormat + " for " + file.string());
    }

    parseGltfMaterialOverrides(file);

    Assimp::Importer importer;
    unsigned flags = aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality | aiProcess_RemoveRedundantMaterials | aiProcess_SortByPType;
    if (options.triangulate) flags |= aiProcess_Triangulate;
    if (options.generateNormals) flags |= aiProcess_GenSmoothNormals;
    if (options.generateTangents) flags |= aiProcess_CalcTangentSpace;
    if (options.flipUVs) flags |= aiProcess_FlipUVs;
    if (options.optimizeMeshes) flags |= aiProcess_OptimizeMeshes;
    if (options.validateData) flags |= aiProcess_ValidateDataStructure;
    // glTF is authored in a right-handed Y-up convention that maps cleanly to the renderer.
    // FBX/OBJ coordinate conversion is intentionally not forced here; users can pre-transform
    // imported roots if they need a project-specific convention.

    const aiScene* scene = importer.ReadFile(file.string(), flags);
    if (!scene || !scene->mRootNode) throw std::runtime_error(std::string("Assimp load failed: ") + importer.GetErrorString());

    if (options.inspectGltfExtensions) inspectSceneExtensions(scene);
    collectBoneNodeNames(scene);
    auto root = convertNode(scene, scene->mRootNode);
    root->name = file.filename().string();
    resolvePendingSkins();
    if (options.loadAnimations) convertAnimations(scene);

    AssimpLoadResult result;
    result.root = root;
    result.animations = importedAnimations;
    result.format = importedFormat;
    result.hasMeshes = scene->HasMeshes();
    result.hasSkins = !pendingSkins.empty() || !boneObjects.empty();
    result.hasAnimations = !result.animations.empty();
    result.extensionReport = extensionReport;
    return result;
}

void AssimpLoader::parseGltfMaterialOverrides(const std::filesystem::path& file) {
    gltfMaterialOverrides.clear();
    std::string ext = lower_ext(file);
    if (ext != "glb" && ext != "gltf") return;

    std::string json;
    std::vector<std::byte> bin;
    if (ext == "glb") {
        std::ifstream in(file, std::ios::binary);
        if (!in) return;
        std::vector<char> raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        std::vector<std::byte> data(raw.size());
        if (!raw.empty()) std::memcpy(data.data(), raw.data(), raw.size());
        if (data.size() < 20) return;
        auto rd32 = [&](std::size_t off) -> std::uint32_t {
            if (off + 4 > data.size()) return 0;
            std::uint32_t v = 0;
            std::memcpy(&v, data.data() + off, 4);
            return v;
        };
        if (rd32(0) != 0x46546C67u) return; // glTF
        std::size_t off = 12;
        while (off + 8 <= data.size()) {
            std::uint32_t len = rd32(off); off += 4;
            std::uint32_t type = rd32(off); off += 4;
            if (off + len > data.size()) break;
            const std::byte* ptr = data.data() + off;
            if (type == 0x4E4F534Au) { // JSON
                json.assign(reinterpret_cast<const char*>(ptr), reinterpret_cast<const char*>(ptr) + len);
            } else if (type == 0x004E4942u) { // BIN\0
                bin.assign(ptr, ptr + len);
            }
            off += len;
        }
    } else {
        std::ifstream in(file);
        if (!in) return;
        json.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    }
    if (json.empty()) return;

    struct BufferViewInfo { std::size_t offset = 0; std::size_t length = 0; };
    std::vector<BufferViewInfo> bufferViews;
    for (const auto& bv : json_array_objects(json, "bufferViews")) {
        int off = 0, len = 0;
        json_int(bv, "byteOffset", off);
        json_int(bv, "byteLength", len);
        bufferViews.push_back({static_cast<std::size_t>(std::max(0, off)), static_cast<std::size_t>(std::max(0, len))});
    }

    struct ImageInfo { int bufferView = -1; std::string mimeType; std::string name; };
    std::vector<ImageInfo> images;
    for (const auto& im : json_array_objects(json, "images")) {
        ImageInfo info;
        json_int(im, "bufferView", info.bufferView);
        info.mimeType = json_string_value(im, "mimeType");
        info.name = json_string_value(im, "name");
        images.push_back(info);
    }

    struct TextureInfo { int source = -1; };
    std::vector<TextureInfo> textures;
    for (const auto& tx : json_array_objects(json, "textures")) {
        TextureInfo info;
        json_int(tx, "source", info.source);
        textures.push_back(info);
    }

    auto loadTextureIndex = [&](int textureIndex, ColorSpace cs, TextureChannel channel) -> std::shared_ptr<Texture> {
        if (textureIndex < 0 || textureIndex >= static_cast<int>(textures.size())) return nullptr;
        int imageIndex = textures[textureIndex].source;
        if (imageIndex < 0 || imageIndex >= static_cast<int>(images.size())) return nullptr;
        const auto& im = images[imageIndex];
        if (im.bufferView < 0 || im.bufferView >= static_cast<int>(bufferViews.size())) return nullptr;
        const auto& bv = bufferViews[im.bufferView];
        if (bv.offset + bv.length > bin.size() || bv.length == 0) return nullptr;
        auto tex = make_ref<Texture>();
        tex->name = im.name.empty() ? ("gltf_texture_" + std::to_string(textureIndex)) : im.name;
        tex->sourcePath = "gltf://texture/" + std::to_string(textureIndex);
        tex->mimeType = im.mimeType;
        tex->embedded = true;
        tex->flipY = false;
        tex->colorSpace = cs;
        tex->scalarChannel = channel;
        tex->format = cs == ColorSpace::SRGB ? TextureFormat::SRGB8Alpha8 : TextureFormat::RGBA8;
        if (!decode_image_bytes(*tex, bin.data() + bv.offset, bv.length, tex->mimeType.empty() ? tex->sourcePath : tex->mimeType)) {
            // Texture::compressedContainer is an enum, not a payload holder.
            // If stb cannot decode this embedded PNG/JPEG payload, keep the texture as an
            // ordinary unresolved 2D texture so the renderer uploads its safe white fallback.
            // KTX2/Basis payload preservation is handled by the dedicated KTX2 path.
            tex->compressed = false;
            tex->compressedContainer = CompressedTextureContainer::None;
            tex->transcodeMessage = "embedded image payload could not be decoded; using safe texture fallback";
        }
        return tex;
    };

    const auto materials = json_array_objects(json, "materials");
    gltfMaterialOverrides.resize(materials.size());
    for (std::size_t i = 0; i < materials.size(); ++i) {
        const std::string& mat = materials[i];
        auto& ov = gltfMaterialOverrides[i];
        std::string tr = json_member_block(mat, "KHR_materials_transmission");
        if (!tr.empty()) {
            float v = 0.0f;
            if (json_number(tr, "transmissionFactor", v)) {
                ov.hasTransmission = true;
                ov.transmission = glm::clamp(v, 0.0f, 1.0f);
                ov.valid = true;
            }
        }
        std::string vol = json_member_block(mat, "KHR_materials_volume");
        if (!vol.empty()) {
            ov.hasVolume = true;
            ov.valid = true;
            json_number(vol, "thicknessFactor", ov.thickness);
            json_number(vol, "attenuationDistance", ov.attenuationDistance);
            json_vec3(vol, "attenuationColor", ov.attenuationColor);
            std::string thicknessTex = json_member_block(vol, "thicknessTexture");
            int texIndex = -1;
            if (!thicknessTex.empty() && json_int(thicknessTex, "index", texIndex)) {
                ov.thicknessMap = loadTextureIndex(texIndex, ColorSpace::LinearSRGB, TextureChannel::R);
                if (ov.thicknessMap) {
                    ov.thicknessMap->uvChannel = 0;
                    ov.thicknessMap->scalarChannel = TextureChannel::R;
                    ov.thicknessMap->transcodeMessage += "; glTF KHR_materials_volume.thicknessTexture";
                }
            }
        }
        std::string ior = json_member_block(mat, "KHR_materials_ior");
        if (!ior.empty()) {
            float v = 1.5f;
            if (json_number(ior, "ior", v)) { ov.hasIor = true; ov.ior = v; ov.valid = true; }
        }
        std::string disp = json_member_block(mat, "KHR_materials_dispersion");
        if (!disp.empty()) {
            float v = 0.0f;
            if (json_number(disp, "dispersion", v)) { ov.hasDispersion = true; ov.dispersion = v; ov.valid = true; }
        }
    }
}


void AssimpLoader::collectBoneNodeNames(const aiScene* scene) {
    if (!scene || !options.loadSkinning) return;
    std::set<std::string> names;
    for (unsigned mi = 0; mi < scene->mNumMeshes; ++mi) {
        aiMesh* mesh = scene->mMeshes[mi];
        if (!mesh) continue;
        for (unsigned bi = 0; bi < mesh->mNumBones; ++bi) {
            names.insert(mesh->mBones[bi]->mName.C_Str());
        }
    }
    boneNodeNames.assign(names.begin(), names.end());
}

std::shared_ptr<Object3D> AssimpLoader::convertNode(const aiScene* scene, aiNode* node) {
    const std::string nodeName = node->mName.C_Str();
    const bool isBoneNode = std::find(boneNodeNames.begin(), boneNodeNames.end(), nodeName) != boneNodeNames.end();
    std::shared_ptr<Object3D> out = isBoneNode ? std::static_pointer_cast<Object3D>(make_ref<Bone>()) : make_ref<Object3D>();
    out->name = nodeName;
    aiVector3D scaling, position;
    aiQuaternion rotation;
    node->mTransformation.Decompose(scaling, rotation, position);
    out->position = to_glm(position);
    out->quaternion = to_glm(rotation);
    out->scale = to_glm(scaling);
    out->matrixAutoUpdate = true;
    out->updateMatrix();
    nodeObjects[nodeName] = out.get();
    if (auto* b = dynamic_cast<Bone*>(out.get())) boneObjects[nodeName] = b;

    for (unsigned i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        out->add(convertMesh(scene, mesh));
    }
    for (unsigned i = 0; i < node->mNumChildren; ++i) {
        out->add(convertNode(scene, node->mChildren[i]));
    }
    return out;
}

std::shared_ptr<Mesh> AssimpLoader::convertMesh(const aiScene* scene, aiMesh* mesh) {
    std::vector<float> positions, normals, tangents, uvs, uv2s, colors;
    positions.reserve(mesh->mNumVertices * 3);
    normals.reserve(mesh->mNumVertices * 3);
    tangents.reserve(mesh->mNumVertices * 4);
    uvs.reserve(mesh->mNumVertices * 2);
    uv2s.reserve(mesh->mNumVertices * 2);
    colors.reserve(mesh->mNumVertices * 3);

    for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
        auto& p = mesh->mVertices[i]; positions.insert(positions.end(), {p.x, p.y, p.z});
        if (mesh->HasNormals()) { auto& n = mesh->mNormals[i]; normals.insert(normals.end(), {n.x, n.y, n.z}); }
        else normals.insert(normals.end(), {0, 1, 0});
        if (mesh->HasTangentsAndBitangents()) {
            auto& t = mesh->mTangents[i];
            tangents.insert(tangents.end(), {t.x, t.y, t.z, 1.0f});
        }
        if (mesh->HasTextureCoords(0)) { auto& uv = mesh->mTextureCoords[0][i]; uvs.insert(uvs.end(), {uv.x, uv.y}); }
        if (mesh->HasTextureCoords(1)) { auto& uv = mesh->mTextureCoords[1][i]; uv2s.insert(uv2s.end(), {uv.x, uv.y}); }
        if (mesh->HasVertexColors(0)) { auto& c = mesh->mColors[0][i]; colors.insert(colors.end(), {c.r, c.g, c.b}); }
    }

    std::vector<std::uint32_t> indices;
    for (unsigned f = 0; f < mesh->mNumFaces; ++f) append_indices_for_face(mesh->mFaces[f], indices);

    std::vector<float> skinIndex(mesh->mNumVertices * 4, 0.0f);
    std::vector<float> skinWeight(mesh->mNumVertices * 4, 0.0f);
    std::vector<std::string> boneNames;
    std::vector<glm::mat4> boneInverses;
    if (options.loadSkinning && mesh->HasBones()) {
        boneNames.reserve(mesh->mNumBones);
        boneInverses.reserve(mesh->mNumBones);
        for (unsigned bi = 0; bi < mesh->mNumBones; ++bi) {
            aiBone* b = mesh->mBones[bi];
            boneNames.emplace_back(b->mName.C_Str());
            boneInverses.push_back(to_glm(b->mOffsetMatrix));
            for (unsigned wi = 0; wi < b->mNumWeights; ++wi) {
                const auto& w = b->mWeights[wi];
                const unsigned v = w.mVertexId;
                if (v >= mesh->mNumVertices) continue;
                for (int slot = 0; slot < 4; ++slot) {
                    if (skinWeight[v * 4 + slot] <= 0.0f) {
                        skinIndex[v * 4 + slot] = static_cast<float>(bi);
                        skinWeight[v * 4 + slot] = w.mWeight;
                        break;
                    }
                }
            }
        }
        for (unsigned v = 0; v < mesh->mNumVertices; ++v) {
            float sum = skinWeight[v*4+0] + skinWeight[v*4+1] + skinWeight[v*4+2] + skinWeight[v*4+3];
            if (sum > 1e-6f) {
                for (int k = 0; k < 4; ++k) skinWeight[v*4+k] /= sum;
            } else {
                skinWeight[v*4+0] = 1.0f;
            }
        }
    }

    std::vector<std::string> morphTargetNames;
    std::vector<BufferAttribute> morphPositions;
    std::vector<BufferAttribute> morphNormals;
    if (options.loadMorphTargets && mesh->mNumAnimMeshes > 0) {
        morphTargetNames.reserve(mesh->mNumAnimMeshes);
        morphPositions.reserve(mesh->mNumAnimMeshes);
        morphNormals.reserve(mesh->mNumAnimMeshes);
        for (unsigned mi = 0; mi < mesh->mNumAnimMeshes; ++mi) {
            const aiAnimMesh* am = mesh->mAnimMeshes[mi];
            if (!am) continue;
            std::string targetName = am->mName.length ? am->mName.C_Str() : ("morphTarget_" + std::to_string(mi));
            morphTargetNames.push_back(targetName);
            if (am->mVertices) {
                std::vector<float> mp; mp.reserve(mesh->mNumVertices * 3);
                for (unsigned vi = 0; vi < mesh->mNumVertices; ++vi) {
                    const auto& v = am->mVertices[vi];
                    mp.insert(mp.end(), {v.x, v.y, v.z});
                }
                morphPositions.push_back(BufferAttribute::fromVector(mp, 3, AttributeType::Float32));
            }
            if (am->mNormals) {
                std::vector<float> mn; mn.reserve(mesh->mNumVertices * 3);
                for (unsigned vi = 0; vi < mesh->mNumVertices; ++vi) {
                    const auto& n = am->mNormals[vi];
                    mn.insert(mn.end(), {n.x, n.y, n.z});
                }
                morphNormals.push_back(BufferAttribute::fromVector(mn, 3, AttributeType::Float32));
            }
        }
    }

    auto geo = make_ref<BufferGeometry>();
    geo->name = mesh->mName.C_Str();
    // Assimp aiAnimMesh values are absolute target values for glTF/FBX blend shapes.
    geo->morphTargetsRelative = false;
    geo->setAttribute("position", BufferAttribute::fromVector(positions, 3, AttributeType::Float32));
    geo->setAttribute("normal", BufferAttribute::fromVector(normals, 3, AttributeType::Float32));
    if (!tangents.empty()) geo->setAttribute("tangent", BufferAttribute::fromVector(tangents, 4, AttributeType::Float32));
    if (!uvs.empty()) geo->setAttribute("uv", BufferAttribute::fromVector(uvs, 2, AttributeType::Float32));
    if (!uv2s.empty()) geo->setAttribute("uv2", BufferAttribute::fromVector(uv2s, 2, AttributeType::Float32));
    if (!colors.empty()) geo->setAttribute("color", BufferAttribute::fromVector(colors, 3, AttributeType::Float32));
    if (!morphPositions.empty()) geo->setMorphAttribute("position", std::span<const BufferAttribute>(morphPositions.data(), morphPositions.size()));
    if (!morphNormals.empty()) geo->setMorphAttribute("normal", std::span<const BufferAttribute>(morphNormals.data(), morphNormals.size()));
    if (!boneNames.empty()) {
        geo->setAttribute("skinIndex", BufferAttribute::fromVector(skinIndex, 4, AttributeType::Float32));
        geo->setAttribute("skinWeight", BufferAttribute::fromVector(skinWeight, 4, AttributeType::Float32));
    }
    geo->setIndex(std::span<const std::uint32_t>(indices.data(), indices.size()));
    geo->computeBoundingSphere();

    std::shared_ptr<Material> material;
    if (mesh->mMaterialIndex < scene->mNumMaterials) material = convertMaterial(scene, scene->mMaterials[mesh->mMaterialIndex], mesh->mMaterialIndex);
    if (!material) material = make_ref<MeshBasicMaterial>();
    if (!colors.empty()) material->vertexColors = true;

    std::shared_ptr<Mesh> out;
    if (!boneNames.empty()) {
        auto skinned = make_ref<SkinnedMesh>(geo, material);
        out = skinned;
        pendingSkins.push_back({skinned.get(), boneNames, boneInverses});
    } else {
        out = make_ref<Mesh>(geo, material);
    }
    out->name = mesh->mName.C_Str();
    if (!morphTargetNames.empty()) {
        out->morphTargetInfluences.assign(morphTargetNames.size(), 0.0f);
        for (std::size_t i = 0; i < morphTargetNames.size(); ++i) out->morphTargetDictionary[morphTargetNames[i]] = static_cast<int>(i);
    }
    out->castShadow = true;
    out->receiveShadow = true;
    return out;
}

std::shared_ptr<Material> AssimpLoader::convertMaterial(const aiScene* scene, aiMaterial* mat, unsigned materialIndex) {
    aiColor4D base;
    aiString name;
    mat->Get(AI_MATKEY_NAME, name);
    if (options.inspectGltfExtensions) inspectMaterialExtensions(mat);

    auto loadFirstTexture = [&](std::initializer_list<aiTextureType> types) -> std::shared_ptr<Texture> {
        for (aiTextureType type : types) {
            const unsigned count = mat ? mat->GetTextureCount(type) : 0;
            for (unsigned i = 0; i < count; ++i) {
                if (auto tex = loadMaterialTexture(scene, mat, type, i)) return tex;
            }
        }
        return nullptr;
    };

    auto loadTextureByPathHint = [&](std::initializer_list<aiTextureType> types, std::initializer_list<const char*> hints) -> std::shared_ptr<Texture> {
        for (aiTextureType type : types) {
            const unsigned count = mat ? mat->GetTextureCount(type) : 0;
            for (unsigned i = 0; i < count; ++i) {
                aiString texPath;
                if (mat->GetTexture(type, i, &texPath) != AI_SUCCESS) continue;
                std::string lower = texPath.C_Str();
                std::replace(lower.begin(), lower.end(), '\\', '/');
                std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
                for (const char* hint : hints) {
                    if (hint && lower.find(hint) != std::string::npos) {
                        if (auto tex = loadMaterialTexture(scene, mat, type, i)) return tex;
                    }
                }
            }
        }
        return nullptr;
    };

    if (options.preferBasicMaterial) {
        auto m = make_ref<MeshBasicMaterial>();
        m->name = name.C_Str();
        if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_BASE_COLOR, &base) || AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &base)) {
            m->color = {base.r, base.g, base.b}; m->opacity = base.a; m->transparent = base.a < 1.0f;
        }
        if (options.loadTextures) {
            m->map = loadFirstTexture({aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE});
            if (!m->map) m->map = loadTextureByPathHint({aiTextureType_UNKNOWN}, {"diffuse", "albedo", "basecolor", "base_color", "color"});
            m->alphaMap = loadFirstTexture({aiTextureType_OPACITY});
            if (!m->alphaMap) m->alphaMap = loadTextureByPathHint({aiTextureType_UNKNOWN}, {"opacity", "alpha"});
        }
        return m;
    }

    auto m = make_ref<MeshStandardMaterial>();
    m->name = name.C_Str();
    if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_BASE_COLOR, &base) || AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &base)) {
        m->color = {base.r, base.g, base.b}; m->opacity = base.a; m->transparent = base.a < 1.0f;
    }
    aiColor4D emissive;
    if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_EMISSIVE, &emissive)) m->emissive = {emissive.r, emissive.g, emissive.b};

    float roughness = 1.0f, metallic = 0.0f, opacity = m->opacity;
    aiGetMaterialFloat(mat, AI_MATKEY_ROUGHNESS_FACTOR, &roughness);
    aiGetMaterialFloat(mat, AI_MATKEY_METALLIC_FACTOR, &metallic);
    aiGetMaterialFloat(mat, AI_MATKEY_OPACITY, &opacity);
    // Assimp exposes glTF PBR factors under different raw keys across versions.
    // Prefer explicit glTF aliases when present; otherwise keep the generic keys above.
    get_any_float(mat, roughness, {"$mat.gltf.pbrMetallicRoughness.roughnessFactor", "$mat.gltf.roughnessFactor", "$mat.gltf.roughness.factor"});
    get_any_float(mat, metallic, {"$mat.gltf.pbrMetallicRoughness.metallicFactor", "$mat.gltf.metallicFactor", "$mat.gltf.metallic.factor"});
    m->roughness = glm::clamp(roughness, 0.045f, 1.0f);
    m->metalness = glm::clamp(metallic, 0.0f, 1.0f);
    m->opacity = glm::clamp(opacity, 0.0f, 1.0f);
    if (opacity < 1.0f) { m->transparent = true; m->depthWrite = false; }

    float normalTextureScale = 1.0f;
    if (get_any_float(mat, normalTextureScale, {"$mat.gltf.normalTexture.scale", "$mat.gltf.normalScale", "$tex.normal.scale"})) {
        m->normalScale = {normalTextureScale, normalTextureScale};
    }
    float emissiveStrength = 1.0f;
    if (get_any_float(mat, emissiveStrength, {"$mat.gltf.emissiveStrength", "$mat.gltf.emissive_strength.factor", "$mat.gltf.KHR_materials_emissive_strength.emissiveStrength"})) {
        m->emissiveIntensity = emissiveStrength;
    }

    float aoStrength = 1.0f;
    if (get_any_float(mat, aoStrength, {"$mat.gltf.occlusionTexture.strength", "$tex.occlusion.strength"})) {
        m->aoMapIntensity = aoStrength;
    }

    int twoSided = 0;
    if (mat->Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS && twoSided) m->side = Side::DoubleSide;

    aiString alphaMode;
    if (get_string(mat, "$mat.gltf.alphaMode", 0, 0, alphaMode)) {
        const std::string mode = alphaMode.C_Str();
        if (mode == "BLEND") { m->transparent = true; m->depthWrite = false; }
        if (mode == "MASK") { m->transparent = false; m->depthWrite = true; m->alphaTest = 0.5f; get_float(mat, "$mat.gltf.alphaCutoff", 0, 0, m->alphaTest); }
    }

    // Common glTF KHR_materials_* fields exposed by Assimp through raw material keys.
    float transmission = 0.0f, ior = 1.5f, thickness = 0.0f, clearcoat = 0.0f, clearcoatRoughness = 0.0f, sheenRoughness = 1.0f;
    float attenuationDistance = std::numeric_limits<float>::infinity();
    float specularIntensity = 1.0f, iridescence = 0.0f, iridescenceIOR = 1.3f, anisotropy = 0.0f, dispersion = 0.0f;
    aiColor4D attenuationColor{1,1,1,1}, sheenColor{0,0,0,1}, specularColor{1,1,1,1};
    const GltfMaterialOverride* gltfOverride = materialIndex < gltfMaterialOverrides.size() ? &gltfMaterialOverrides[materialIndex] : nullptr;

    bool wantsPhysical = false;
    wantsPhysical |= get_float(mat, "$mat.gltf.transmission.factor", 0, 0, transmission) && transmission > 0.0f;
    wantsPhysical |= get_float(mat, "$mat.gltf.ior", 0, 0, ior);
    wantsPhysical |= get_float(mat, "$mat.gltf.volume.thicknessFactor", 0, 0, thickness) && thickness > 0.0f;
    wantsPhysical |= get_float(mat, "$mat.gltf.volume.attenuationDistance", 0, 0, attenuationDistance);
    wantsPhysical |= get_any_color(mat, attenuationColor, {"$mat.gltf.volume.attenuationColor"});
    wantsPhysical |= get_float(mat, "$mat.gltf.clearcoat.factor", 0, 0, clearcoat) && clearcoat > 0.0f;
    wantsPhysical |= get_float(mat, "$mat.gltf.clearcoat.roughnessFactor", 0, 0, clearcoatRoughness) && clearcoatRoughness > 0.0f;
    wantsPhysical |= get_float(mat, "$mat.gltf.sheen.roughnessFactor", 0, 0, sheenRoughness) && sheenRoughness < 1.0f;
    wantsPhysical |= get_any_color(mat, sheenColor, {"$mat.gltf.sheen.colorFactor"});
    wantsPhysical |= get_float(mat, "$mat.gltf.specular.factor", 0, 0, specularIntensity) && std::abs(specularIntensity - 1.0f) > 1e-4f;
    wantsPhysical |= get_any_color(mat, specularColor, {"$mat.gltf.specular.colorFactor"});
    wantsPhysical |= get_float(mat, "$mat.gltf.iridescence.factor", 0, 0, iridescence) && iridescence > 0.0f;
    wantsPhysical |= get_float(mat, "$mat.gltf.iridescence.ior", 0, 0, iridescenceIOR);
    wantsPhysical |= get_float(mat, "$mat.gltf.anisotropy.strength", 0, 0, anisotropy) && anisotropy != 0.0f;

    if (gltfOverride && gltfOverride->valid) {
        if (gltfOverride->hasTransmission) { transmission = gltfOverride->transmission; wantsPhysical = true; }
        if (gltfOverride->hasIor) { ior = gltfOverride->ior; wantsPhysical = true; }
        if (gltfOverride->hasVolume) {
            thickness = gltfOverride->thickness;
            attenuationDistance = gltfOverride->attenuationDistance;
            attenuationColor = aiColor4D(gltfOverride->attenuationColor.r, gltfOverride->attenuationColor.g, gltfOverride->attenuationColor.b, 1.0f);
            wantsPhysical = true;
        }
        if (gltfOverride->hasDispersion) { dispersion = gltfOverride->dispersion; wantsPhysical = true; }
    }

    std::shared_ptr<MeshStandardMaterial> target = m;
    std::shared_ptr<MeshPhysicalMaterial> physical;
    if (wantsPhysical) {
        physical = make_ref<MeshPhysicalMaterial>();
        static_cast<MeshStandardMaterial&>(*physical) = *m;
        // Preserve a unique material identity after copying base Standard fields.
        physical->id = next_object_id();
        physical->version = 0;
        physical->needsUpdate = true;
        physical->type = MaterialType::MeshPhysical;
        physical->transmission = transmission;
        // glTF KHR_materials_transmission is NOT alpha transparency in three.js.
        // It is an opaque transmissive surface rendered through the transmission
        // background pass: the shader writes a refracted/reflected color with alpha
        // 1.0 instead of blending the whole mesh as a ghost.  The previous fallback
        // set transparent=true/depthWrite=false, which made DragonDispersion.glb
        // look like a semi-transparent shell rather than thick refractive glass.
        if (transmission > 0.0f || thickness > 0.0f) {
            physical->transparent = false;
            physical->depthWrite = true;
            physical->opacity = 1.0f;
        }
        physical->ior = ior;
        physical->thickness = thickness;
        physical->clearcoat = clearcoat;
        physical->clearcoatRoughness = clearcoatRoughness;
        physical->sheenRoughness = sheenRoughness;
        physical->attenuationDistance = attenuationDistance;
        physical->attenuationColor = {attenuationColor.r, attenuationColor.g, attenuationColor.b};
        physical->sheenColor = {sheenColor.r, sheenColor.g, sheenColor.b};
        physical->sheen = glm::max(glm::max(sheenColor.r, sheenColor.g), sheenColor.b);
        physical->specularIntensity = specularIntensity;
        physical->specularColor = {specularColor.r, specularColor.g, specularColor.b};
        physical->iridescence = iridescence;
        physical->iridescenceIOR = iridescenceIOR;
        physical->anisotropy = anisotropy;
        physical->dispersion = dispersion;
        target = physical;
    }

    if (options.loadTextures) {
        target->map = loadFirstTexture({aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE});
        if (!target->map) target->map = loadTextureByPathHint({aiTextureType_UNKNOWN}, {"diffuse", "albedo", "basecolor", "base_color", "color"});
        if (target->map) mark_color_texture(*target->map);
        target->normalMap = loadFirstTexture({aiTextureType_NORMALS, aiTextureType_HEIGHT});
        if (!target->normalMap) target->normalMap = loadTextureByPathHint({aiTextureType_UNKNOWN}, {"normal", "nrm", "bump"});
        if (target->normalMap) mark_scalar_texture(*target->normalMap);
        target->roughnessMap = loadFirstTexture({aiTextureType_DIFFUSE_ROUGHNESS});
        if (!target->roughnessMap) target->roughnessMap = loadTextureByPathHint({aiTextureType_UNKNOWN}, {"roughness", "rough"});
        target->metalnessMap = loadFirstTexture({aiTextureType_METALNESS});
        if (!target->metalnessMap) target->metalnessMap = loadTextureByPathHint({aiTextureType_UNKNOWN}, {"metallic", "metalness", "metal"});
        // glTF metallicRoughnessTexture stores roughness in G and metalness in B.
        // Assimp may expose it through one or both texture slots depending on version.
        // Sharing the same Texture preserves the correct channel sampling in the shader.
        if (target->roughnessMap && !target->metalnessMap) target->metalnessMap = target->roughnessMap;
        if (target->metalnessMap && !target->roughnessMap) target->roughnessMap = target->metalnessMap;
        if (target->roughnessMap) { target->roughnessMap->scalarChannel = TextureChannel::G; mark_scalar_texture(*target->roughnessMap, TextureChannel::G); }
        if (target->metalnessMap) { target->metalnessMap->scalarChannel = TextureChannel::B; mark_scalar_texture(*target->metalnessMap, TextureChannel::B); }
        target->aoMap = loadFirstTexture({aiTextureType_AMBIENT_OCCLUSION, aiTextureType_LIGHTMAP});
        if (!target->aoMap) target->aoMap = loadTextureByPathHint({aiTextureType_UNKNOWN}, {"ao", "ambientocclusion", "occlusion"});
        if (target->aoMap) { mark_scalar_texture(*target->aoMap, TextureChannel::R); target->aoMap->uvChannel = std::max(target->aoMap->uvChannel, 1); }
        target->emissiveMap = loadFirstTexture({aiTextureType_EMISSIVE});
        if (!target->emissiveMap) target->emissiveMap = loadTextureByPathHint({aiTextureType_UNKNOWN}, {"emissive", "emission", "emit"});
        if (target->emissiveMap) mark_color_texture(*target->emissiveMap);
        target->alphaMap = loadFirstTexture({aiTextureType_OPACITY});
        if (!target->alphaMap) target->alphaMap = loadTextureByPathHint({aiTextureType_UNKNOWN}, {"opacity", "alpha"});
        if (target->alphaMap) mark_scalar_texture(*target->alphaMap, TextureChannel::G);
        target->lightMap = loadFirstTexture({aiTextureType_LIGHTMAP});
        if (target->lightMap) { mark_color_texture(*target->lightMap); target->lightMap->uvChannel = std::max(target->lightMap->uvChannel, 1); }
        if (physical) {
            // Assimp exposes some KHR_materials_* texture slots in recent versions,
            // but enum names vary across package versions. Use stable numeric fallbacks
            // to keep xmake package builds source-compatible. Missing slots simply return null.
            physical->transmissionMap = loadMaterialTexture(scene, mat, static_cast<aiTextureType>(21));
            physical->clearcoatMap = loadMaterialTexture(scene, mat, static_cast<aiTextureType>(20));
            physical->clearcoatRoughnessMap = loadMaterialTexture(scene, mat, static_cast<aiTextureType>(22));
            physical->clearcoatNormalMap = loadMaterialTexture(scene, mat, static_cast<aiTextureType>(23));
            physical->sheenColorMap = loadMaterialTexture(scene, mat, static_cast<aiTextureType>(19));
            physical->sheenRoughnessMap = loadMaterialTexture(scene, mat, static_cast<aiTextureType>(24));
            physical->specularIntensityMap = loadMaterialTexture(scene, mat, static_cast<aiTextureType>(25));
            physical->specularColorMap = loadMaterialTexture(scene, mat, static_cast<aiTextureType>(26));
            physical->thicknessMap = loadMaterialTexture(scene, mat, static_cast<aiTextureType>(27));
            if (!physical->thicknessMap && gltfOverride && gltfOverride->thicknessMap) physical->thicknessMap = gltfOverride->thicknessMap;
            if (physical->thicknessMap) mark_scalar_texture(*physical->thicknessMap, TextureChannel::R);
            physical->iridescenceMap = loadMaterialTexture(scene, mat, static_cast<aiTextureType>(28));
            physical->iridescenceThicknessMap = loadMaterialTexture(scene, mat, static_cast<aiTextureType>(29));
            physical->anisotropyMap = loadMaterialTexture(scene, mat, static_cast<aiTextureType>(30));
        }
    }
    return target;
}


static std::filesystem::path find_texture_file_fallback(const std::filesystem::path& baseDir, const std::string& rawPath) {
    if (rawPath.empty()) return {};
    std::string normalized = rawPath;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    std::filesystem::path rawFs(normalized);
    std::vector<std::filesystem::path> candidates;
    if (rawFs.is_absolute()) candidates.push_back(rawFs);
    candidates.push_back(baseDir / rawFs);
    candidates.push_back(baseDir / rawFs.filename());
    candidates.push_back(baseDir / "textures" / rawFs.filename());
    candidates.push_back(baseDir / "Textures" / rawFs.filename());
    candidates.push_back(baseDir / "texture" / rawFs.filename());
    candidates.push_back(baseDir / "Texture" / rawFs.filename());
    for (const auto& c : candidates) {
        std::error_code ec;
        if (!c.empty() && std::filesystem::exists(c, ec)) return c.lexically_normal();
    }
    // FBX often stores absolute paths from another machine.  Try a shallow-ish
    // recursive basename search under the model directory so exported texture
    // folders still work without rewriting the FBX.
    std::error_code ec;
    const auto fileName = rawFs.filename();
    if (!fileName.empty() && std::filesystem::exists(baseDir, ec)) {
        int visited = 0;
        for (auto it = std::filesystem::recursive_directory_iterator(baseDir, std::filesystem::directory_options::skip_permission_denied, ec);
             !ec && it != std::filesystem::recursive_directory_iterator(); ++it) {
            if (++visited > 4096) break;
            if (!it->is_regular_file(ec)) continue;
            if (it->path().filename() == fileName) return it->path().lexically_normal();
        }
    }
    return {};
}

std::shared_ptr<Texture> AssimpLoader::loadMaterialTexture(const aiScene* scene, aiMaterial* mat, aiTextureType type, unsigned index) {
    if (!mat || mat->GetTextureCount(type) <= index) return nullptr;
    aiString path;
    aiTextureMapping mapping = aiTextureMapping_UV;
    unsigned uvIndex = 0;
    float blend = 1.0f;
    aiTextureOp op = aiTextureOp_Multiply;
    aiTextureMapMode mapMode[2] = {aiTextureMapMode_Wrap, aiTextureMapMode_Wrap};
    if (mat->GetTexture(type, index, &path, &mapping, &uvIndex, &blend, &op, mapMode) != AI_SUCCESS) return nullptr;

    auto t = std::make_shared<Texture>();
    t->name = path.C_Str();
    t->sourcePath = t->name;
    t->colorSpace = (type == aiTextureType_BASE_COLOR || type == aiTextureType_DIFFUSE || type == aiTextureType_EMISSIVE) ? ColorSpace::SRGB : ColorSpace::LinearSRGB;
    t->format = (t->colorSpace == ColorSpace::SRGB) ? TextureFormat::SRGB8Alpha8 : TextureFormat::RGBA8;
    // Match three.js loader semantics: glTF textures are uploaded with flipY=false,
    // while FBX/OBJ/DAE textures keep the default TextureLoader orientation (flipY=true).
    // If the caller explicitly asked Assimp to flip UVs, do not flip pixels as well.
    const bool isGltfAsset = importedFormat == "gltf" || importedFormat == "glb";
    t->flipY = !isGltfAsset && !options.flipUVs;
    t->uvChannel = static_cast<int>(uvIndex);
    t->wrapS = to_wrap(mapMode[0]);
    t->wrapT = to_wrap(mapMode[1]);
    apply_texture_transform(*t, mat, type, index);

    const std::string raw = path.C_Str();
    if (scene && options.loadEmbeddedTextures) {
        int embeddedIndex = -1;
        if (const aiTexture* embedded = find_embedded_texture(scene, raw, &embeddedIndex)) {
            fill_texture_from_embedded(*t, embedded, embeddedIndex, raw);
            updateTextureExtensionMetadata(*t, t->mimeType.empty() ? raw : t->mimeType);
            return t;
        }
    }

    std::filesystem::path resolved = find_texture_file_fallback(baseDir, raw);
    if (resolved.empty()) {
        // Keep the original path for diagnostics, but avoid pretending the texture
        // was successfully loaded.  This is especially important for FBX files
        // that contain Windows absolute texture paths.
        resolved = (baseDir / std::filesystem::path(raw)).lexically_normal();
        t->transcodeMessage = "external texture not found; tried raw path, basename, common texture folders, and recursive basename search";
    }
    t->sourcePath = resolved.lexically_normal().string();
    t->name = t->sourcePath;
    updateTextureExtensionMetadata(*t, raw);
    if (!t->compressed && std::filesystem::exists(t->sourcePath)) {
        if (!decode_image_file(*t, t->sourcePath)) {
            t->transcodeMessage = "texture file found but image decode failed; enable stb or check file format";
        } else {
            t->transcodeMessage = "decoded external image file";
        }
    }
    return t;
}

void AssimpLoader::resolvePendingSkins() {
    for (auto& p : pendingSkins) {
        if (!p.mesh) continue;
        auto skeleton = make_ref<Skeleton>();
        skeleton->bones.reserve(p.boneNames.size());
        skeleton->boneInverses = p.boneInverses;
        for (std::size_t i = 0; i < p.boneNames.size(); ++i) {
            Bone* b = nullptr;
            if (auto it = boneObjects.find(p.boneNames[i]); it != boneObjects.end()) b = it->second;
            if (!b) {
                auto orphan = make_ref<Bone>();
                orphan->name = p.boneNames[i];
                orphan->boneIndex = static_cast<int>(i);
                // Keep the orphan alive by attaching to the skinned mesh. This is a fallback for formats
                // whose bone nodes are not present in the visible node tree after Assimp postprocess.
                p.mesh->add(orphan);
                b = orphan.get();
                boneObjects[orphan->name] = b;
            }
            b->boneIndex = static_cast<int>(i);
            skeleton->bones.push_back(b);
        }
        if (skeleton->boneInverses.size() != skeleton->bones.size()) skeleton->calculateInverses();
        skeleton->boneMatrices.resize(skeleton->bones.size(), glm::mat4(1.0f));
        p.mesh->bind(skeleton, glm::mat4(1.0f));
        p.mesh->kind = ObjectKind::SkinnedMesh;
    }
}

void AssimpLoader::convertAnimations(const aiScene* scene) {
    if (!scene || !scene->HasAnimations()) return;
    importedAnimations.reserve(scene->mNumAnimations);
    for (unsigned ai = 0; ai < scene->mNumAnimations; ++ai) {
        aiAnimation* anim = scene->mAnimations[ai];
        AnimationClip clip;
        clip.name = anim->mName.length ? anim->mName.C_Str() : ("Animation_" + std::to_string(ai));
        double ticksPerSecond = anim->mTicksPerSecond > 0.0 ? anim->mTicksPerSecond : 25.0;
        clip.duration = static_cast<float>(anim->mDuration / ticksPerSecond);
        for (unsigned ci = 0; ci < anim->mNumChannels; ++ci) {
            aiNodeAnim* ch = anim->mChannels[ci];
            const std::string nodeName = ch->mNodeName.C_Str();
            if (ch->mNumPositionKeys > 0) {
                KeyframeTrack tr; tr.targetPath = nodeName + ".position"; tr.valueType = TrackValueType::Vec3;
                tr.times.reserve(ch->mNumPositionKeys); tr.values.reserve(ch->mNumPositionKeys * 3);
                for (unsigned k = 0; k < ch->mNumPositionKeys; ++k) {
                    tr.times.push_back(static_cast<float>(ch->mPositionKeys[k].mTime / ticksPerSecond));
                    auto v = to_glm(ch->mPositionKeys[k].mValue); tr.values.insert(tr.values.end(), {v.x, v.y, v.z});
                }
                clip.tracks.push_back(std::move(tr));
            }
            if (ch->mNumRotationKeys > 0) {
                KeyframeTrack tr; tr.targetPath = nodeName + ".quaternion"; tr.valueType = TrackValueType::Quat;
                tr.times.reserve(ch->mNumRotationKeys); tr.values.reserve(ch->mNumRotationKeys * 4);
                for (unsigned k = 0; k < ch->mNumRotationKeys; ++k) {
                    tr.times.push_back(static_cast<float>(ch->mRotationKeys[k].mTime / ticksPerSecond));
                    auto q = to_glm(ch->mRotationKeys[k].mValue); tr.values.insert(tr.values.end(), {q.x, q.y, q.z, q.w});
                }
                clip.tracks.push_back(std::move(tr));
            }
            if (ch->mNumScalingKeys > 0) {
                KeyframeTrack tr; tr.targetPath = nodeName + ".scale"; tr.valueType = TrackValueType::Vec3;
                tr.times.reserve(ch->mNumScalingKeys); tr.values.reserve(ch->mNumScalingKeys * 3);
                for (unsigned k = 0; k < ch->mNumScalingKeys; ++k) {
                    tr.times.push_back(static_cast<float>(ch->mScalingKeys[k].mTime / ticksPerSecond));
                    auto v = to_glm(ch->mScalingKeys[k].mValue); tr.values.insert(tr.values.end(), {v.x, v.y, v.z});
                }
                clip.tracks.push_back(std::move(tr));
            }
        }
        for (unsigned mi = 0; mi < anim->mNumMorphMeshChannels; ++mi) {
            aiMeshMorphAnim* ch = anim->mMorphMeshChannels[mi];
            if (!ch || ch->mNumKeys == 0) continue;
            const std::string meshName = ch->mName.C_Str();
            // Assimp stores morph channels as sparse target indices + weights per key.
            // Convert each referenced target into one scalar track, matching the
            // three.js binding shape: MeshName.morphTargetInfluences[index].
            std::map<unsigned, std::vector<std::pair<float, float>>> targetKeys;
            for (unsigned k = 0; k < ch->mNumKeys; ++k) {
                const aiMeshMorphKey& key = ch->mKeys[k];
                float time = static_cast<float>(key.mTime / ticksPerSecond);
                for (unsigned wi = 0; wi < key.mNumValuesAndWeights; ++wi) {
                    unsigned targetIndex = static_cast<unsigned>(key.mValues[wi]);
                    float weight = static_cast<float>(key.mWeights[wi]);
                    targetKeys[targetIndex].push_back({time, weight});
                }
            }
            for (auto& [targetIndex, keys] : targetKeys) {
                std::sort(keys.begin(), keys.end(), [](auto& a, auto& b){ return a.first < b.first; });
                KeyframeTrack tr;
                tr.targetPath = meshName + ".morphTargetInfluences[" + std::to_string(targetIndex) + "]";
                tr.valueType = TrackValueType::Float;
                tr.times.reserve(keys.size());
                tr.values.reserve(keys.size());
                for (auto& [time, weight] : keys) { tr.times.push_back(time); tr.values.push_back(weight); }
                clip.tracks.push_back(std::move(tr));
            }
        }
        importedAnimations.push_back(std::move(clip));
    }
}

} // namespace threecpp

namespace threecpp {

void AssimpLoader::inspectSceneExtensions(const aiScene* scene) {
    if (!scene) return;
    // Assimp does not expose glTF extension arrays through one stable public API
    // across all package versions. Keep a conservative support matrix here so
    // validation output can distinguish imported extensions from staged ones.
    extensionReport.add("KHR_texture_transform", GltfExtensionStatus::Imported, "mapped to Texture::textureTransform when Assimp exposes UV transform keys");
    extensionReport.add("KHR_texture_basisu", GltfExtensionStatus::MetadataOnly, "KTX2/Basis payload is preserved; transcoder upload path is staged");
    extensionReport.add("KHR_materials_emissive_strength", GltfExtensionStatus::Imported, "mapped to MeshStandardMaterial::emissiveIntensity");
    extensionReport.add("KHR_materials_ior", GltfExtensionStatus::Imported, "mapped to MeshPhysicalMaterial::ior");
    extensionReport.add("KHR_materials_volume", GltfExtensionStatus::Imported, "mapped to thickness/attenuation fields when Assimp exposes raw keys");
    extensionReport.add("KHR_materials_transmission", GltfExtensionStatus::Imported, "mapped to MeshPhysicalMaterial::transmission");
    extensionReport.add("KHR_materials_clearcoat", GltfExtensionStatus::Imported, "mapped to clearcoat fields");
    extensionReport.add("KHR_materials_sheen", GltfExtensionStatus::Imported, "mapped to sheen fields");
    extensionReport.add("KHR_materials_specular", GltfExtensionStatus::Imported, "mapped to specular fields");
    extensionReport.add("KHR_materials_iridescence", GltfExtensionStatus::Imported, "mapped to iridescence fields");
    extensionReport.add("KHR_materials_anisotropy", GltfExtensionStatus::Imported, "mapped to anisotropy fields");
}

void AssimpLoader::inspectMaterialExtensions(aiMaterial* mat) {
    if (!mat) return;
    for (unsigned i = 0; i < mat->mNumProperties; ++i) {
        const aiMaterialProperty* prop = mat->mProperties[i];
        if (!prop) continue;
        const std::string key = prop->mKey.C_Str();
        if (key.find("texture_transform") != std::string::npos || key.find("TextureTransform") != std::string::npos) {
            ++extensionReport.textureTransforms;
        }
        if (key.find("KHR_materials_") != std::string::npos || key.find("$mat.gltf.") != std::string::npos) {
            // These are counted as physical-capable material hints rather than
            // material instances because Assimp can expose extension keys even
            // when factors remain at defaults.
            if (key.find("transmission") != std::string::npos ||
                key.find("volume") != std::string::npos ||
                key.find("clearcoat") != std::string::npos ||
                key.find("sheen") != std::string::npos ||
                key.find("specular") != std::string::npos ||
                key.find("ior") != std::string::npos ||
                key.find("iridescence") != std::string::npos ||
                key.find("anisotropy") != std::string::npos) {
                ++extensionReport.physicalMaterials;
                break;
            }
        }
    }
}

void AssimpLoader::updateTextureExtensionMetadata(Texture& tex, const std::string& rawPath) {
    std::string lower = rawPath;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    if (lower.find(".ktx2") != std::string::npos || lower.find("image/ktx2") != std::string::npos || lower.find("ktx2") != std::string::npos) {
        tex.compressed = true;
        tex.compressedContainer = CompressedTextureContainer::KTX2;
        tex.compressionScheme = "KTX2/BasisU";
        tex.generateMipmaps = false;
        ++extensionReport.ktx2Textures;
        ++extensionReport.basisPayloads;
        if (!tex.embedded && !tex.sourcePath.empty() && std::filesystem::exists(tex.sourcePath)) {
            try {
                auto kt = TextureLoader::loadKTX2Metadata(tex.sourcePath, tex.colorSpace == ColorSpace::SRGB);
                tex.width = kt->width;
                tex.height = kt->height;
                tex.vkFormat = kt->vkFormat;
                tex.typeSize = kt->typeSize;
                tex.layerCount = kt->layerCount;
                tex.faceCount = kt->faceCount;
                tex.levelCount = kt->levelCount;
                tex.supercompressionScheme = kt->supercompressionScheme;
                tex.compressedLevels = kt->compressedLevels;
                tex.pixels = kt->pixels;
                tex.compressionScheme = kt->compressionScheme;
                tex.transcodeMessage = "KTX2 payload and mip levels loaded; GL upload path will transcode or use fallback";
                tex.markNeedsUpdate();
            } catch (...) {
                // Keep metadata-only path usable even without readable KTX2 headers.
            }
        }
    }
    if (lower.find("basis") != std::string::npos || lower.find(".basis") != std::string::npos) {
        tex.compressed = true;
        tex.compressedContainer = CompressedTextureContainer::BasisUniversal;
        tex.compressionScheme = "Basis Universal";
        tex.generateMipmaps = false;
        ++extensionReport.basisPayloads;
    }
    if (tex.hasTextureTransform) ++extensionReport.textureTransforms;
}

} // namespace threecpp
