#pragma once
#include "common.hpp"
#include "platform/gl_headers.hpp"

namespace threecpp {

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

} // namespace threecpp
