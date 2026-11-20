// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#pragma once

#include "pch.hpp"

struct Vertex
{
    GLfloat x{}, y{}, z{};
    GLfloat u{}, v{};
};

struct Texture
{
    GLuint texture{};
    std::string type{};
    std::string path{};
};

class Mesh
{
public:
    static Result<Box<Mesh>> create(const std::vector<Vertex> &vertices, const std::vector<GLuint> &indices, const std::vector<Texture> &textures);

    ~Mesh();

    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;

    void draw();

private:
    Mesh() = default;

private:
    GLsizei m_index_count{};
    GLuint m_vao{}, m_vbo{}, m_ibo{};
    std::vector<Texture> m_textures{};
};
