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

    ~ModelLoader() = default;

    ModelLoader(const ModelLoader &) = delete;
    ModelLoader &operator=(const ModelLoader &) = delete;

private:
    ModelLoader() = default;

private:
    std::string m_dir_path;
    std::vector<TextureData> m_textures_loaded;
    std::map<const aiNode*, int32_t> m_node_joints;
    std::map<std::string, int32_t> m_joint_map;
    std::set<std::string> m_duplicate_joint_names;
    std::map<int32_t, GLuint> m_rigid_bones;

    Result<> setup_skeleton(aiNode *node, int32_t parent_index, std::vector<Model::Joint>& joints);
    Result<int32_t> find_joint(const std::string &name) const;
    Result<GLuint> add_bone(std::vector<Model::Bone> &bones, int32_t joint_index, const Mat4 &offset);
    Result<> process_node(const aiScene *scene, aiNode *node, std::vector<Box<Mesh>>& meshes, std::vector<Model::Bone>& bones);
    Result<Box<Mesh>> process_mesh(const aiScene *scene, aiNode *node, aiMesh *mesh, std::vector<Model::Bone>& bones);
    Result<> load_animations(const aiScene* scene, std::vector<Model::Animation>& animations);

    Result<GLuint> load_embedded_texture(const aiTexture *embedded_texture);
    Result<std::vector<GLuint>> load_material_textures(aiMaterial *mat, aiTextureType type, const std::string &type_name, const aiScene *scene);
};
