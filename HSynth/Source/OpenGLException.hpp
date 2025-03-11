#pragma once

#include <stdexcept>
#include <string>

typedef unsigned int GLenum;
#ifdef DEBUG
#define OPENGL_ERROR_HANDLE(message)                           \
    {                                                          \
        GLenum error = juce::gl::glGetError();                 \
        if (error != 0) throw OpenGLException(message, error); \
    }

#define INIT_OPENGL_CALLBACK              \
    if (juce::gl::glDebugMessageCallback) \
        juce::gl::glDebugMessageCallback(MessageCallback, 0);
#else
#define OPENGL_ERROR_HANDLE(message) ;
#define INIT_OPENGL_CALLBACK ;
#endif

class OpenGLException : public std::runtime_error {
   public:
    explicit OpenGLException(const std::string& msg, GLenum errorCode)
        : std::runtime_error(
              msg + " (OpenGL Error Code: " + std::to_string(errorCode) + ")"),
          errorCode(errorCode) {}

    GLenum getErrorCode() const { return errorCode; }

   private:
    GLenum errorCode;
};

static void MessageCallback(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLsizei length,
                            const GLchar* message, const void* userParam) {
    std::cerr << "GL DEBUG: " << message << std::endl;
}