/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2018, Robotic Eyes GmbH

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the
following conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/

#ifndef ASSIMP_BUILD_NO_EXPORT
#ifndef ASSIMP_BUILD_NO_REX_EXPORTER

#include "RexExporter.h"
#include <assimp/Exceptional.h>
#include <assimp/StringComparison.h>
#include <assimp/material.h>
#include <assimp/scene.h>
#include <assimp/version.h>
#include <algorithm>
#include <assimp/Exporter.hpp>
#include <assimp/IOSystem.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

using namespace Assimp;
using namespace rex;

namespace Assimp {

// ------------------------------------------------------------------------------------------------
// Worker function for exporting a scene to Robotic Eyes REX format. Prototyped and registered in Exporter.cpp
void ExportSceneRex(const char *pFile, IOSystem * /* pIOSystem */, const aiScene *pScene, const ExportProperties * /*pProperties*/) {
    RexExporter exporter(pFile, pScene);
    exporter.Start();
}
} // namespace Assimp

// ------------------------------------------------------------------------------------------------
RexExporter::RexExporter(const char *fileName, const aiScene *pScene) :
        m_Scene(pScene) {

    m_File = std::make_shared<FileWrapper>(fileName, "wb");

    if (m_File == nullptr) {
        throw std::runtime_error("Cannot open file for writing.");
    }
}

// ------------------------------------------------------------------------------------------------
RexExporter::~RexExporter() {
}

// ------------------------------------------------------------------------------------------------
void RexExporter::Start() {
    printf("Starting REX exporter ...\n");

    WriteGeometryFile();
}

// ------------------------------------------------------------------------------------------------
void RexExporter::WriteGeometryFile() {
    printf("Write geometry file\n");

    std::vector<DataPtr> meshPtrs;
    std::vector<DataPtr> materialPtrs;
    std::vector<DataPtr> imagePtrs;

    rex_header *header = rex_header_create();
    int startBlockIndex = 0;
    int startBlockMaterials = 0;

    // get materials and textures
    GetMaterialsAndTextures();

    // write texture files
    WriteImages(header, startBlockIndex, imagePtrs);
    startBlockIndex += (int)imagePtrs.size();
    startBlockMaterials = startBlockIndex;

    // write materials
    WriteMaterials(header, startBlockIndex, materialPtrs);
    startBlockIndex += (int)materialPtrs.size();

    // get meshes
    WriteMeshes(header, startBlockIndex, startBlockMaterials, meshPtrs);

    long header_sz;
    uint8_t *header_ptr = rex_header_write(header, &header_sz);

    ::fseek(m_File->ptr(), 0, SEEK_SET);
    m_File->write(header_ptr, header_sz, 1, "writeHeader");

    // write images (texture files)
    for (DataPtr m : imagePtrs) {
        m_File->write(m.data, m.size, 1, "writeImage");
        FREE(m.data);
    }

    // write materials
    for (DataPtr m : materialPtrs) {
        m_File->write(m.data, m.size, 1, "writeMaterial");
        FREE(m.data);
    }

    // write meshes
    for (DataPtr m : meshPtrs) {
        m_File->write(m.data, m.size, 1, "writeMesh");
        FREE(m.data)
    }

    FREE(header_ptr);
}

// ------------------------------------------------------------------------------------------------
void RexExporter::WriteImages(rex_header *header, int startId, std::vector<DataPtr> &imagePtrs) {
    imagePtrs.resize(m_TextureMap.size());

    printf("Found %zu texture files\n", m_TextureMap.size());

    int i = 0;
    std::vector<std::string> names;
    m_TextureMap.getKeys(names);

    for (std::string &fileName : names) {
        rex_image img;

        // load texture file
        long size;
        std::string testFileWithPath = m_File->getFilePath() + fileName;
        img.data = read_file_binary(testFileWithPath.c_str(), &size);
        img.sz = (uint64_t)size;

        auto dotPos = fileName.rfind(".");
        auto fileExt = fileName.substr(dotPos + 1, fileName.size() - dotPos - 1);

        // convert file extension to lower case
        for (size_t j = 0; j < fileExt.size(); j++) {
            fileExt[j] = (char)std::tolower(fileExt[j]);
        }

        if (fileExt.compare("png") == 0) {
            img.compression = Png;
        } else if ((fileExt.compare("jpeg") == 0) || (fileExt.compare("jpg") == 0)) {
            img.compression = Jpeg;
        } else {
            img.compression = Raw24;
        }

        imagePtrs[i].data = rex_block_write_image(startId + i /*id*/, header, &img, &imagePtrs[i].size);
        i++;
    }
}

// ------------------------------------------------------------------------------------------------
void RexExporter::WriteMeshes(rex_header *header, int startId, int startMaterials, std::vector<DataPtr> &meshPtrs) {
    // collect mesh geometry
    aiMatrix4x4 mBase;
    AddNode(m_Scene->mRootNode, mBase);

    printf("Found %d meshes\n", (int)m_Meshes.size());

    meshPtrs.resize(m_Meshes.size());

    // now write all mesh instances
    int i = 0;
    for (MeshInstance &m : m_Meshes) {
        rex_mesh rexMesh;
        rex_mesh_init(&rexMesh);

        rexMesh.lod = 0;
        rexMesh.max_lod = 0;
        sprintf(rexMesh.name, "%s", m.name.c_str());
        rexMesh.nr_triangles = uint32_t(m.triangles.size());
        rexMesh.nr_vertices = uint32_t(m.verticesWithColors.size());

        // vertices with colors
        std::vector<VertexData> verticesWithColors;
        m.verticesWithColors.getKeys(verticesWithColors);
        std::vector<aiVector3D> vertices;
        std::vector<aiColor3D> colors;
        std::vector<aiVector3D> textureCoords;
        for (const VertexData &v : verticesWithColors) {
            vertices.push_back(v.vp);
            colors.push_back(v.vc);
            textureCoords.push_back(v.vt);
        }
        std::vector<float> vertexArray;
        std::vector<float> colorArray;
        std::vector<float> textureCoordArray;

        GetVertexArray(vertices, vertexArray);
        GetColorArray(colors, colorArray);
        GetTextureCoordArray(textureCoords, textureCoordArray);

        rexMesh.positions = &vertexArray[0];
        if (m.useColors)
            rexMesh.colors = &colorArray[0];
        rexMesh.tex_coords = &textureCoordArray[0];

        // triangles
        std::vector<uint32_t> triangles;
        GetTriangleArray(m.triangles, triangles);
        rexMesh.triangles = &triangles[0];

        // material
        rexMesh.material_id = startMaterials + m.materialId;
        meshPtrs[i].data = rex_block_write_mesh(startId + i /*id*/, header, &rexMesh, &meshPtrs[i].size);

        i++;
    }
}

// ------------------------------------------------------------------------------------------------
void RexExporter::GetVertexArray(const std::vector<aiVector3D> vector, std::vector<float> &vectorArray) {
    vectorArray.resize(vector.size() * 3);

    // !! Flip coordinate system
    for (size_t i = 0; i < vector.size(); i++) {
        size_t index = i * 3;
        vectorArray[index] = vector[i].x;
        vectorArray[index + 1] = vector[i].z;
        vectorArray[index + 2] = -vector[i].y;
    }
}

// ------------------------------------------------------------------------------------------------
void RexExporter::GetColorArray(const std::vector<aiColor3D> vector, std::vector<float> &colorArray) {
    colorArray.resize(uint32_t(vector.size()) * 3);
    for (size_t i = 0; i < vector.size(); i++) {
        size_t index = i * 3;
        colorArray[index] = vector[i].r;
        colorArray[index + 1] = vector[i].g;
        colorArray[index + 2] = vector[i].b;
    }
}

// ------------------------------------------------------------------------------------------------
void RexExporter::GetTextureCoordArray(const std::vector<aiVector3D> vector, std::vector<float> &textureCoordArray) {
    textureCoordArray.resize(uint32_t(vector.size()) * 2);

    for (size_t i = 0; i < vector.size(); i++) {
        size_t index = i * 2;
        textureCoordArray[index] = vector[i].x;
        textureCoordArray[index + 1] = vector[i].y;
    }
}

// ------------------------------------------------------------------------------------------------
void RexExporter::GetTriangleArray(const std::vector<Triangle> &triangles, std::vector<uint32_t> &triangleArray) {
    triangleArray.resize(uint32_t(triangles.size()) * 3);
    for (size_t i = 0; i < triangles.size(); i++) {
        Triangle t = triangles.at(i);
        triangleArray[3 * i] = t.indices[0];
        triangleArray[3 * i + 1] = t.indices[1];
        triangleArray[3 * i + 2] = t.indices[2];
    }
}

// ------------------------------------------------------------------------------------------------
void RexExporter::WriteMaterials(rex_header *header, int startId, std::vector<DataPtr> &materialPtrs) {
    materialPtrs.resize(uint32_t(m_Materials.size()));
    for (unsigned int i = 0; i < m_Materials.size(); ++i) {
        materialPtrs[i].data = rex_block_write_material(startId + i /*id*/, header, &m_Materials[i], &materialPtrs[i].size);
    }
}

// ------------------------------------------------------------------------------------------------
void RexExporter::GetMaterialsAndTextures() {

    bool embeddedTextures = (m_Scene->mNumTextures > 0);
    if (embeddedTextures) {
        printf("Embeddeed textures used.\n");
    } else {
        printf("No embeddeed textures used.\n");
    }

    printf("Found %d materials\n", m_Scene->mNumMaterials);
    m_Materials.resize(m_Scene->mNumMaterials);
    for (unsigned int i = 0; i < m_Scene->mNumMaterials; ++i) {
        const aiMaterial *const mat = m_Scene->mMaterials[i];

        // write material
        rex_material_standard rexMat;
        rexMat.alpha = 1;
        rexMat.ns = 0;

        aiColor4D c;
        aiString s;
        if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_DIFFUSE, c)) {
            rexMat.kd_red = c.r;
            rexMat.kd_green = c.g;
            rexMat.kd_blue = c.b;
            rexMat.kd_textureId = 0x7fffffffffffffffL;
            if (AI_SUCCESS == mat->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), s)) {
                // path to texture file or index if texture is embedded
                if (!embeddedTextures) {
                    rexMat.kd_textureId = m_TextureMap.getIndex(s.data);
                } else {
                    // TODO embedded texture
                }
            }
        }
        if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_AMBIENT, c)) {
            rexMat.ka_red = c.r;
            rexMat.ka_green = c.g;
            rexMat.ka_blue = c.b;
            rexMat.ka_textureId = 0x7fffffffffffffffL;
            if (AI_SUCCESS == mat->Get(AI_MATKEY_TEXTURE_AMBIENT(0), s)) {
                // path to texture file or index if texture is embedded
                if (!embeddedTextures) {
                    rexMat.ka_textureId = m_TextureMap.getIndex(s.data);
                } else {
                    // TODO embedded texture
                }
            }
        }
        if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_SPECULAR, c)) {
            rexMat.ks_red = c.r;
            rexMat.ks_green = c.g;
            rexMat.ks_blue = c.b;
            rexMat.ks_textureId = 0x7fffffffffffffffL;
            if (AI_SUCCESS == mat->Get(AI_MATKEY_TEXTURE_SPECULAR(0), s)) {
                if (!embeddedTextures) {
                    rexMat.ks_textureId = m_TextureMap.getIndex(s.data);
                } else {
                    // TODO embedded texture
                }
            }
        }

        ai_real o;
        if (AI_SUCCESS == mat->Get(AI_MATKEY_OPACITY, o)) {
            rexMat.alpha = o;
        }
        if (AI_SUCCESS == mat->Get(AI_MATKEY_SHININESS, o) && o) {
            rexMat.ns = o;
        }
        m_Materials[i] = rexMat;
    }
}

// ------------------------------------------------------------------------------------------------
void RexExporter::AddMesh(const aiString &name, const aiMesh *m, const aiMatrix4x4 &mat) {
    m_Meshes.push_back(MeshInstance());
    MeshInstance &mesh = m_Meshes.back();

    mesh.name = std::string(name.data, name.length);
    mesh.materialId = m->mMaterialIndex;

    // find number of faces which are triangles
    int meshSize = 0;
    for (unsigned int i = 0; i < m->mNumFaces; ++i) {
        if (m->mFaces[i].mNumIndices == 3) {
            meshSize++;
        }
    }

    mesh.triangles.resize(meshSize);
    mesh.useColors = (nullptr != m->mColors[0]);

    int triangleIndex = 0;
    for (unsigned int i = 0; i < m->mNumFaces; ++i) {
        const aiFace &f = m->mFaces[i];

        if (f.mNumIndices != 3) {
            // TODO no triangle - add line or point
            continue;
        }

        Triangle &triangle = mesh.triangles[triangleIndex];
        triangleIndex++;
        triangle.indices.resize(3);

        for (unsigned int a = 0; a < 3; ++a) {
            const unsigned int idx = f.mIndices[a];

            aiVector3D vert = mat * m->mVertices[idx];

            if (mesh.useColors) {
                aiColor4D col4 = m->mColors[0][idx];
                if (m->mTextureCoords[0]) {
                    triangle.indices[a] = mesh.verticesWithColors.getIndex({ vert, aiColor3D(col4.r, col4.g, col4.b), m->mTextureCoords[0][idx] });
                } else {
                    triangle.indices[a] = mesh.verticesWithColors.getIndex({ vert, aiColor3D(col4.r, col4.g, col4.b), aiVector3D(0, 0, 0) });
                }
            } else {
                if (m->mTextureCoords[0]) {
                    triangle.indices[a] = mesh.verticesWithColors.getIndex({ vert, aiColor3D(0, 0, 0), m->mTextureCoords[0][idx] });
                } else {
                    triangle.indices[a] = mesh.verticesWithColors.getIndex({ vert, aiColor3D(0, 0, 0), aiVector3D(0, 0, 0) });
                }
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
void RexExporter::AddNode(const aiNode *nd, const aiMatrix4x4 &mParent) {
    const aiMatrix4x4 &mAbs = mParent * nd->mTransformation;

    aiMesh *cm(nullptr);
    for (unsigned int i = 0; i < nd->mNumMeshes; ++i) {
        cm = m_Scene->mMeshes[nd->mMeshes[i]];
        if (nullptr != cm) {
            AddMesh(cm->mName, m_Scene->mMeshes[nd->mMeshes[i]], mAbs);
        } else {
            AddMesh(nd->mName, m_Scene->mMeshes[nd->mMeshes[i]], mAbs);
        }
    }

    for (unsigned int i = 0; i < nd->mNumChildren; ++i) {
        AddNode(nd->mChildren[i], mAbs);
    }
}

// ------------------------------------------------------------------------------------------------

#endif // ASSIMP_BUILD_NO_REX_EXPORTER
#endif // ASSIMP_BUILD_NO_EXPORT
