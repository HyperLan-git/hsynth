#pragma once

#include "GLBuffer.hpp"
#include <vector>
#include <initializer_list>

struct Uniform {
    enum Type { INT, FLOAT, I64, UINT, DOUBLE, U64 } type;
    union {
        int i;
        float f;
        int64_t i64;
        uint u;
        double d;
        uint64_t u64;
    } value;
    int location;
};

class ComputeShader {
   public:
    explicit ComputeShader(const char* code,
                           std::initializer_list<GLsizeiptr> bufferSizes);

    void compile(const char* code);

    void run(uint16_t x, uint16_t y = 1, uint16_t z = 1);

    inline GLBuffer& getBuffer(uint16_t i) { return buffers[i]; }

    ~ComputeShader() { juce::gl::glDeleteProgram(program); }

   private:
    uint16_t program = 0;
    std::vector<GLBuffer> buffers;
    bool linked = false;

    JUCE_DECLARE_NON_COPYABLE(ComputeShader);
};