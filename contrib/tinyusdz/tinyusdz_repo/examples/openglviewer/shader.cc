// From gltf-insight
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
#include "shader.hh"

#include <iostream>
#include <stdexcept>

//#include "glm/gtc/type_ptr.hpp"
//#include "glm/matrix.hpp"

namespace example {

shader::shader(shader&& other) { *this = std::move(other); }

shader& shader::operator=(shader&& other) {
  program_ = other.program_;
  shader_name_ = std::move(other.shader_name_);
  other.program_ = 0;
  return *this;
}

shader::~shader() {
  if (glIsProgram(program_) == GL_TRUE) glDeleteProgram(program_);
}

shader::shader() {}

shader::shader(const char* shader_name, const char* vertex_shader_source_code,
               const char* fragment_shader_source_code)
    : shader_name_(shader_name) {
  std::cout << "Creating " << shader_name << "\n";

  const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  program_ = glCreateProgram();

#ifdef __EMSCRIPTEN__
  const std::string shader_preamble =
      "#version 300 es\nprecision mediump float;\n";
#else
  const std::string shader_preamble = "#version 330\n";
#endif

  // prepend version header
  std::string vtx_source =
      shader_preamble + std::string(vertex_shader_source_code);
  std::string frag_source =
      shader_preamble + std::string(fragment_shader_source_code);
  const GLchar* vtx_source_ptrs[1] = {vtx_source.c_str()};
  const GLchar* frag_source_ptrs[1] = {frag_source.c_str()};

  // Load source code
  glShaderSource(vertex_shader, 1, vtx_source_ptrs, nullptr);

  glShaderSource(fragment_shader, 1, frag_source_ptrs, nullptr);

  // Compile shader
  GLint success = 0;
  char info_log[512];

  glCompileShader(vertex_shader);
  glCompileShader(fragment_shader);

  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertex_shader, sizeof info_log, nullptr, info_log);
    std::cout << shader_name_ << " vertex shader build error: " << info_log
              << '\n';
    throw std::runtime_error("Cannot build vertex shader for " + shader_name_ +
                             " : " + std::string(info_log));
  }
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragment_shader, sizeof info_log, nullptr, info_log);
    std::cout << shader_name_ << " fragment shader build error: " << info_log
              << '\n';
    throw std::runtime_error("Cannot build fragment shader for " +
                             shader_name_ + " : " + std::string(info_log));
  }

  // Link shader
  glAttachShader(program_, vertex_shader);
  glAttachShader(program_, fragment_shader);
  glLinkProgram(program_);

  glGetProgramiv(program_, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(program_, sizeof info_log, nullptr, info_log);
    std::cout << info_log << "\n";
  }

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
}

void shader::use(const std::array<float, 4> &hightcol) const {
  glUseProgram(program_);
  set_uniform("highlight_color", hightcol);
}

const char* shader::get_name() const { return shader_name_.c_str(); }

void shader::set_uniform(const char* name, const float value) const {
  if (!name) return;

  const auto location = glGetUniformLocation(program_, name);
  if (location != -1) glUniform1f(location, value);
#if defined(UNIFORM_DEBUG_VERBOSE) && (defined(DEBUG) || defined(_DEBUG))
  else
    std::cerr << "Warn: uniform " << name << " cannot be set in shader "
              << shader_name_ << "\n";
#endif
}

void shader::set_uniform(const char* name, const int value) const {
  if (!name) return;

  const auto location = glGetUniformLocation(program_, name);
  if (location != -1) glUniform1i(location, value);
#if defined(UNIFORM_DEBUG_VERBOSE) && (defined(DEBUG) || defined(_DEBUG))
  else
    std::cerr << "Warn: uniform " << name << " cannot be set in shader "
              << shader_name_ << "\n";
#endif
}

void shader::set_uniform(const char* name, const std::array<float, 4>& v) const {
  if (!name) return;

  const auto location = glGetUniformLocation(program_, name);
  if (location != -1) glUniform4f(location, v[0], v[1], v[2], v[3]);
#if defined(UNIFORM_DEBUG_VERBOSE) && (defined(DEBUG) || defined(_DEBUG))
  else
    std::cerr << "Warn: uniform " << name << " cannot be set in shader "
              << shader_name_ << "\n";
#endif
}

void shader::set_uniform(const char* name, const std::array<float, 3>& v) const {
  if (!name) return;

  const auto location = glGetUniformLocation(program_, name);
  if (location != -1) glUniform3f(location, v[0], v[1], v[2]);
#if defined(UNIFORM_DEBUG_VERBOSE) && (defined(DEBUG) || defined(_DEBUG))
  else
    std::cerr << "Warn: uniform " << name << " cannot be set in shader "
              << shader_name_ << "\n";
#endif
}

void shader::set_uniform(const char* name, const std::array<float, 16>& m) const {
  if (!name) return;
  const auto location = glGetUniformLocation(program_, name);

  if (location != -1)
    glUniformMatrix4fv(location, 1, GL_FALSE, &m[0]);
#if defined(UNIFORM_DEBUG_VERBOSE) && (defined(DEBUG) || defined(_DEBUG))
  else
    std::cerr << "Warn: uniform " << name << " cannot be set in shader "
              << shader_name_ << "\n";
#endif
}

void shader::set_uniform(const char* name, const std::array<float, 9>& m) const {
  if (!name) return;
  const auto location = glGetUniformLocation(program_, name);

  if (location != -1)
    glUniformMatrix3fv(location, 1, GL_FALSE, &m[0]);
#if defined(UNIFORM_DEBUG_VERBOSE) && (defined(DEBUG) || defined(_DEBUG))
  else
    std::cerr << "Warn: uniform " << name << " cannot be set in shader "
              << shader_name_ << "\n";
#endif
}

void shader::set_uniform(const char* name,
                         const std::vector<std::array<float, 16>>& matrices) const {
  if (!name) return;
  if (matrices.empty()) return;

  const auto location = glGetUniformLocation(program_, name);
  if (location != -1)
    glUniformMatrix4fv(location, GLsizei(matrices.size()), GL_FALSE,
                       &matrices[0][0]);
#if defined(UNIFORM_DEBUG_VERBOSE) && (defined(DEBUG) || defined(_DEBUG))
  else
    std::cerr << "Warn: uniform " << name << " cannot be set in shader "
              << shader_name_ << "\n";
#endif
}

void shader::set_uniform(const char* name, size_t number_of_matrices,
                         float* data) const {
  if (!name) return;
  if (!number_of_matrices) return;
  if (!data) return;

  const auto location = glGetUniformLocation(program_, name);
  if (location != -1)
    glUniformMatrix4fv(location, GLsizei(number_of_matrices), GL_FALSE, data);
#if defined(UNIFORM_DEBUG_VERBOSE) && (defined(DEBUG) || defined(_DEBUG))
  else
    std::cerr << "Warn: uniform " << name << " cannot be set in shader "
              << shader_name_ << "\n";
#endif
}

GLuint shader::get_program() const { return program_; }


} // namespace example
