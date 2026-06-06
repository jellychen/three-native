#include "renderer/GLProgram.hpp"
#include <iostream>

namespace threecpp {

GLProgram::GLProgram(ProgramKey k, std::string_view vertex, std::string_view fragment) : key(k) {
    GLuint vs = compile(GL_VERTEX_SHADER, vertex);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragment);
    id = glCreateProgram();
    glAttachShader(id, vs); glAttachShader(id, fs);
    glLinkProgram(id);
    GLint ok = 0; glGetProgramiv(id, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096]; GLsizei len = 0; glGetProgramInfoLog(id, sizeof(log), &len, log);
        throw std::runtime_error(std::string("GL program link failed: ") + std::string(log, len));
    }
    glDeleteShader(vs); glDeleteShader(fs);
}

GLProgram::GLProgram(GLProgram&& o) noexcept { *this = std::move(o); }
GLProgram& GLProgram::operator=(GLProgram&& o) noexcept { if (this != &o) { id = o.id; key = o.key; uniforms = std::move(o.uniforms); o.id = 0; } return *this; }
GLProgram::~GLProgram() { if (id) glDeleteProgram(id); }

GLuint GLProgram::compile(GLenum type, std::string_view src) {
    GLuint s = glCreateShader(type);
    const char* c = src.data(); GLint len = static_cast<GLint>(src.size());
    glShaderSource(s, 1, &c, &len);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096]; GLsizei out = 0; glGetShaderInfoLog(s, sizeof(log), &out, log);
        throw std::runtime_error(std::string("GL shader compile failed: ") + std::string(log, out));
    }
    return s;
}

GLint GLProgram::uniform(const std::string& name) {
    auto it = uniforms.find(name);
    if (it != uniforms.end()) return it->second;
    GLint loc = glGetUniformLocation(id, name.c_str());
    uniforms[name] = loc;
    return loc;
}

} // namespace threecpp
