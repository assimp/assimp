// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
//
// Minimal OpenGL 3.3 function loader for this sample 
// (In practice use something like GLAD or GLEW instead of this hand written one)
// ----------------------------------------------------------------------------

#pragma once

#include "pch.hpp"

#ifdef _WIN32
#define SAMPLE_GL_APIENTRY __stdcall
#else
#define SAMPLE_GL_APIENTRY
#endif

#ifndef GL_VERSION_1_5
typedef std::ptrdiff_t GLsizeiptr;
typedef std::ptrdiff_t GLintptr;
#endif
#ifndef GL_VERSION_2_0
typedef char GLchar;
#endif

#ifndef GL_TEXTURE_WRAP_R
#define GL_TEXTURE_WRAP_R 0x8072
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#endif
#ifndef GL_UNIFORM_BUFFER
#define GL_UNIFORM_BUFFER 0x8A11
#define GL_INVALID_INDEX 0xFFFFFFFFu
#endif

#define SAMPLE_GL_FUNCTIONS(X)                                                                                         \
    X(void, glActiveTexture, (GLenum texture))                                                                         \
    X(void, glAttachShader, (GLuint program, GLuint shader))                                                           \
    X(void, glBindBuffer, (GLenum target, GLuint buffer))                                                              \
    X(void, glBindBufferBase, (GLenum target, GLuint index, GLuint buffer))                                            \
    X(void, glBindSampler, (GLuint unit, GLuint sampler))                                                              \
    X(void, glBindVertexArray, (GLuint array))                                                                         \
    X(void, glBufferData, (GLenum target, GLsizeiptr size, const void *data, GLenum usage))                      \
    X(void, glBufferSubData, (GLenum target, GLintptr offset, GLsizeiptr size, const void *data))          \
    X(void, glCompileShader, (GLuint shader))                                                                          \
    X(GLuint, glCreateProgram, ())                                                                                     \
    X(GLuint, glCreateShader, (GLenum type))                                                                           \
    X(void, glDeleteBuffers, (GLsizei n, const GLuint *buffers))                                                       \
    X(void, glDeleteProgram, (GLuint program))                                                                         \
    X(void, glDeleteSamplers, (GLsizei count, const GLuint *samplers))                                                 \
    X(void, glDeleteShader, (GLuint shader))                                                                           \
    X(void, glDeleteVertexArrays, (GLsizei n, const GLuint *arrays))                                                   \
    X(void, glEnableVertexAttribArray, (GLuint index))                                                                 \
    X(void, glGenBuffers, (GLsizei n, GLuint * buffers))                                                               \
    X(void, glGenerateMipmap, (GLenum target))                                                                         \
    X(void, glGenSamplers, (GLsizei count, GLuint * samplers))                                                         \
    X(void, glGenVertexArrays, (GLsizei n, GLuint * arrays))                                                           \
    X(void, glGetProgramInfoLog, (GLuint program, GLsizei bufSize, GLsizei * length, GLchar * infoLog))          \
    X(void, glGetProgramiv, (GLuint program, GLenum pname, GLint * params))                                            \
    X(void, glGetShaderInfoLog, (GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog))            \
    X(void, glGetShaderiv, (GLuint shader, GLenum pname, GLint * params))                                              \
    X(GLuint, glGetUniformBlockIndex, (GLuint program, const GLchar *uniformBlockName))                          \
    X(GLint, glGetUniformLocation, (GLuint program, const GLchar *name))                                         \
    X(void, glLinkProgram, (GLuint program))                                                                           \
    X(void, glSamplerParameteri, (GLuint sampler, GLenum pname, GLint param))                                          \
    X(void, glShaderSource, (GLuint shader, GLsizei count, const GLchar *const *string, const GLint *length))    \
    X(void, glUniform1i, (GLint location, GLint v0))                                                                   \
    X(void, glUniformBlockBinding, (GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding))             \
    X(void, glUseProgram, (GLuint program))                                                                            \
    X(void, glVertexAttribIPointer, (GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer))      \
    X(void, glVertexAttribPointer, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer))

#define SAMPLE_GL_DECLARE(ret, name, params)                    \
    using PFN_sample_##name = ret(SAMPLE_GL_APIENTRY *) params; \
    extern PFN_sample_##name sample_##name;
SAMPLE_GL_FUNCTIONS(SAMPLE_GL_DECLARE)
#undef SAMPLE_GL_DECLARE

#define glActiveTexture sample_glActiveTexture
#define glAttachShader sample_glAttachShader
#define glBindBuffer sample_glBindBuffer
#define glBindBufferBase sample_glBindBufferBase
#define glBindSampler sample_glBindSampler
#define glBindVertexArray sample_glBindVertexArray
#define glBufferData sample_glBufferData
#define glBufferSubData sample_glBufferSubData
#define glCompileShader sample_glCompileShader
#define glCreateProgram sample_glCreateProgram
#define glCreateShader sample_glCreateShader
#define glDeleteBuffers sample_glDeleteBuffers
#define glDeleteProgram sample_glDeleteProgram
#define glDeleteSamplers sample_glDeleteSamplers
#define glDeleteShader sample_glDeleteShader
#define glDeleteVertexArrays sample_glDeleteVertexArrays
#define glEnableVertexAttribArray sample_glEnableVertexAttribArray
#define glGenBuffers sample_glGenBuffers
#define glGenerateMipmap sample_glGenerateMipmap
#define glGenSamplers sample_glGenSamplers
#define glGenVertexArrays sample_glGenVertexArrays
#define glGetProgramInfoLog sample_glGetProgramInfoLog
#define glGetProgramiv sample_glGetProgramiv
#define glGetShaderInfoLog sample_glGetShaderInfoLog
#define glGetShaderiv sample_glGetShaderiv
#define glGetUniformBlockIndex sample_glGetUniformBlockIndex
#define glGetUniformLocation sample_glGetUniformLocation
#define glLinkProgram sample_glLinkProgram
#define glSamplerParameteri sample_glSamplerParameteri
#define glShaderSource sample_glShaderSource
#define glUniform1i sample_glUniform1i
#define glUniformBlockBinding sample_glUniformBlockBinding
#define glUseProgram sample_glUseProgram
#define glVertexAttribIPointer sample_glVertexAttribIPointer
#define glVertexAttribPointer sample_glVertexAttribPointer

Result<> load_gl_functions();

#ifdef SAMPLE_GL_LOADER_IMPLEMENTATION

#define SAMPLE_GL_DEFINE(ret, name, params) PFN_sample_##name sample_##name = nullptr;
SAMPLE_GL_FUNCTIONS(SAMPLE_GL_DEFINE)
#undef SAMPLE_GL_DEFINE

Result<> load_gl_functions() {
#define SAMPLE_GL_LOAD(ret, name, params)                                                \
    sample_##name = reinterpret_cast<PFN_sample_##name>(glutGetProcAddress(#name)); \
    if (sample_##name == nullptr) return "failed to load the function '" #name "'";
    SAMPLE_GL_FUNCTIONS(SAMPLE_GL_LOAD)
#undef SAMPLE_GL_LOAD
    return Ok();
}

#endif