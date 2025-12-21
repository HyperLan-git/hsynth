#pragma once

#include <initializer_list>
#include <vector>

#include "GLBuffer.hpp"

struct Uniform {
    enum Type { INT, FLOAT, I64, UINT, DOUBLE, U64 } type;
    union {
        int i;
        float f;
        int64_t i64;
        uint32_t u;
        double d;
        uint64_t u64;
    } value;
    int location;
};

int getShaderMaxWorkGroups();
void getShaderMaxWorkGroupSizes(int& x, int& y, int& z);

class ComputeShader {
   public:
    explicit ComputeShader(const char* code,
                           std::initializer_list<GLsizeiptr> bufferSizes);

    void compile(const char* code);

    void run(uint16_t x, uint16_t y = 1, uint16_t z = 1);

    inline GLBuffer& getBuffer(uint16_t i) { return buffers[i]; }

    ~ComputeShader() { juce::gl::glDeleteProgram(program); }

   private:
    uint32_t program = 0;
    std::vector<GLBuffer> buffers;
    bool linked = false;

    JUCE_DECLARE_NON_COPYABLE(ComputeShader);
};