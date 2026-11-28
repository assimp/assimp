// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#include "gl_loader.hpp"
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

template <class AiKey, class T, class ConvertFn>
static void copy_keys(const AiKey *keys, unsigned int count, std::vector<Model::AnimationKey<T>> &out, ConvertFn convert_fn)
{
    const bool cubic = count > 0 && count % 3 == 0 && keys[0].mInterpolation == aiAnimInterpolation_Cubic_Spline;
    const unsigned int first = cubic ? 1 : 0;
    const unsigned int stride = cubic ? 3 : 1;

    out.clear();
    for (unsigned int k = first; k < count; k += stride)
        out.push_back({keys[k].mTime, convert_fn(keys[k].mValue)});
}

namespace TextureLoader
{
  Result<GLuint> from_file(const std::string& fileName);
  Result<GLuint> from_memory(const uint8_t *data, size_t size);
  Result<GLuint> from_pixels(const uint8_t *pixels, GLsizei width, GLsizei height, GLenum format);
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

    if (scene->mRootNode->mTransformation.Determinant() == 0.0f)
        return "scene root transform is not invertible";
    CHECK_RESULT(loader.setup_skeleton(scene->mRootNode, -1, model->m_joints));
    model->m_global_inverse_transform = inversed(scene->mRootNode->mTransformation);

    loader.m_dir_path = filename.substr(0, filename.find_last_of("/\\"));
    const auto result = loader.process_node(scene, scene->mRootNode, model->m_meshes, model->m_bones);

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
    
    m_node_joints[node] = index;
    if (!m_joint_map.emplace(joint_name, index).second)
        m_duplicate_joint_names.insert(joint_name);
    
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

Result<int32_t> ModelLoader::find_joint(const std::string &name) const
{
    if (m_duplicate_joint_names.count(name))
        return "joint name \"" + name + "\" is used by more than one node";
    const auto it = m_joint_map.find(name);
    if (it == m_joint_map.end())
        return "failed to find a joint named \"" + name + "\"";
    return it->second;
}

Result<GLuint> ModelLoader::add_bone(std::vector<Model::Bone> &bones, int32_t joint_index, const Mat4 &offset)
{
    for (size_t i = 0; i < bones.size(); i++)
    {
        if (bones[i].joint_index == joint_index && bones[i].offset_matrix == offset)
            return static_cast<GLuint>(i);
    }

    if (bones.size() >= Model::MAX_JOINT_COUNT)
        return "model uses too many bones";
    bones.push_back({joint_index, offset});
    
    return static_cast<GLuint>(bones.size() - 1);
}

Result<> ModelLoader::process_node(const aiScene *scene, aiNode *node, std::vector<Box<Mesh>>& meshes, std::vector<Model::Bone>& bones)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *ai_mesh = scene->mMeshes[node->mMeshes[i]];
        
        Box<Mesh> mesh;
        CHECK_ASSIGN_RESULT(
            process_mesh(scene, node, ai_mesh, bones),
            mesh
        );

        meshes.emplace_back(std::move(mesh));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        CHECK_RESULT(process_node(scene, node->mChildren[i], meshes, bones));
    }

    return Ok();
}

Result<Box<Mesh>> ModelLoader::process_mesh(const aiScene *scene, aiNode *node, aiMesh *mesh, std::vector<Model::Bone>& bones)
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
        
        int32_t joint_index = -1;
        CHECK_ASSIGN_RESULT(find_joint(bone->mName.C_Str()), joint_index);

        GLuint bone_id = 0;
        CHECK_ASSIGN_RESULT(add_bone(bones, joint_index, bone->mOffsetMatrix), bone_id);

        for (unsigned int j = 0; j < bone->mNumWeights; j++)
        {
            const auto& weight = bone->mWeights[j];
            auto& vertex = vertices[weight.mVertexId];
            for (uint32_t k = 0; k < Vertex::NUM_MAX_BONES; k++)
            {
                if (vertex.bone_weights[k] != 0.0f)
                    continue;

                vertex.bone_ids[k] = bone_id;
                vertex.bone_weights[k] = weight.mWeight;
                break;
            }
        }
    }

    std::optional<GLuint> rigid_bone_id;
    for (auto& vertex : vertices)
    {
        if (vertex.bone_weights[0] != 0.0f)
            continue;

        if (!rigid_bone_id)
        {
            GLuint bone_id = 0;
            CHECK_ASSIGN_RESULT(add_bone(bones, m_node_joints.at(node), Mat4{}), bone_id);
            rigid_bone_id = bone_id;
        }

        vertex.bone_ids[0] = *rigid_bone_id;
        vertex.bone_weights[0] = 1.0f;
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

            CHECK_ASSIGN_RESULT(find_joint(ai_channel->mNodeName.C_Str()), channel.joint_index);

            // NOTE: We always interpolate linearly
            copy_keys(ai_channel->mPositionKeys, ai_channel->mNumPositionKeys, channel.position_keys,
                [](const aiVector3D &v) { return Vec3(v); });
            copy_keys(ai_channel->mRotationKeys, ai_channel->mNumRotationKeys, channel.rotation_keys,
                [](const aiQuaternion &q) { return Quat(static_cast<float>(q.w), static_cast<float>(q.x), static_cast<float>(q.y), static_cast<float>(q.z)); });
            copy_keys(ai_channel->mScalingKeys, ai_channel->mNumScalingKeys, channel.scaling_keys,
                [](const aiVector3D &v) { return Vec3(v); });

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
            TextureLoader::from_pixels(reinterpret_cast<const uint8_t*>(embedded_texture->pcData),
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
    Result<GLuint> from_pixels(const uint8_t *pixels, GLsizei width, GLsizei height, GLenum format)
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

        auto result = from_pixels(reinterpret_cast<const uint8_t*>(pixels), w, h, GL_RGBA);
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
