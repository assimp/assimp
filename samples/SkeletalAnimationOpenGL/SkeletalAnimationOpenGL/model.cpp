// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#include "model.hpp"

Model::~Model()
{
    glDeleteTextures(m_textures.size(), m_textures.data());
}

void Model::draw()
{
    for(auto& m: m_meshes)
        m->draw();
}
