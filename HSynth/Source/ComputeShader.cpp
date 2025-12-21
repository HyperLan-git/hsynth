#include "ComputeShader.hpp"

ComputeShader::ComputeShader(const char* code,
                             std::initializer_list<GLsizeiptr> bufferSizes)
    : program(juce::gl::glCreateProgram()) {
    juce::Logger::writeToLog("Creating compute shader");
    buffers.reserve(bufferSizes.size());
    juce::Logger::writeToLog("Creating compute shader buffers");
    for (auto& sz : bufferSizes) {
        buffers.emplace_back(sz);
    }
    compile(code);
}

void ComputeShader::compile(const char* code) {
    juce::Logger::writeToLog("Compiling compute shader");
    using namespace juce::gl;
    uint32_t shader = juce::gl::glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);

    GLint res = 0;
    constexpr int logLength = 1024;
    char shaderErrorMessage[logLength] = {0};

    glGetShaderiv(shader, GL_COMPILE_STATUS, &res);

    if (!res) {
        glGetShaderInfoLog(shader, logLength, NULL, shaderErrorMessage);
        throw std::runtime_error(shaderErrorMessage);
    }
    if (linked) {
        GLuint old;
        glGetAttachedShaders(program, 1, nullptr, &old);
        if (old) {
            glDetachShader(program, old);
            glDeleteShader(old);
        }
    }
    glAttachShader(program, shader);
    juce::Logger::writeToLog("Linking compute shader");
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &res);

    if (!res) {
        glGetProgramInfoLog(program, logLength, NULL, shaderErrorMessage);
        throw std::runtime_error(shaderErrorMessage);
    }
    linked = true;
    OPENGL_ERROR_HANDLE("Error when creating shader !");
}

void applyUniform(const struct Uniform& uniform) {
    switch (uniform.type) {
        case Uniform::Type::INT:
            juce::gl::glUniform1i(uniform.location, uniform.value.i);
            break;
        case Uniform::Type::FLOAT:
            juce::gl::glUniform1f(uniform.location, uniform.value.f);
            break;
        case Uniform::Type::DOUBLE:
            juce::gl::glUniform1d(uniform.location, uniform.value.d);
            break;
        case Uniform::Type::I64:
            juce::gl::glUniform1i64ARB(uniform.location, uniform.value.i64);
            break;
        case Uniform::Type::U64:
            juce::gl::glUniform1ui64ARB(uniform.location, uniform.value.u64);
            break;
        case Uniform::Type::UINT:
            juce::gl::glUniform1ui(uniform.location, uniform.value.u);
            break;
    }
}

void ComputeShader::run(uint16_t x, uint16_t y, uint16_t z) {
    using namespace juce::gl;
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    glUseProgram(program);
    /*if (nUniforms > 0)
        for (std::size_t i = 0; i < nUniforms; i++) applyUniform(uniforms[i]);*/

    for (uint16_t i = 0; i < buffers.size(); i++) {
        buffers[i].bindBuffer(i + 1);
    }
    glDispatchCompute(x, y, z);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    glUseProgram(0);
    OPENGL_ERROR_HANDLE("Error when running shader !");
}

int getShaderMaxWorkGroups() {
    using namespace juce::gl;
    int x;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &x);
    OPENGL_ERROR_HANDLE("Error when getting max compute shader work groups !");
    return x;
}

void getShaderMaxWorkGroupSizes(int& x, int& y, int& z) {
    using namespace juce::gl;
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &x);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &y);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &z);
    OPENGL_ERROR_HANDLE(
        "Error when getting max compute shader work group sizes !");
}