// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#include "model.hpp"
#include "texture_loader.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

Model::~Model()
{
    glDeleteTextures(m_textures.size(), m_textures.data());
}

void Model::draw()
{
    for(auto& m: m_meshes)
        m->draw();
}

Result<Box<Model>> ModelLoader::from_file(const std::string &filename)
{
    Box<Model> model(new Model());

    Assimp::Importer importer;
    const aiScene *pScene = importer.ReadFile(filename, aiProcess_Triangulate);

    if (!pScene)
        return importer.GetErrorString();

    ModelLoader loader{};
    loader.m_dir_path = filename.substr(0, filename.find_last_of("/\\"));
    CHECK_RESULT(loader.process_node(pScene, pScene->mRootNode, model->m_meshes, model->m_textures));

    return model;
}

ModelLoader::~ModelLoader()
{
}

Result<> ModelLoader::process_node(const aiScene *scene, aiNode *node, std::vector<Box<Mesh>>& meshes, std::vector<GLuint>& textures) 
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *ai_mesh = scene->mMeshes[node->mMeshes[i]];
        
        Box<Mesh> mesh;
        CHECK_ASSIGN_RESULT(
            process_mesh(scene, ai_mesh, textures),
            mesh
        );

        meshes.emplace_back(std::move(mesh));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        CHECK_RESULT(this->process_node(scene, node->mChildren[i], meshes, textures));
    }

    return Ok();
}

Result<Box<Mesh>> ModelLoader::process_mesh(const aiScene *scene, aiMesh *mesh, std::vector<GLuint>& textures)
{
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<GLuint> mesh_textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex{};

        vertex.x = mesh->mVertices[i].x;
        vertex.y = mesh->mVertices[i].y;
        vertex.z = mesh->mVertices[i].z;

        if (mesh->mTextureCoords[0])
        {
            vertex.u = static_cast<float>(mesh->mTextureCoords[0][i].x);
            vertex.v = static_cast<float>(mesh->mTextureCoords[0][i].y);
        }

        vertices.push_back(vertex);
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

        textures.insert(textures.end(), diffuse_maps.begin(), diffuse_maps.end());
        mesh_textures.insert(mesh_textures.end(), diffuse_maps.begin(), diffuse_maps.end());
    }

    return Mesh::create(vertices, indices, mesh_textures);
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
