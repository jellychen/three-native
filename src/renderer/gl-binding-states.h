#pragma once
#include "common.h"
#include "platform/gl-headers.h"

namespace THREE {

class GLBindingStates {
    GLuint currentVAO = 0;
public:
    void bindVertexArray(GLuint vao) {
        if (currentVAO == vao) return;
        currentVAO = vao;
        glBindVertexArray(vao);
    }
    void reset() { currentVAO = 0; glBindVertexArray(0); }
};

} // namespace THREE
