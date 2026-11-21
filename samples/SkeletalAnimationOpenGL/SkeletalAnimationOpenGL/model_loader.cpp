// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#include "model_loader.hpp"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "contrib/stb/stb_image.h"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

namespace TextureLoader
{
  Result<GLuint> from_file(const std::string& fileName);
  Result<GLuint> from_memory(const uint8_t *data, size_t size);
  Result<GLuint> from_pixels(const void *pixels, GLsizei width, GLsizei height, GLenum format);
}

Result<Box<Model>> ModelLoader::from_file(const std::string &filename)
{
    Box<Model> model(new Model());

    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, Vertex::NUM_MAX_BONES);
    const aiScene *scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_LimitBoneWeights);

    if (!scene)
        return importer.GetErrorString();

    ModelLoader loader{};

    CHECK_RESULT(loader.setup_skeleton(scene->mRootNode, -1, model->m_joints));
    model->m_global_inverse_transform = inversed(scene->mRootNode->mTransformation);

    loader.m_dir_path = filename.substr(0, filename.find_last_of("/\\"));
    const auto result = loader.process_node(scene, scene->mRootNode, model->m_meshes, model->m_joints);

    for (const auto &loaded : loader.m_textures_loaded)
        model->m_textures.push_back(loaded.texture);

    if (result.is_error())
        return result.get_error();

    CHECK_RESULT(loader.load_animations(scene, model->m_animations));

    for (uint32_t i = 0; i < Model::MAX_JOINT_COUNT; i++)
        model->m_joint_transforms[i] = Mat4{}; // Default constructor gives identity matrix

    return model;
}

Result<> ModelLoader::setup_skeleton(aiNode *node, int32_t parent_index, std::vector<Model::Joint>& joints)
{
    const auto index = static_cast<int32_t>(joints.size());
    
    const std::string joint_name(node->mName.C_Str());
    
    if (m_joint_map.count(joint_name))
        return "found a duplicate joint name";

    m_joint_map[joint_name] = index;
    
    Model::Joint joint{};
    joint.parent_index = parent_index;
    joint.bind_transform = node->mTransformation;
    joints.push_back(joint);

    if (joints.size() > Model::MAX_JOINT_COUNT)
        return "model contains too many joints";

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        CHECK_RESULT(setup_skeleton(node->mChildren[i], index, joints));
    }

    return Ok();
}

Result<> ModelLoader::process_node(const aiScene *scene, aiNode *node, std::vector<Box<Mesh>>& meshes, std::vector<Model::Joint>& joints)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *ai_mesh = scene->mMeshes[node->mMeshes[i]];
        
        Box<Mesh> mesh;
        CHECK_ASSIGN_RESULT(
            process_mesh(scene, node, ai_mesh, joints),
            mesh
        );

        meshes.emplace_back(std::move(mesh));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        CHECK_RESULT(process_node(scene, node->mChildren[i], meshes, joints));
    }

    return Ok();
}

Result<Box<Mesh>> ModelLoader::process_mesh(const aiScene *scene, aiNode *node, aiMesh *mesh, std::vector<Model::Joint>& joints)
{
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<GLuint> mesh_textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex{};

        vertex.pos[0] = mesh->mVertices[i].x;
        vertex.pos[1] = mesh->mVertices[i].y;
        vertex.pos[2] = mesh->mVertices[i].z;

        if (mesh->mTextureCoords[0])
        {
            vertex.texcoord[0] = static_cast<float>(mesh->mTextureCoords[0][i].x);
            vertex.texcoord[1] = static_cast<float>(mesh->mTextureCoords[0][i].y);
        }

        std::fill(vertex.bone_weights, vertex.bone_weights + Vertex::NUM_MAX_BONES, 0.0f);

        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumBones; i++)
    {
        const auto& bone = mesh->mBones[i];
        
        const std::string bone_name(bone->mName.C_Str());
        const auto bone_id = m_joint_map.find(bone_name);
        if (bone_id == m_joint_map.end())
            return "failed to find a joint named \"" + bone_name + "\"";

        joints[bone_id->second].offset_matrix = bone->mOffsetMatrix;

        for (unsigned int j = 0; j < bone->mNumWeights; j++)
        {
            const auto& weight = bone->mWeights[j];
            auto& vertex = vertices[weight.mVertexId];
            for (uint32_t k = 0; k < Vertex::NUM_MAX_BONES; k++)
            {
                if (vertex.bone_weights[k] == 0.0f)
                {
                    vertex.bone_ids[k] = bone_id->second;
                    vertex.bone_weights[k] = weight.mWeight;
                    break;
                }
            }
        }
    }

    if (mesh->mNumBones == 0)
    {
        const auto node_joint = static_cast<GLuint>(m_joint_map.at(node->mName.C_Str()));
        for (auto& vertex : vertices)
        {
            vertex.bone_ids[0] = node_joint;
            vertex.bone_weights[0] = 1.0f;
        }
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        const aiFace &face = mesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    if (mesh->mMaterialIndex < scene->mNumMaterials)
    {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

        std::vector<GLuint> diffuse_maps;
        CHECK_ASSIGN_RESULT(
            this->load_material_textures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene),
            diffuse_maps
        );

        mesh_textures.insert(mesh_textures.end(), diffuse_maps.begin(), diffuse_maps.end());
    }

    return Mesh::create(vertices, indices, mesh_textures);
}

Result<> ModelLoader::load_animations(const aiScene* scene, std::vector<Model::Animation>& animations)
{
    for (unsigned int i = 0; i < scene->mNumAnimations; i++)
    {
        Model::Animation anim{};

        const auto& ai_anim = scene->mAnimations[i];

        anim.name = ai_anim->mName.C_Str();
        anim.duration = ai_anim->mDuration;
        anim.ticks_per_second = ai_anim->mTicksPerSecond ? ai_anim->mTicksPerSecond : Model::DEFAULT_TICKS_PER_SECOND;

        for (unsigned int j = 0; j < ai_anim->mNumChannels; j++)
        {
            Model::AnimationChannel channel{};

            const auto& ai_channel = ai_anim->mChannels[j];

            {
                const std::string name(ai_channel->mNodeName.C_Str());
                const auto it = m_joint_map.find(name);
                if (it == m_joint_map.end())
                    return "animation \"" + anim.name + "\" references unknown joint \"" + name + "\"";
                channel.joint_index = it->second;
            }

            // NOTE: We ignore the interpolation types and always use linear interpolation in this sample
            channel.position_keys.resize(ai_channel->mNumPositionKeys);
            channel.rotation_keys.resize(ai_channel->mNumRotationKeys);
            channel.scaling_keys.resize(ai_channel->mNumScalingKeys);
            for (unsigned int k = 0; k < ai_channel->mNumPositionKeys; k++)
                channel.position_keys[k] = {ai_channel->mPositionKeys[k].mTime, ai_channel->mPositionKeys[k].mValue};
            for (unsigned int k = 0; k < ai_channel->mNumRotationKeys; k++)
                channel.rotation_keys[k] = {ai_channel->mRotationKeys[k].mTime, static_cast<Quat>(ai_channel->mRotationKeys[k].mValue)};
            for (unsigned int k = 0; k < ai_channel->mNumScalingKeys; k++)
                channel.scaling_keys[k] = {ai_channel->mScalingKeys[k].mTime, ai_channel->mScalingKeys[k].mValue};

            anim.channels.push_back(std::move(channel));
        }

        animations.push_back(std::move(anim));
    }

    return Ok();
}

Result<std::vector<GLuint>> ModelLoader::load_material_textures(aiMaterial *mat, aiTextureType type, const std::string &type_name, const aiScene *scene)
{
    std::vector<GLuint> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        bool skip = false;
        for (size_t j = 0; j < m_textures_loaded.size(); j++)
        {
            if (std::strcmp(m_textures_loaded[j].path.c_str(), str.C_Str()) == 0)
            {
                textures.push_back(m_textures_loaded[j].texture);
                skip = true;
                break;
            }
        }

        if (skip)
            continue;

        TextureData texture;

        const aiTexture *embedded_texture = scene->GetEmbeddedTexture(str.C_Str());
        if (embedded_texture)
        {
            CHECK_ASSIGN_RESULT(
                load_embedded_texture(embedded_texture),
                texture.texture
            );
        }
        else
        {
            CHECK_ASSIGN_RESULT(
                TextureLoader::from_file(m_dir_path + '/' + std::string(str.C_Str())),
                texture.texture
            );
        }

        texture.type = type_name;
        texture.path = str.C_Str();
        textures.push_back(texture.texture);
        m_textures_loaded.push_back(texture);
    }

    return textures;
}

Result<GLuint> ModelLoader::load_embedded_texture(const aiTexture *embedded_texture)
{
    if (embedded_texture->mHeight)
    {
        GLuint texture = 0;

        CHECK_ASSIGN_RESULT(
            TextureLoader::from_pixels(embedded_texture->pcData,
                static_cast<GLsizei>(embedded_texture->mWidth),
                static_cast<GLsizei>(embedded_texture->mHeight),
                GL_BGRA),
            texture
        );

        return texture;
    }
    else
    {
        GLuint texture = 0;

        const size_t size = embedded_texture->mWidth;

        CHECK_ASSIGN_RESULT(
            TextureLoader::from_memory(reinterpret_cast<const uint8_t *>(embedded_texture->pcData), size),
            texture
        );

        return texture;
    }
}

namespace TextureLoader
{
    Result<GLuint> from_pixels(const void *pixels, GLsizei width, GLsizei height, GLenum format)
    {
        GLuint texture = 0;

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);

        if (glGetError() != GL_NO_ERROR) {
            glDeleteTextures(1, &texture);
            return "failed to create the texture from pixels";
        }

        return texture;
    }

    Result<GLuint> from_memory(const uint8_t *data, size_t size)
    {
        if (data == nullptr || size == 0 || size > static_cast<size_t>(INT_MAX))
            return "invalid arguments";

        stbi_set_flip_vertically_on_load(1);

        int w = 0, h = 0, n = 0;
        const auto pixels = stbi_load_from_memory(data, static_cast<int>(size), &w, &h, &n, STBI_rgb_alpha);
        if (pixels == nullptr)
            return "failed to decode the embedded image: " + std::string(stbi_failure_reason());

        auto result = from_pixels(pixels, w, h, GL_RGBA);
        stbi_image_free(pixels);

        return result;
    }

    Result<GLuint> from_file(const std::string& file_name)
    {
        stbi_set_flip_vertically_on_load(1);
        
        int w = 0, h = 0, n = 0;
        const auto pixels = stbi_load(file_name.c_str(), &w, &h, &n, STBI_rgb_alpha);
        if (pixels == nullptr)
            return "failed to load the image \"" + file_name + "\": " + std::string(stbi_failure_reason());

        auto result = from_pixels(pixels, w, h, GL_RGBA);
        stbi_image_free(pixels);

        return result;
    }
}
