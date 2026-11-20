// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#include "mesh.hpp"

void Texture::release() 
{
    if (texture)
    {
        glDeleteTextures(1, &texture);
        texture = 0;
    }
}

Mesh::Mesh(const std::vector<Vertex> &vertices, const std::vector<GLuint> &indices, const std::vector<Texture> &textures): m_textures(textures)
{
    if (vertices.empty() || indices.empty())
        throw std::runtime_error("mesh has no vertices or indices.");

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(Vertex) * vertices.size()), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(GLuint) * indices.size()), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(offsetof(Vertex, x)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(offsetof(Vertex, u)));

    glBindVertexArray(0);

    if (glGetError() != GL_NO_ERROR)
    {
        destroy();
        throw std::runtime_error("failed to create vertex/index buffers.");
    }

    m_index_count = (GLsizei)indices.size();
}

Mesh::~Mesh()
{
    destroy();
}

void Mesh::destroy()
{
    if (m_ibo)
    {
        glDeleteBuffers(1, &m_ibo);
        m_ibo = 0;
    }

    if (m_vbo)
    {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }

    if (m_vao)
    {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}

void Mesh::draw()
{
    glBindVertexArray(m_vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textures.empty() ? 0 : m_textures[0].texture);

    glDrawElements(GL_TRIANGLES, m_index_count, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
}
