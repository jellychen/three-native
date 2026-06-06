#pragma once
#include "core/Object3D.hpp"

namespace threecpp {

class Bone : public Object3D {
public:
    int boneIndex = -1;
    Bone() { kind = ObjectKind::Bone; }
};

class Skeleton {
public:
    std::vector<Bone*> bones;
    std::vector<glm::mat4> boneInverses;
    std::vector<glm::mat4> boneMatrices;

    void calculateInverses() {
        boneInverses.resize(bones.size());
        for (std::size_t i = 0; i < bones.size(); ++i) boneInverses[i] = glm::inverse(bones[i]->matrixWorld);
        boneMatrices.resize(bones.size(), glm::mat4(1.0f));
    }

    void update() {
        if (boneMatrices.size() != bones.size()) boneMatrices.resize(bones.size(), glm::mat4(1.0f));
        for (std::size_t i = 0; i < bones.size(); ++i) {
            const glm::mat4 inv = i < boneInverses.size() ? boneInverses[i] : glm::mat4(1.0f);
            boneMatrices[i] = bones[i]->matrixWorld * inv;
        }
    }
};

} // namespace threecpp
