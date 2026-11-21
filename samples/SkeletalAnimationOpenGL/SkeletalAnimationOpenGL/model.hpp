// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#pragma once

#include "math.hpp"
#include "mesh.hpp"

class Model
{
public:
    ~Model();

    Model(const Model &) = delete;
    Model &operator=(const Model &) = delete;

    void draw();

private:
    Model() = default;

private:
    std::vector<Box<Mesh>> m_meshes;
    std::vector<GLuint> m_textures;

    friend class ModelLoader;
};
