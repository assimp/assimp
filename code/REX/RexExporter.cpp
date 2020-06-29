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
    std::vector<DataPtr> linePtrs;
    DataPtr pointPtr;

    rex_header *header = rex_header_create();
    uint64_t startBlockIndex = 0;
    uint64_t startBlockMaterials = 0;

    // get materials and textures
    GetMaterialsAndTextures();

    // write texture files
    WriteImages(header, startBlockIndex, imagePtrs);
    startBlockIndex += (uint64_t)imagePtrs.size();
    startBlockMaterials = startBlockIndex;

    // write materials
    WriteMaterials(header, startBlockIndex, materialPtrs);
    startBlockIndex += (uint64_t)materialPtrs.size();

    // get triangles, lines and points
    WriteObjects(header, startBlockIndex, startBlockMaterials, meshPtrs, linePtrs, pointPtr);
    startBlockIndex += (uint64_t)meshPtrs.size();

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

    // write lines
    for (DataPtr l : linePtrs) {
        m_File->write(l.data, l.size, 1, "writeLine");
        FREE(l.data)
    }

    // write points
    if (pointPtr.size > 0) {
        m_File->write(pointPtr.data, pointPtr.size, 1, "writePoints");
        FREE(pointPtr.data)
    }

    FREE(header_ptr);
}

// ------------------------------------------------------------------------------------------------
void RexExporter::WriteImages(rex_header *header, uint64_t startId, std::vector<DataPtr> &imagePtrs) {
    imagePtrs.resize(m_TextureMap.size());

    uint64_t i = 0;
    std::vector<std::string> names;
    m_TextureMap.getKeys(names);

    int nrOfNotFoundImages = 0;
    for (std::string &fileName : names) {
        rex_image img;

        // load texture file
        long size;
        std::string testFileWithPath = m_File->getFilePath() + fileName;
        img.data = read_file_binary(testFileWithPath.c_str(), &size);

        if (img.data == nullptr) {
            printf("Cannot load texture file %s \n", names[i].c_str());
            nrOfNotFoundImages++;
            continue;
        }
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
    imagePtrs.resize(m_TextureMap.size() - nrOfNotFoundImages);

    printf("Texture files: assigned: %zu, loaded: %lu\n", m_TextureMap.size(), m_TextureMap.size() - nrOfNotFoundImages);
}

// ------------------------------------------------------------------------------------------------
void RexExporter::WriteLines(uint64_t startId, rex_header *header, std::vector<DataPtr> &linePtrs)
{
    // printf("Found %d lines\n", (int)m_Lines.size());

    linePtrs.resize(m_Lines.size());

    // now write all line instances
    uint64_t i = 0;
    for (LineInstance &l : m_Lines) {
        rex_lineset rexLine;

        rexLine.red = l.color.r;
        rexLine.green = l.color.g;
        rexLine.blue = l.color.b;
        rexLine.alpha = l.color.a;
        rexLine.nr_vertices = 2;

        std::vector<float> vertexArray;
        GetVertexArray(l.vertices, vertexArray);
        rexLine.vertices = &vertexArray[0];

        uint64_t id = startId + i;
        linePtrs[i].data = rex_block_write_lineset(id, header, &rexLine, &linePtrs[i].size);

        i++;
    }
}

// ------------------------------------------------------------------------------------------------
void RexExporter::WritePoints(uint64_t startId, rex_header *header, DataPtr &pointPtrs)
{
    // printf("Found %d points\n", (int)m_Points.size());

    if (m_Points.size() == 0) {
        pointPtrs.size = 0;
        return;
    }

    // now write all points to one pointlist

    rex_pointlist rexPointList;
    std::vector<aiVector3D> points;
    points.resize(m_Points.size());

    std::vector<aiColor3D> colors;
    colors.resize(m_Points.size());

    for (size_t i = 0; i < m_Points.size(); i++) {
        points[i] = m_Points[i].vertex;
        colors[i] = m_Points[i].color;
    }

    std::vector<float> vertexArray;
    GetVertexArray(points, vertexArray);

    std::vector<float> colorArray;
    GetColorArray(colors, colorArray);

    rexPointList.nr_vertices = m_Points.size();
    if (m_Points.size() > 0 && m_Points[0].hasColor) {
        rexPointList.nr_colors = m_Points.size();
        rexPointList.colors = &colorArray[0];
    } else {
        rexPointList.nr_colors = 0;
    }

    rexPointList.positions = &vertexArray[0];

    pointPtrs.data = rex_block_write_pointlist(startId, header, &rexPointList, &pointPtrs.size);
}

// ------------------------------------------------------------------------------------------------
void RexExporter::WriteObjects(rex_header *header, uint64_t startId, uint64_t startMaterials, std::vector<DataPtr> &meshPtrs, std::vector<DataPtr> &linePtrs, DataPtr &pointPtrs)
{
    // collect mesh geometry
    aiMatrix4x4 mBase;
    AddNode(m_Scene->mRootNode, mBase);

    WriteMeshes(header, startId, startMaterials, meshPtrs);
    WriteLines(startId + (uint64_t)meshPtrs.size(), header, linePtrs);
    WritePoints(startId + (uint64_t)meshPtrs.size() + (uint64_t)linePtrs.size(), header, pointPtrs);
}

// ------------------------------------------------------------------------------------------------
void RexExporter::WriteMeshes(rex_header *header, uint64_t startId, uint64_t startMaterials, std::vector<DataPtr> &meshPtrs)
{
    size_t newSize = m_Meshes.size();
    meshPtrs.resize(m_Meshes.size());

    // now write all mesh instances
    uint64_t i = 0;
    for (MeshInstance &m : m_Meshes) {
        rex_mesh rexMesh;
        rex_mesh_init(&rexMesh);

        rexMesh.lod = 0;
        rexMesh.max_lod = 0;
        sprintf(rexMesh.name, "%s", m.name.c_str());
        rexMesh.nr_triangles = uint32_t(m.triangles.size());
        if (rexMesh.nr_triangles == 0) {
            // no real mesh
            newSize--;
            if (newSize >= 0) {
                meshPtrs.resize(newSize);
            }
            continue;
        }

        rexMesh.nr_vertices = uint32_t(m.verticesWithColorsAndTextureCoords.size());

        // vertices with colors
        std::vector<VertexData> verticesWithColors;
        m.verticesWithColorsAndTextureCoords.getKeys(verticesWithColors);
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
     printf("Found %d meshes\n", (int)newSize);
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
void RexExporter::GetTriangleArray(const std::vector<IndexList> &triangles, std::vector<uint32_t> &triangleArray) {
    triangleArray.resize(uint32_t(triangles.size()) * 3);
    for (size_t i = 0; i < triangles.size(); i++) {
        IndexList t = triangles.at(i);
        triangleArray[3 * i] = t.indices[0];
        triangleArray[3 * i + 1] = t.indices[1];
        triangleArray[3 * i + 2] = t.indices[2];
    }
}

// ------------------------------------------------------------------------------------------------
void RexExporter::WriteMaterials(rex_header *header, uint64_t startId, std::vector<DataPtr> &materialPtrs) {
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

    // printf("Found %d materials\n", m_Scene->mNumMaterials);
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

    // find number of faces which are triangles, lines or points
    int meshSize = 0;

    for (unsigned int i = 0; i < m->mNumFaces; ++i) {
        if (m->mFaces[i].mNumIndices == 3) {
            meshSize++;
        }
    }

    mesh.triangles.resize(meshSize);
    mesh.useColors = (nullptr != m->mColors[0]);
    if (m->mNumFaces == 0 && m->mNumVertices > 0)
    {
        // point cloud
        AddPoints(m, mat);
    }
    int triangleIndex = 0;
    for (unsigned int i = 0; i < m->mNumFaces; ++i) {
        const aiFace &f = m->mFaces[i];

        switch (f.mNumIndices) {
            case 1:
                AddPoint(m, &m->mFaces[i], mat);
                continue;
            case 2:
                AddLine(m, &m->mFaces[i], mat);
                continue;
            case 3:
                // triangles
                break;
            default:
               continue;
        }

        IndexList &triangle = mesh.triangles[triangleIndex];
        triangleIndex++;
        triangle.indices.resize(3);
        for (unsigned int a = 0; a < 3; ++a) {
            const unsigned int idx = f.mIndices[a];

            aiVector3D vert = mat * m->mVertices[idx];

            if (mesh.useColors) {
                aiColor4D col4 = m->mColors[0][idx];
                if (m->mTextureCoords[0]) {
                    triangle.indices[a] = mesh.verticesWithColorsAndTextureCoords.getIndex({ vert, aiColor3D(col4.r, col4.g, col4.b), m->mTextureCoords[0][idx] });
                } else {
                    triangle.indices[a] = mesh.verticesWithColorsAndTextureCoords.getIndex({ vert, aiColor3D(col4.r, col4.g, col4.b), aiVector3D(0, 0, 0) });
                }
            } else {
                if (m->mTextureCoords[0]) {
                    triangle.indices[a] = mesh.verticesWithColorsAndTextureCoords.getIndex({ vert, aiColor3D(0, 0, 0), m->mTextureCoords[0][idx] });
                } else {
                    triangle.indices[a] = mesh.verticesWithColorsAndTextureCoords.getIndex({ vert, aiColor3D(0, 0, 0), aiVector3D(0, 0, 0) });
                }
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
void RexExporter::AddPoints(const aiMesh *m, const aiMatrix4x4 &mat)
{
    for (size_t i = 0; i < m->mNumVertices; i++) {
        m_Points.push_back(PointInstance());
        PointInstance &point = m_Points.back();

        point.vertex = mat * m->mVertices[i];

        point.hasColor = false;

        if (m->mColors[0]) {
            point.hasColor = true;
            aiColor4D col4 = m->mColors[0][i];
            point.color = aiColor3D(col4.r, col4.g, col4.b);
        }
    }
}

// ------------------------------------------------------------------------------------------------
void RexExporter::AddPoint(const aiMesh *m, const aiFace *f, const aiMatrix4x4 &mat)
{
    m_Points.push_back(PointInstance());
    PointInstance &point = m_Points.back();

    point.vertex = mat * m->mVertices[f->mIndices[0]];
    point.hasColor = false;
    if (m->mColors[0]) {
        point.hasColor = true;
        aiColor4D col4 = m->mColors[0][0];
        point.color = aiColor3D(col4.r, col4.g, col4.b);
    }
}

// ------------------------------------------------------------------------------------------------
void RexExporter::AddLine(const aiMesh *m, const aiFace *f, const aiMatrix4x4 &mat)
{
    m_Lines.push_back(LineInstance());
    LineInstance &line = m_Lines.back();

    line.vertices.resize(2);
    line.vertices[0] = mat * m->mVertices[f->mIndices[0]];
    line.vertices[1] = mat * m->mVertices[f->mIndices[1]];
    if (m->mColors[0])
    {
        line.color = m->mColors[0][0];
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
