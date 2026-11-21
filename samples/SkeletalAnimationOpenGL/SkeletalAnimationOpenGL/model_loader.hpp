// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#pragma once

#include "model.hpp"

#include <assimp/scene.h>

class ModelLoader
{
    struct TextureData
    {
        GLuint texture{};
        std::string type{};
        std::string path{};
    };

public:
    static Result<Box<Model>> from_file(const std::string &filename);

    ~ModelLoader();

    ModelLoader(const ModelLoader &) = delete;
    ModelLoader &operator=(const ModelLoader &) = delete;

private:
    ModelLoader() = default;

private:
    std::string m_dir_path;
    std::vector<TextureData> m_textures_loaded;
    std::map<std::string, int32_t> m_joint_map;

    Result<> setup_skeleton(aiNode *node, int32_t parent_index, std::vector<Model::Joint>& joints);
    Result<> process_node(const aiScene *scene, aiNode *node, std::vector<Box<Mesh>>& meshes, std::vector<Model::Joint>& joints);
    Result<Box<Mesh>> process_mesh(const aiScene *scene, aiMesh *mesh, std::vector<Model::Joint>& joints);
    Result<> load_animations(const aiScene* scene, std::vector<Model::Animation>& animations);

    Result<GLuint> load_embedded_texture(const aiTexture *embedded_texture);
    Result<std::vector<GLuint>> load_material_textures(aiMaterial *mat, aiTextureType type, const std::string &type_name, const aiScene *scene);
};
