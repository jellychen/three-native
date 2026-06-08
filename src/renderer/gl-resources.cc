#include "renderer/gl-resources.h"
#include "core/renderable.h"
#include "texture/ktx2-transcoder.h"

namespace THREE {

static GLenum gl_type(AttributeType t) {
    switch (t) {
        case AttributeType::Float32: return GL_FLOAT;
        case AttributeType::Uint32: return GL_UNSIGNED_INT;
        case AttributeType::Int32: return GL_INT;
        case AttributeType::Uint16: return GL_UNSIGNED_SHORT;
        case AttributeType::Int16: return GL_SHORT;
        case AttributeType::Uint8: return GL_UNSIGNED_BYTE;
        case AttributeType::Int8: return GL_BYTE;
    }
    return GL_FLOAT;
}

static int location_for_name(const std::string& name) {
    if (name == "position") return 0;
    if (name == "normal" || name == "instanceStart") return 1;
    if (name == "uv" || name == "instanceEnd") return 2;
    if (name == "color") return 3;
    if (name == "uv2") return 15;
    if (name == "skinIndex") return 4;
    if (name == "skinWeight") return 5;
    if (name == "lineDistance") return 6;
    if (name.rfind("morphTarget", 0) == 0) {
        int idx = std::atoi(name.c_str() + 11);
        if (idx >= 0 && idx < 4) return 7 + idx;
    }
    if (name.rfind("morphNormal", 0) == 0) {
        int idx = std::atoi(name.c_str() + 11);
        if (idx >= 0 && idx < 4) return 11 + idx;
    }
    return -1;
}

static GLenum texture_internal_format(TextureFormat fmt) {
    switch (fmt) {
        case TextureFormat::SRGB8Alpha8: return GL_SRGB8_ALPHA8;
        case TextureFormat::RGBA8: return GL_RGBA8;
        case TextureFormat::RGB16F: return GL_RGB16F;
        case TextureFormat::RGBA16F: return GL_RGBA16F;
        case TextureFormat::Depth24Stencil8: return GL_DEPTH24_STENCIL8;
    }
    return GL_RGBA8;
}

static GLenum texture_format(TextureFormat fmt, int channels) {
    if (fmt == TextureFormat::RGB16F) return GL_RGB;
    if (channels == 1) return GL_RED;
    if (channels == 2) return GL_RG;
    if (channels == 3) return GL_RGB;
    return GL_RGBA;
}

static GLenum texture_type(TextureFormat fmt) {
    if (fmt == TextureFormat::RGB16F || fmt == TextureFormat::RGBA16F) return GL_HALF_FLOAT;
    return GL_UNSIGNED_BYTE;
}

static GLenum texture_wrap(TextureWrap w) {
    switch (w) {
        case TextureWrap::Repeat: return GL_REPEAT;
        case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
        case TextureWrap::ClampToEdge: return GL_CLAMP_TO_EDGE;
    }
    return GL_CLAMP_TO_EDGE;
}


static std::vector<std::byte> make_flipped_y_copy(const Texture& t) {
    if (t.height <= 1 || t.pixels.empty()) return t.pixels;
    const std::size_t h = static_cast<std::size_t>(t.height);
    const std::size_t rowBytes = t.pixels.size() / h;
    if (rowBytes == 0 || rowBytes * h != t.pixels.size()) return t.pixels;
    std::vector<std::byte> flipped(t.pixels.size());
    for (std::size_t y = 0; y < h; ++y) {
        const std::byte* src = t.pixels.data() + (h - 1u - y) * rowBytes;
        std::byte* dst = flipped.data() + y * rowBytes;
        std::copy(src, src + rowBytes, dst);
    }
    return flipped;
}

static GLenum texture_filter(TextureFilter f, bool minification) {
    switch (f) {
        case TextureFilter::Nearest: return GL_NEAREST;
        case TextureFilter::Linear: return GL_LINEAR;
        case TextureFilter::LinearMipmapLinear: return minification ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
    }
    return GL_LINEAR;
}

GLResourceManager::~GLResourceManager() { clear(); }

GLGeometryResource& GLResourceManager::getOrCreateGeometry(const BufferGeometry& g) {
    auto it = geometries.find(g.id);
    if (it != geometries.end()) {
        if (it->second.sourceVersion == g.version) return it->second;
        disposeGeometry(g.id);
    }

    GLGeometryResource res;
    res.sourceVersion = g.version;
    glGenVertexArrays(1, &res.vao);
    glBindVertexArray(res.vao);

    auto uploadAttribute = [&](const std::string& name, const BufferAttribute& attr) {
        int loc = location_for_name(name);
        if (loc < 0) return;
        GLuint buffer = 0;
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(attr.data.size()), attr.data.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(static_cast<GLuint>(loc));
        glVertexAttribPointer(static_cast<GLuint>(loc), attr.itemSize, gl_type(attr.type), attr.normalized ? GL_TRUE : GL_FALSE, 0, nullptr);
        res.buffers.push_back(buffer);
    };

    for (const auto& [name, attr] : g.attributes) uploadAttribute(name, attr);

    if (auto it = g.morphAttributes.find("position"); it != g.morphAttributes.end()) {
        const int maxMorphs = std::min<int>(4, static_cast<int>(it->second.size()));
        for (int i = 0; i < maxMorphs; ++i) uploadAttribute("morphTarget" + std::to_string(i), it->second[static_cast<std::size_t>(i)]);
    }
    if (auto it = g.morphAttributes.find("normal"); it != g.morphAttributes.end()) {
        const int maxMorphs = std::min<int>(4, static_cast<int>(it->second.size()));
        for (int i = 0; i < maxMorphs; ++i) uploadAttribute("morphNormal" + std::to_string(i), it->second[static_cast<std::size_t>(i)]);
    }

    if (!g.indices.empty()) {
        res.hasIndex = true;
        res.indexCount = static_cast<int>(g.indices.size());
        glGenBuffers(1, &res.indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, res.indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(g.indices.size() * sizeof(std::uint32_t)), g.indices.data(), GL_STATIC_DRAW);
    }
    res.vertexCount = g.vertexCount();
    glBindVertexArray(0);
    auto [inserted, _] = geometries.emplace(g.id, std::move(res));
    return inserted->second;
}

GLTextureResource& GLResourceManager::getOrCreateTexture(const Texture& t) {
    auto it = textures.find(t.id);
    if (it != textures.end()) {
        if (it->second.sourceVersion == t.version) return it->second;
        disposeTexture(t.id);
    }

    GLTextureResource res;
    res.sourceVersion = t.version;
    res.target = GL_TEXTURE_2D;
    res.width = t.width;
    res.height = t.height;
    glGenTextures(1, &res.id);
    glBindTexture(GL_TEXTURE_2D, res.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture_wrap(t.wrapS));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture_wrap(t.wrapT));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture_filter(t.minFilter, true));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture_filter(t.magFilter, false));

    if (t.compressed && t.compressedContainer == CompressedTextureContainer::KTX2) {
        KTX2TranscodeOptions opts;
        opts.preferSRGB = t.colorSpace == ColorSpace::SRGB;
        auto result = KTX2Transcoder::transcode(t, CompressedTextureCapabilities::queryGL(), opts);
        if (result.success && result.usedCompressedUpload && !result.levels.empty()) {
            const GLenum internal = KTX2Transcoder::glInternalFormat(result.gpuFormat, t.colorSpace == ColorSpace::SRGB);
            for (const auto& level : result.levels) {
                if (level.data.empty()) continue;
                glCompressedTexImage2D(GL_TEXTURE_2D,
                                       static_cast<GLint>(level.level),
                                       internal,
                                       static_cast<GLsizei>(level.width),
                                       static_cast<GLsizei>(level.height),
                                       0,
                                       static_cast<GLsizei>(level.data.size()),
                                       level.data.data());
            }
        } else if (result.success && result.usedRGBAFallback && !result.rgbaPixels.empty()) {
            const GLenum internal = t.colorSpace == ColorSpace::SRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
            glTexImage2D(GL_TEXTURE_2D, 0, internal, std::max(1, result.width), std::max(1, result.height), 0, GL_RGBA, GL_UNSIGNED_BYTE, result.rgbaPixels.data());
            if (t.generateMipmaps) glGenerateMipmap(GL_TEXTURE_2D);
        } else {
            const std::uint8_t magenta[4] = {255, 0, 255, 255};
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, magenta);
        }
    } else if (t.width > 0 && t.height > 0 && !t.pixels.empty()) {
        const void* pixelData = t.pixels.data();
        std::vector<std::byte> flippedPixels;
        if (t.flipY) {
            flippedPixels = make_flipped_y_copy(t);
            pixelData = flippedPixels.data();
        }
        glTexImage2D(GL_TEXTURE_2D, 0, texture_internal_format(t.format), t.width, t.height, 0, texture_format(t.format, t.channels), texture_type(t.format), pixelData);
        if (t.generateMipmaps) glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        const std::uint8_t white[4] = {255, 255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    auto [inserted, _] = textures.emplace(t.id, std::move(res));
    return inserted->second;
}


GLTextureResource& GLResourceManager::getOrCreateCubeTexture(const CubeTexture& t) {
    auto it = textures.find(t.id);
    if (it != textures.end()) {
        if (it->second.sourceVersion == t.version) return it->second;
        disposeTexture(t.id);
    }

    GLTextureResource res;
    res.sourceVersion = t.version;
    res.target = GL_TEXTURE_CUBE_MAP;
    res.width = t.width;
    res.height = t.height;
    glGenTextures(1, &res.id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, res.id);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, t.mipLevels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, std::max(0, t.mipLevels - 1));

    const GLenum fmt = texture_format(t.format, t.channels);
    const GLenum internal = texture_internal_format(t.format);
    const GLenum typ = texture_type(t.format);
    int levels = std::max(1, t.mipLevels);
    for (int mip = 0; mip < levels; ++mip) {
        int size = std::max(1, t.width >> mip);
        for (int face = 0; face < 6; ++face) {
            const std::vector<std::byte>* src = nullptr;
            if (!t.mipFaces.empty() && mip < static_cast<int>(t.mipFaces.size())) {
                src = &t.mipFaces[static_cast<std::size_t>(mip)][static_cast<std::size_t>(face)];
            } else if (mip == 0) {
                src = &t.faces[static_cast<std::size_t>(face)];
            }
            const void* data = (src && !src->empty()) ? src->data() : nullptr;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, mip, internal, size, size, 0, fmt, typ, data);
        }
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    auto [inserted, _] = textures.emplace(t.id, std::move(res));
    return inserted->second;
}

void GLResourceManager::bindTexture2D(const std::shared_ptr<Texture>& texture, GLint uniformLocation, int unit, bool normalFallback) {
    if (uniformLocation < 0) return;
    glActiveTexture(GL_TEXTURE0 + unit);
    GLuint id = 0;
    if (texture) id = getOrCreateTexture(*texture).id;
    else id = normalFallback ? getFallbackNormal() : getFallbackWhite();
    glBindTexture(GL_TEXTURE_2D, id);
    glUniform1i(uniformLocation, unit);
}


void GLResourceManager::bindCubeTexture(const std::shared_ptr<CubeTexture>& texture, GLint uniformLocation, int unit) {
    if (uniformLocation < 0) return;
    glActiveTexture(GL_TEXTURE0 + unit);
    GLuint id = 0;
    if (texture) id = getOrCreateCubeTexture(*texture).id;
    else id = getFallbackWhite();
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
    glUniform1i(uniformLocation, unit);
}


GLInstanceResource& GLResourceManager::updateInstancedAttributes(InstancedMesh& mesh) {
    GLInstanceResource& res = instances[mesh.id];
    const int count = std::max(0, mesh.count);
    res.count = count;

    if (res.matrixBuffer == 0) glGenBuffers(1, &res.matrixBuffer);
    if (res.colorBuffer == 0) glGenBuffers(1, &res.colorBuffer);

    if (mesh.instanceMatrixNeedsUpdate || res.matrixVersion != mesh.instanceMatrixVersion) {
        glBindBuffer(GL_ARRAY_BUFFER, res.matrixBuffer);
        const std::size_t matrixBytes = static_cast<std::size_t>(count) * sizeof(glm::mat4);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(matrixBytes), mesh.instanceMatrices.empty() ? nullptr : mesh.instanceMatrices.data(), GL_DYNAMIC_DRAW);
        res.matrixVersion = mesh.instanceMatrixVersion;
        mesh.instanceMatrixNeedsUpdate = false;
    }

    if (mesh.instanceColorNeedsUpdate || res.colorVersion != mesh.instanceColorVersion) {
        glBindBuffer(GL_ARRAY_BUFFER, res.colorBuffer);
        const std::size_t colorBytes = static_cast<std::size_t>(count) * sizeof(glm::vec4);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(colorBytes), mesh.instanceColors.empty() ? nullptr : mesh.instanceColors.data(), GL_DYNAMIC_DRAW);
        res.colorVersion = mesh.instanceColorVersion;
        mesh.instanceColorNeedsUpdate = false;
    }

    // Attach per-instance state to the currently bound geometry VAO. This mirrors
    // three.js InstancedBufferAttribute behavior: the geometry VAO remains cached,
    // while each InstancedMesh can rebind its own matrix/color buffers before draw.
    glBindBuffer(GL_ARRAY_BUFFER, res.matrixBuffer);
    const GLsizei stride = static_cast<GLsizei>(sizeof(glm::mat4));
    for (GLuint i = 0; i < 4; ++i) {
        const GLuint loc = 11u + i;
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(static_cast<std::uintptr_t>(sizeof(glm::vec4) * i)));
        glVertexAttribDivisor(loc, 1);
    }

    glBindBuffer(GL_ARRAY_BUFFER, res.colorBuffer);
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(glm::vec4)), nullptr);
    glVertexAttribDivisor(6, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return res;
}

GLuint GLResourceManager::getFallbackWhite() {
    if (fallbackWhite) return fallbackWhite;
    const std::uint8_t white[4] = {255, 255, 255, 255};
    glGenTextures(1, &fallbackWhite);
    glBindTexture(GL_TEXTURE_2D, fallbackWhite);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    return fallbackWhite;
}

GLuint GLResourceManager::getFallbackNormal() {
    if (fallbackNormal) return fallbackNormal;
    const std::uint8_t normal[4] = {128, 128, 255, 255};
    glGenTextures(1, &fallbackNormal);
    glBindTexture(GL_TEXTURE_2D, fallbackNormal);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, normal);
    return fallbackNormal;
}

void GLResourceManager::disposeGeometry(ObjectId id) {
    auto it = geometries.find(id);
    if (it == geometries.end()) return;
    auto& r = it->second;
    for (auto b : r.buffers) glDeleteBuffers(1, &b);
    if (r.indexBuffer) glDeleteBuffers(1, &r.indexBuffer);
    if (r.vao) glDeleteVertexArrays(1, &r.vao);
    geometries.erase(it);
}

void GLResourceManager::disposeInstance(ObjectId id) {
    auto it = instances.find(id);
    if (it == instances.end()) return;
    if (it->second.matrixBuffer) glDeleteBuffers(1, &it->second.matrixBuffer);
    if (it->second.colorBuffer) glDeleteBuffers(1, &it->second.colorBuffer);
    instances.erase(it);
}

void GLResourceManager::disposeTexture(ObjectId id) {
    auto it = textures.find(id);
    if (it == textures.end()) return;
    if (it->second.id) glDeleteTextures(1, &it->second.id);
    textures.erase(it);
}

void GLResourceManager::clear() {
    std::vector<ObjectId> geometryIds;
    geometryIds.reserve(geometries.size());
    for (auto& [id, _] : geometries) geometryIds.push_back(id);
    for (auto id : geometryIds) disposeGeometry(id);

    std::vector<ObjectId> textureIds;
    textureIds.reserve(textures.size());
    for (auto& [id, _] : textures) textureIds.push_back(id);
    for (auto id : textureIds) disposeTexture(id);

    std::vector<ObjectId> instanceIds;
    instanceIds.reserve(instances.size());
    for (auto& [id, _] : instances) instanceIds.push_back(id);
    for (auto id : instanceIds) disposeInstance(id);

    if (fallbackWhite) { glDeleteTextures(1, &fallbackWhite); fallbackWhite = 0; }
    if (fallbackNormal) { glDeleteTextures(1, &fallbackNormal); fallbackNormal = 0; }
}

} // namespace THREE
