// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#pragma once

#include "pch.hpp"

namespace TextureLoader
{
  GLuint create_from_memory(const uint8_t *data, size_t dataSize);
  GLuint create_from_file(const char *fileName);
  GLuint create_from_pixels(const void *pixels, GLsizei width, GLsizei height, GLenum format);
}
