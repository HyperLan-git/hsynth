#pragma once

#include "JuceHeader.h"
#include "OpenGLException.hpp"

//static void unbindBuffer(GLenum type) { juce::gl::glBindBuffer(type, 0); }

class GLBuffer {
   public:
    explicit GLBuffer(GLsizeiptr sz, GLenum usage = juce::gl::GL_DYNAMIC_READ,
                      void* data = nullptr,
                      GLenum type = juce::gl::GL_SHADER_STORAGE_BUFFER);

    GLBuffer(const GLBuffer&) = delete;
    GLBuffer(GLBuffer&&) = default;

    void bindBuffer(uint16_t idx);

    inline void unbind() { juce::gl::glBindBuffer(type, 0); }

    inline void* mapToPtr(GLsizeiptr sz,
                          GLenum usage = juce::gl::GL_MAP_READ_BIT,
                          GLintptr offset = 0) {
        using namespace juce::gl;
        glBindBuffer(type, buffer);
        return glMapBufferRange(type, offset, sz, usage);
        // return glMapBuffer(type, usage);
    }

    // Returns false if memory corruption or some other error happened
    inline bool unMap() { return juce::gl::glUnmapBuffer(type); }

    inline GLsizeiptr getSize() const { return size; }

    ~GLBuffer();

   private:
    uint32_t buffer;
    GLenum type;
    GLsizeiptr size;
};