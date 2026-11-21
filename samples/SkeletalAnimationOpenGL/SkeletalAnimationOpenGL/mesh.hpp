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
    static constexpr uint32_t NUM_MAX_BONES = 4;

    GLfloat pos[3];
    GLfloat texcoord[2];
    GLuint bone_ids[NUM_MAX_BONES];
    GLfloat bone_weights[NUM_MAX_BONES];
};

class Mesh
{
public:
    static Result<Box<Mesh>> create(const std::vector<Vertex> &vertices, const std::vector<GLuint> &indices, const std::vector<GLuint> &textures);

    ~Mesh();

    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;

    void draw();

private:
    Mesh() = default;

private:
    GLsizei m_index_count{};
    GLuint m_vao{}, m_vbo{}, m_ibo{};
    std::vector<GLuint> m_textures{};
};
