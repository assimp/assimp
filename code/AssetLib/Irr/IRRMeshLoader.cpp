/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

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
---------------------------------------------------------------------------
*/

/** @file Implementation of the IrrMesh importer class */

#ifndef ASSIMP_BUILD_NO_IRRMESH_IMPORTER

#include "IRRMeshLoader.h"
#include <assimp/ParsingUtils.h>
#include <assimp/fast_atof.h>
#include <assimp/importerdesc.h>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/IOSystem.hpp>
#include <memory>

using namespace Assimp;

static constexpr aiImporterDesc desc = {
    "Irrlicht Mesh Reader",
    "",
    "",
    "http://irrlicht.sourceforge.net/",
    aiImporterFlags_SupportTextFlavour,
    0,
    0,
    0,
    0,
    "xml irrmesh"
};

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file.
bool IRRMeshImporter::CanRead(const std::string &pFile, IOSystem *pIOHandler, bool /*checkSig*/) const {
    /* NOTE: A simple check for the file extension is not enough
     * here. Irrmesh and irr are easy, but xml is too generic
     * and could be collada, too. So we need to open the file and
     * search for typical tokens.
     */
    static const char *tokens[] = { "irrmesh" };
    return SearchFileHeaderForToken(pIOHandler, pFile, tokens, AI_COUNT_OF(tokens));
}

// ------------------------------------------------------------------------------------------------
// Get a list of all file extensions which are handled by this class
const aiImporterDesc *IRRMeshImporter::GetInfo() const {
    return &desc;
}

static void releaseMaterial(aiMaterial **mat) {
    if (*mat != nullptr) {
        delete *mat;
        *mat = nullptr;
    }
}

static void releaseMesh(aiMesh **mesh) {
    if (*mesh != nullptr) {
        delete *mesh;
        *mesh = nullptr;
    }
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure.
void IRRMeshImporter::InternReadFile(const std::string &pFile,
        aiScene *pScene, IOSystem *pIOHandler) {
    std::unique_ptr<IOStream> file(pIOHandler->Open(pFile));

    // Check whether we can read from the file
    if (file == nullptr) {
        throw DeadlyImportError("Failed to open IRRMESH file ", pFile);
    }

    // Construct the irrXML parser
    XmlParser parser;
    if (!parser.parse(file.get())) {
        throw DeadlyImportError("XML parse error while loading IRRMESH file ", pFile);
    }
    XmlNode root = parser.getRootNode();

    // final data
    std::vector<aiMaterial *> materials;
    std::vector<aiMesh *> meshes;
    materials.reserve(5);
    meshes.reserve(5);

    // temporary data - current mesh buffer
    // TODO move all these to inside loop
    aiMaterial *curMat = nullptr;
    aiMesh *curMesh = nullptr;
    unsigned int curMatFlags = 0;

    std::vector<aiVector3D> curVertices, curNormals, curTangents, curBitangents;
    std::vector<aiColor4D> curColors;
    std::vector<aiVector3D> curUVs, curUV2s;

    // some temporary variables
    // textMeaning is a 15 year old variable, that could've been an enum
    // int textMeaning = 0; // 0=none? 1=vertices 2=indices
    // int vertexFormat = 0; // 0 = normal; 1 = 2 tcoords, 2 = tangents
    bool useColors = false;

    // irrmesh files have a top level <mesh> owning multiple <buffer> nodes.
    // Each <buffer> contains <material>, <vertices>, and <indices>
    // <material> tags here directly owns the material data specs
    // <vertices> are a vertex per line, contains position, UV1 coords, maybe UV2, normal, tangent, bitangent
    // <boundingbox> is ignored, I think assimp recalculates those?

    // Parse the XML file
    pugi::xml_node const &meshNode = root.child("mesh");
    for (pugi::xml_node bufferNode : meshNode.children()) {
        if (ASSIMP_stricmp(bufferNode.name(), "buffer")) {
            // Might be a useless warning
            ASSIMP_LOG_WARN("IRRMESH: Ignoring non buffer node <", bufferNode.name(), "> in mesh declaration");
            continue;
        }

        curMat = nullptr;
        curMesh = nullptr;

        curVertices.clear();
        curColors.clear();
        curNormals.clear();
        curUV2s.clear();
        curUVs.clear();
        curTangents.clear();
        curBitangents.clear();

        // TODO ensure all three nodes are present and populated
        // before allocating everything

        // Get first material node
        pugi::xml_node materialNode = bufferNode.child("material");
        if (materialNode) {
            curMat = ParseMaterial(materialNode, curMatFlags);
            // Warn if there's more materials
            if (materialNode.next_sibling("material")) {
                ASSIMP_LOG_WARN("IRRMESH: Only one material description per buffer, please");
            }
        } else {
            ASSIMP_LOG_ERROR("IRRMESH: Buffer must contain one material");
            continue;
        }

        // Get first vertices node
        pugi::xml_node verticesNode = bufferNode.child("vertices");
        if (verticesNode) {
            pugi::xml_attribute vertexCountAttrib = verticesNode.attribute("vertexCount");
            int vertexCount = vertexCountAttrib.as_int();
            if (vertexCount == 0) {
                // This is possible ... remove the mesh from the list and skip further reading
                ASSIMP_LOG_WARN("IRRMESH: Found mesh with zero vertices");
                releaseMaterial(&curMat);
                continue; // Bail out early
            };

            curVertices.reserve(vertexCount);
            curNormals.reserve(vertexCount);
            curColors.reserve(vertexCount);
            curUVs.reserve(vertexCount);

            VertexFormat vertexFormat;
            // Determine the file format
            pugi::xml_attribute typeAttrib = verticesNode.attribute("type");
            if (!ASSIMP_stricmp("2tcoords", typeAttrib.value())) {
                curUV2s.reserve(vertexCount);
                vertexFormat = VertexFormat::t2coord;
                if (curMatFlags & AI_IRRMESH_EXTRA_2ND_TEXTURE) {
                    // *********************************************************
                    // We have a second texture! So use this UV channel
                    // for it. The 2nd texture can be either a normal
                    // texture (solid_2layer or lightmap_xxx) or a normal
                    // map (normal_..., parallax_...)
                    // *********************************************************
                    int idx = 1;
                    aiMaterial *mat = (aiMaterial *)curMat;

                    if (curMatFlags & AI_IRRMESH_MAT_lightmap) {
                        mat->AddProperty(&idx, 1, AI_MATKEY_UVWSRC_LIGHTMAP(0));
                    } else if (curMatFlags & AI_IRRMESH_MAT_normalmap_solid) {
                        mat->AddProperty(&idx, 1, AI_MATKEY_UVWSRC_NORMALS(0));
                    } else if (curMatFlags & AI_IRRMESH_MAT_solid_2layer) {
                        mat->AddProperty(&idx, 1, AI_MATKEY_UVWSRC_DIFFUSE(1));
                    }
                }
            } else if (!ASSIMP_stricmp("tangents", typeAttrib.value())) {
                curTangents.reserve(vertexCount);
                curBitangents.reserve(vertexCount);
                vertexFormat = VertexFormat::tangent;
            } else if (!ASSIMP_stricmp("standard", typeAttrib.value())) {
                vertexFormat = VertexFormat::standard;
            } else {
                // Unsupported format, discard whole buffer/mesh
                // Assuming we have a correct material, then release it
                // We don't have a correct mesh for sure here
                releaseMaterial(&curMat);
                ASSIMP_LOG_ERROR("IRRMESH: Unknown vertex format");
                continue; // Skip rest of buffer
            };

            // We know what format buffer is, collect numbers
            std::string v = verticesNode.text().get();
            const char *end = v.c_str() + v.size();
            ParseBufferVertices(v.c_str(), end, vertexFormat,
                    curVertices, curNormals,
                    curTangents, curBitangents,
                    curUVs, curUV2s, curColors, useColors);
        }

        // Get indices
        // At this point we have some vertices and a valid material
        // Collect indices and create aiMesh at the same time
        pugi::xml_node indicesNode = bufferNode.child("indices");
        if (indicesNode) {
            // start a new mesh
            curMesh = new aiMesh();

            // allocate storage for all faces
            pugi::xml_attribute attr = indicesNode.attribute("indexCount");
            curMesh->mNumVertices = attr.as_int();
            if (!curMesh->mNumVertices) {
                // This is possible ... remove the mesh from the list and skip further reading
                ASSIMP_LOG_WARN("IRRMESH: Found mesh with zero indices");

                // mesh - away
                releaseMesh(&curMesh);

                // material - away
                releaseMaterial(&curMat);
                continue; // Go to next buffer
            }

            if (curMesh->mNumVertices % 3) {
                ASSIMP_LOG_WARN("IRRMESH: Number if indices isn't divisible by 3");
            }

            curMesh->mNumFaces = curMesh->mNumVertices / 3;
            curMesh->mFaces = new aiFace[curMesh->mNumFaces];

            // setup some members
            curMesh->mMaterialIndex = (unsigned int)materials.size();
            curMesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

            // allocate storage for all vertices
            curMesh->mVertices = new aiVector3D[curMesh->mNumVertices];

            if (curNormals.size() == curVertices.size()) {
                curMesh->mNormals = new aiVector3D[curMesh->mNumVertices];
            }
            if (curTangents.size() == curVertices.size()) {
                curMesh->mTangents = new aiVector3D[curMesh->mNumVertices];
            }
            if (curBitangents.size() == curVertices.size()) {
                curMesh->mBitangents = new aiVector3D[curMesh->mNumVertices];
            }
            if (curColors.size() == curVertices.size() && useColors) {
                curMesh->mColors[0] = new aiColor4D[curMesh->mNumVertices];
            }
            if (curUVs.size() == curVertices.size()) {
                curMesh->mTextureCoords[0] = new aiVector3D[curMesh->mNumVertices];
            }
            if (curUV2s.size() == curVertices.size()) {
                curMesh->mTextureCoords[1] = new aiVector3D[curMesh->mNumVertices];
            }

            // read indices
            aiFace *curFace = curMesh->mFaces;
            aiFace *const faceEnd = curMesh->mFaces + curMesh->mNumFaces;

            aiVector3D *pcV = curMesh->mVertices;
            aiVector3D *pcN = curMesh->mNormals;
            aiVector3D *pcT = curMesh->mTangents;
            aiVector3D *pcB = curMesh->mBitangents;
            aiColor4D *pcC0 = curMesh->mColors[0];
            aiVector3D *pcT0 = curMesh->mTextureCoords[0];
            aiVector3D *pcT1 = curMesh->mTextureCoords[1];

            unsigned int curIdx = 0;
            unsigned int total = 0;

            // NOTE this might explode for UTF-16 and wchars
            const char *sz = indicesNode.text().get();
            const char *end = sz + std::strlen(sz);

            // For each index loop over aiMesh faces
            while (SkipSpacesAndLineEnd(&sz, end)) {
                if (curFace >= faceEnd) {
                    ASSIMP_LOG_ERROR("IRRMESH: Too many indices");
                    break;
                }
                // if new face
                if (!curIdx) {
                    curFace->mNumIndices = 3;
                    curFace->mIndices = new unsigned int[3];
                }

                // Read index base 10
                // function advances the pointer
                unsigned int idx = strtoul10(sz, &sz);
                if (idx >= curVertices.size()) {
                    ASSIMP_LOG_ERROR("IRRMESH: Index out of range");
                    idx = 0;
                }

                // make up our own indices?
                curFace->mIndices[curIdx] = total++;

                // Copy over data to aiMesh
                *pcV++ = curVertices[idx];
                if (pcN)
                    *pcN++ = curNormals[idx];
                if (pcT)
                    *pcT++ = curTangents[idx];
                if (pcB)
                    *pcB++ = curBitangents[idx];
                if (pcC0)
                    *pcC0++ = curColors[idx];
                if (pcT0)
                    *pcT0++ = curUVs[idx];
                if (pcT1)
                    *pcT1++ = curUV2s[idx];

                // start new face
                if (++curIdx == 3) {
                    ++curFace;
                    curIdx = 0;
                }
            }
            // We should be at the end of mFaces
            if (curFace != faceEnd) {
                ASSIMP_LOG_ERROR("IRRMESH: Not enough indices");
            }
        }

        // Finish processing the mesh - do some small material workarounds
        if (curMatFlags & AI_IRRMESH_MAT_trans_vertex_alpha && !useColors) {
            // Take the opacity value of the current material
            // from the common vertex color alpha
            aiMaterial *mat = (aiMaterial *)curMat;
            mat->AddProperty(&curColors[0].a, 1, AI_MATKEY_OPACITY);
        }

        // end of previous buffer. A material and a mesh should be there
        if (!curMat || !curMesh) {
            ASSIMP_LOG_ERROR("IRRMESH: A buffer must contain a mesh and a material");
            releaseMaterial(&curMat);
            releaseMesh(&curMesh);
        } else {
            materials.push_back(curMat);
            meshes.push_back(curMesh);
        }
    }

    // If one is empty then so is the other
    if (materials.empty() || meshes.empty()) {
        throw DeadlyImportError("IRRMESH: Unable to read a mesh from this file");
    }

    // now generate the output scene
    pScene->mNumMeshes = (unsigned int)meshes.size();
    pScene->mMeshes = new aiMesh *[pScene->mNumMeshes];
    for (unsigned int i = 0; i < pScene->mNumMeshes; ++i) {
        pScene->mMeshes[i] = meshes[i];

        // clean this value ...
        pScene->mMeshes[i]->mNumUVComponents[3] = 0;
    }

    pScene->mNumMaterials = (unsigned int)materials.size();
    pScene->mMaterials = new aiMaterial *[pScene->mNumMaterials];
    ::memcpy(pScene->mMaterials, &materials[0], sizeof(void *) * pScene->mNumMaterials);

    pScene->mRootNode = new aiNode();
    pScene->mRootNode->mName.Set("<IRRMesh>");
    pScene->mRootNode->mNumMeshes = pScene->mNumMeshes;
    pScene->mRootNode->mMeshes = new unsigned int[pScene->mNumMeshes];

    for (unsigned int i = 0; i < pScene->mNumMeshes; ++i) {
        pScene->mRootNode->mMeshes[i] = i;
    };
}

void IRRMeshImporter::ParseBufferVertices(const char *sz, const char *end, VertexFormat vertexFormat,
        std::vector<aiVector3D> &vertices, std::vector<aiVector3D> &normals,
        std::vector<aiVector3D> &tangents, std::vector<aiVector3D> &bitangents,
        std::vector<aiVector3D> &UVs, std::vector<aiVector3D> &UV2s,
        std::vector<aiColor4D> &colors, bool &useColors) {
    // read vertices
    do {
        SkipSpacesAndLineEnd(&sz, end);
        aiVector3D temp;
        aiColor4D c;

        // Read the vertex position
        sz = fast_atoreal_move<float>(sz, (float &)temp.x);
        SkipSpaces(&sz, end);

        sz = fast_atoreal_move<float>(sz, (float &)temp.y);
        SkipSpaces(&sz, end);

        sz = fast_atoreal_move<float>(sz, (float &)temp.z);
        SkipSpaces(&sz, end);
        vertices.push_back(temp);

        // Read the vertex normals
        sz = fast_atoreal_move<float>(sz, (float &)temp.x);
        SkipSpaces(&sz, end);

        sz = fast_atoreal_move<float>(sz, (float &)temp.y);
        SkipSpaces(&sz, end);

        sz = fast_atoreal_move<float>(sz, (float &)temp.z);
        SkipSpaces(&sz, end);
        normals.push_back(temp);

        // read the vertex colors
        uint32_t clr = strtoul16(sz, &sz);
        ColorFromARGBPacked(clr, c);

        // If we're pushing more than one distinct color
        if (!colors.empty() && c != *(colors.end() - 1))
            useColors = true;

        colors.push_back(c);
        SkipSpaces(&sz, end);

        // read the first UV coordinate set
        sz = fast_atoreal_move<float>(sz, (float &)temp.x);
        SkipSpaces(&sz, end);

        sz = fast_atoreal_move<float>(sz, (float &)temp.y);
        SkipSpaces(&sz, end);
        temp.z = 0.f;
        temp.y = 1.f - temp.y; // DX to OGL
        UVs.push_back(temp);

        // NOTE these correspond to specific S3DVertex* structs in irr sourcecode
        // So by definition, all buffers have either UV2 or tangents or neither
        // read the (optional) second UV coordinate set
        if (vertexFormat == VertexFormat::t2coord) {
            sz = fast_atoreal_move<float>(sz, (float &)temp.x);
            SkipSpaces(&sz, end);

            sz = fast_atoreal_move<float>(sz, (float &)temp.y);
            temp.y = 1.f - temp.y; // DX to OGL
            UV2s.push_back(temp);
        }
        // read optional tangent and bitangent vectors
        else if (vertexFormat == VertexFormat::tangent) {
            // tangents
            sz = fast_atoreal_move<float>(sz, (float &)temp.x);
            SkipSpaces(&sz, end);

            sz = fast_atoreal_move<float>(sz, (float &)temp.z);
            SkipSpaces(&sz, end);

            sz = fast_atoreal_move<float>(sz, (float &)temp.y);
            SkipSpaces(&sz, end);
            temp.y *= -1.0f;
            tangents.push_back(temp);

            // bitangents
            sz = fast_atoreal_move<float>(sz, (float &)temp.x);
            SkipSpaces(&sz, end);

            sz = fast_atoreal_move<float>(sz, (float &)temp.z);
            SkipSpaces(&sz, end);

            sz = fast_atoreal_move<float>(sz, (float &)temp.y);
            SkipSpaces(&sz, end);
            temp.y *= -1.0f;
            bitangents.push_back(temp);
        }
    } while (SkipLine(&sz, end));
    // IMPORTANT: We assume that each vertex is specified in one
    // line. So we can skip the rest of the line - unknown vertex
    // elements are ignored.
}

#endif // !! ASSIMP_BUILD_NO_IRRMESH_IMPORTER
