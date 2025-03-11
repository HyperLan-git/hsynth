#include "GLBuffer.hpp"

GLBuffer::GLBuffer(GLsizeiptr sz, GLenum usage, void* data, GLenum type)
    : type(type), size(sz) {
    using namespace juce::gl;
    glGenBuffers(1, &buffer);
    glBindBuffer(type, buffer);
    glBufferData(type, sz, data, usage);
    OPENGL_ERROR_HANDLE("Error when allocating buffer !");
}

void GLBuffer::bindBuffer(uint16_t idx) {
    using namespace juce::gl;
    glBindBuffer(type, buffer);
    glBindBufferBase(type, idx, buffer);
    OPENGL_ERROR_HANDLE("Error when binding buffer !");
}

GLBuffer::~GLBuffer() {
    juce::gl::glDeleteBuffers(1, &buffer);
    try {
        OPENGL_ERROR_HANDLE("Error when deleting buffer !");
    } catch (const OpenGLException& e) {
        std::cerr << e.what() << std::endl;
    }
}