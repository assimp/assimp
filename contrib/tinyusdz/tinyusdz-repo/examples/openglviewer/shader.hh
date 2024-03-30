// from gltf-insight
/*
MIT License

Copyright (c) 2019 Light Transport Entertainment Inc. And many contributors.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#else
#include <GLES3/gl3.h>
#endif

//#include "glm/glm.hpp"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <string>
#include <vector>
#include <array>

namespace example {

class shader {
  GLuint program_;
  std::string shader_name_;

 public:
  // default ctor
  shader();
  // delegate ctor
  shader(const char* shader_name, const std::string& vertex_shader_source_code,
         const std::string& fragment_shader_source_code)
      : shader(shader_name, vertex_shader_source_code.c_str(),
               fragment_shader_source_code.c_str()) {}
  // actual ctor
  shader(const char* shader_name, const char* vertex_shader_source_code,
         const char* fragment_shader_source_code);
  ~shader();
  shader(shader&& other);
  shader& operator=(shader&& other);
  shader& operator=(const shader&) = delete;
  shader(const shader&) = delete;

  void use(const std::array<float, 4> &hilightcol = {1.0f, 0.5f, 0.0f, 1.0f}) const;
  GLuint get_program() const;
  const char* get_name() const;

  void set_uniform(const char* name, const float value) const;
  void set_uniform(const char* name, const int value) const;
  void set_uniform(const char* name, const std::array<float, 4>& v) const;
  void set_uniform(const char* name, const std::array<float, 3>& v) const;
  void set_uniform(const char* name, const std::array<float, 16> &m) const;
  void set_uniform(const char* name, const std::array<float, 9>& m) const;
  void set_uniform(const char* name,
                   const std::vector<std::array<float, 16>>& matrices) const;
  void set_uniform(const char* name, size_t number_of_matrices,
                   float* data) const;
};

} // namespace example
