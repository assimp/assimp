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
    struct Joint
    {
        int32_t parent_index{-1};
        
        Mat4 offset_matrix{};
        Mat4 bind_transform{};
        Mat4 current_transform{};
    };

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
    std::vector<Joint> m_joints;
    Mat4 m_global_inverse_transform{};

    friend class ModelLoader;
};
