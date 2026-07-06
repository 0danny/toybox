#pragma once

#include <glad/glad.h>

namespace Shader
{
    GLuint CompileShader(GLenum type, const char* source);
    GLuint CreateCollisionShader();
    void DestroyCollisionRenderer();
}