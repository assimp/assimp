// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#include "app.hpp"

bool App::on_create(const std::string& model_path,
    const std::string& vertex_shader_path,
    const std::string& fragment_shader_path)
{
  return true;
}

App::~App()
{

}

void App::on_render()
{
  glClearColor(100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void App::on_resize(int32_t width, int32_t height)
{
  glViewport(0, 0, width, height);
}
