#pragma once
#include "common.hpp"
#include "geometry/BufferGeometry.hpp"
#include "texture/Texture.hpp"
#include "platform/gl_headers.hpp"

namespace threecpp {

class InstancedMesh;

struct GLGeometryResource {
    GLuint vao = 0;
    std::vector<GLuint> buffers;
    GLuint indexBuffer = 0;
    int vertexCount = 0;
    int indexCount = 0;
    bool hasIndex = false;
    std::uint64_t sourceVersion = std::numeric_limits<std::uint64_t>::max();
};

struct GLInstanceResource {
    GLuint matrixBuffer = 0;
    GLuint colorBuffer = 0;
    int count = 0;
    std::uint64_t matrixVersion = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t colorVersion = std::numeric_limits<std::uint64_t>::max();
};

struct GLTextureResource {
    GLuint id = 0;
    GLenum target = GL_TEXTURE_2D;
    int width = 0;
    int height = 0;
    std::uint64_t sourceVersion = std::numeric_limits<std::uint64_t>::max();
};

class GLResourceManager {
    std::unordered_map<ObjectId, GLGeometryResource> geometries;
    std::unordered_map<ObjectId, GLTextureResource> textures;
    std::unordered_map<ObjectId, GLInstanceResource> instances;
    GLuint fallbackWhite = 0;
    GLuint fallbackNormal = 0;

public:
    ~GLResourceManager();
    GLGeometryResource& getOrCreateGeometry(const BufferGeometry& geometry);
    GLTextureResource& getOrCreateTexture(const Texture& texture);
    GLTextureResource& getOrCreateCubeTexture(const CubeTexture& texture);
    GLInstanceResource& updateInstancedAttributes(InstancedMesh& mesh);
    void bindTexture2D(const std::shared_ptr<Texture>& texture, GLint uniformLocation, int unit, bool normalFallback = false);
    void bindCubeTexture(const std::shared_ptr<CubeTexture>& texture, GLint uniformLocation, int unit);
    void disposeGeometry(ObjectId id);
    void disposeTexture(ObjectId id);
    void disposeInstance(ObjectId id);
    void clear();
    int geometryCacheSize() const { return static_cast<int>(geometries.size()); }
    int textureCacheSize() const { return static_cast<int>(textures.size()); }
    int instanceCacheSize() const { return static_cast<int>(instances.size()); }

private:
    GLuint getFallbackWhite();
    GLuint getFallbackNormal();
};

} // namespace threecpp
