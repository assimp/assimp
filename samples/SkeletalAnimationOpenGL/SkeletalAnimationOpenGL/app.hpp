// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#pragma once

#include <glad/glad.h>

#include <string>
#include <stdint.h>

class App
{
public:  
  App() = default;
  App(const App &) = delete;
  App &operator=(const App &) = delete;

  ~App();

  bool on_create(const std::string& model_path,
    const std::string& vertex_shader_path,
    const std::string& fragment_shader_path);

  void on_render();
  void on_resize(int32_t width, int32_t height);
};