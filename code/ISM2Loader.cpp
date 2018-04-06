/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2018, assimp team



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

/** @file ISM2Loader.cpp
 *  @brief Implementation of the main parts of the ISM2 importer class
 */

#ifndef ASSIMP_BUILD_NO_ISM2_IMPORTER

#include <assimp/ByteSwapper.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/scene.h>
#include <assimp/importerdesc.h>
#include <assimp/StreamReader.h>
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include "ISM2Loader.h"
#include "ISM2FileData.h"

using namespace Assimp;
using namespace Assimp::ISM2;

static const aiImporterDesc desc = {
    "Compile Heart ISM2 Importer",
    "",
    "",
    "",
    aiImporterFlags_SupportBinaryFlavour | aiImporterFlags_LimitedSupport |
        aiImporterFlags_Experimental,
    0,
    0,
    0,
    0,
    "ism2"
};

// ------------------------------------------------------------------------------------------------
// Get list of all file extensions that are handled by this loader
const aiImporterDesc* ISM2Importer::GetInfo () const
{
    return &desc;
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file.
bool ISM2Importer::CanRead(const std::string& pFile, IOSystem* pIOHandler, bool checkSig) const
{
    const std::string extension = GetExtension(pFile);

    if (extension == "ism2" || !extension.length() || checkSig) {
        uint32_t token = AI_MAKE_MAGIC(AI_ISM2_MAGIC);

        if (CheckMagicToken(pIOHandler, pFile, &token, 0, 4))
            return true;
    }
    return false;
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure.
void ISM2Importer::InternReadFile( const std::string& pFile, aiScene* pScene,
    IOSystem* pIOHandler)
{
    std::unique_ptr<IOStream> io(pIOHandler->Open(pFile.c_str(), "rb"));

    // looks like there's only one mesh per file
    pScene->mRootNode = new aiNode();
    pScene->mRootNode->mNumMeshes = 1;
    pScene->mRootNode->mMeshes = new unsigned int[1];
    pScene->mRootNode->mMeshes[0] = 0;
    pScene->mNumMeshes = 1;
    pScene->mMeshes = new aiMesh*[1];

    aiMesh *pcMesh = new aiMesh();
    pcMesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
    pScene->mMeshes[0] = pcMesh;

    std::vector<SurfaceHeader> surfaces; // for material index match-ups
    std::vector<aiString> strings;
    //std::vector<Texture> textures;
    std::map<uint32_t, aiBone*> boneIdDict;
    std::vector<Vertex> vertices; // type 1 only

    // Header
    ModelHeaderMeta mhm;
    io->Read(&mhm, sizeof(ModelHeaderMeta), 1);

    ModelHeader mh;
    io->Read(&mh, sizeof(ModelHeader), 1);

    bool le = mh.sectionCount > 0 && mh.sectionCount < 65535 ? true : false;
    StreamReaderAny *sr = new StreamReaderAny(io.get(), le);
    if (le) ByteSwap::Swap(&mh.sectionCount);

    uint32_t *sectionOffsets = new uint32_t[mh.sectionCount];
    uint32_t *sectionTypes = new uint32_t[mh.sectionCount];
    for (uint32_t b = 0; b < mh.sectionCount; b++)
    {
        sectionTypes[b] = sr->GetU4();
        sectionOffsets[b] = sr->GetU4();
    }

    for (uint32_t b = 0; b < mh.sectionCount; b++)
    {
        io->Seek(sectionOffsets[b], aiOrigin_SET);

        switch (sectionTypes[b])
        {
            case Section_Bones:
            {
                BoneDataHeader bdh;

                io->Read(&bdh, sizeof(BoneDataHeader), 1);
                if (!le) ByteSwap::Swap(&bdh.total);
                uint32_t *boneOffsets = new uint32_t[bdh.total];
                pcMesh->mNumBones = bdh.total;
                pcMesh->mBones = new aiBone*[bdh.total];

                for (uint32_t c = 0; c < bdh.total; c++)
                    boneOffsets[c] = sr->GetU4();

                for (uint32_t c = 0; c < bdh.total; c++)
                {
                    aiBone *pBone = new aiBone();
                    BoneHeader bh;

                    io->Seek(boneOffsets[c], aiOrigin_SET);
                    io->Read(&bh, sizeof(BoneHeader), 1);
                    if (!le) {
                        ByteSwap::Swap(&bh.nameStringIndex[0]);
                        ByteSwap::Swap(&bh.id);
                    }

                    uint32_t *boneSectionOffsets = new uint32_t[bh.headerTotal];
                    for (uint32_t d = 0; c < bh.headerTotal; d++)
                        boneSectionOffsets[d] = sr->GetU4();

                    for (uint32_t d = 0; d < bh.headerTotal; d++)
                    {
                        uint32_t boneSectionType = sr->GetU4();

                        switch (boneSectionType) {
                            case Section_SurfaceOffsets:
                            {
                                SurfaceOffsetsHeader soh;

                                io->Read(&soh, sizeof(SurfaceOffsetsHeader), 1);
                                if (!le) ByteSwap::Swap(&soh.total);
                                uint32_t *surfaceOffsets = new uint32_t[soh.total];

                                for (uint32_t e = 0; e < soh.total; e++)
                                    surfaceOffsets[e] = sr->GetU4();

                                for (uint32_t e = 0; e < soh.total; e++)
                                {
                                    SurfaceHeader sh;

                                    io->Seek(surfaceOffsets[e], aiOrigin_SET);
                                    io->Read(&sh, sizeof(SurfaceHeader), 1);
                                    if (!le) ByteSwap::Swap(&sh.textureNameStringIndex);
                                    surfaces.push_back(sh);
                                }

                                break;
                            }
                            case Section_BoneTransforms:
                            {
                                TransformHeader tfh;
                                io->Read(&tfh, sizeof(TransformHeader), 1);
                                if (!le) {
                                    ByteSwap::Swap(&tfh.size);
                                    ByteSwap::Swap(&tfh.total);
                                }

                                uint32_t *transformIndices = new uint32_t[tfh.total];
                                for (uint32_t e = 0; e < tfh.total; e++)
                                    transformIndices[e] = sr->GetU4();

                                aiVector3D scale;
                                float m1[8], m2[8], m3[8], position[3];

                                for (uint32_t t = 0; t < tfh.total; t++)
                                {
                                    io->Seek(transformIndices[t], aiOrigin_SET);
                                    uint32_t transformSectionType = sr->GetU4();

                                    switch (transformSectionType)
                                    {
                                        case Section_BoneTranslation:
                                            position[0] = sr->GetF4();
                                            position[1] = sr->GetF4();
                                            position[2] = sr->GetF4();
                                            break;
                                        case Section_BoneScale:
                                            scale = aiVector3D(sr->GetF4(), sr->GetF4(), sr->GetF4());
                                            break;
                                        case Section_BoneX:
                                            m1[4] = sr->GetF4();
                                            m1[5] = sr->GetF4();
                                            m1[6] = sr->GetF4();
                                            m1[7] = sr->GetF4();
                                            break;
                                        case Section_BoneY:
                                            m2[4] = sr->GetF4();
                                            m2[5] = sr->GetF4();
                                            m2[6] = sr->GetF4();
                                            m2[7] = sr->GetF4();
                                            break;
                                        case Section_BoneZ:
                                            m3[4] = sr->GetF4();
                                            m3[5] = sr->GetF4();
                                            m3[6] = sr->GetF4();
                                            m3[7] = sr->GetF4();
                                            break;
                                        case Section_BoneRotationX:
                                            m1[0] = sr->GetF4();
                                            m1[1] = sr->GetF4();
                                            m1[2] = sr->GetF4();
                                            m1[3] = sr->GetF4();
                                            break;
                                        case Section_BoneRotationY:
                                            m2[0] = sr->GetF4();
                                            m2[1] = sr->GetF4();
                                            m2[2] = sr->GetF4();
                                            m2[3] = sr->GetF4();
                                            break;
                                        case Section_BoneRotationZ:
                                            m3[0] = sr->GetF4();
                                            m3[1] = sr->GetF4();
                                            m3[2] = sr->GetF4();
                                            m3[3] = sr->GetF4();
                                            break;
                                        default:
                                            DefaultLogger::get()->warn(std::string("Unsupported/unknown bone transform section: ") +
                                                std::to_string(transformSectionType));
                                    }
                                }

                                delete[] transformIndices;

                                aiMatrix4x4 transform2;

                                aiMatrix4x4::Scaling(scale, pBone->mOffsetMatrix);
                                pBone->mOffsetMatrix *= aiMatrix4x4().FromEulerAnglesXYZ(m1[7], m2[7], m3[7]);
                                aiMatrix4x4::Scaling(scale, transform2);
                                transform2 *= aiMatrix4x4().FromEulerAnglesXYZ(m1[3], m2[3], m3[3]);
                                pBone->mOffsetMatrix *= transform2;
                                pBone->mOffsetMatrix[3][0] = position[0];
                                pBone->mOffsetMatrix[3][1] = position[1];
                                pBone->mOffsetMatrix[3][2] = position[2];

                                break;
                            }
                            default:
                                DefaultLogger::get()->warn(std::string("Unsupported/unknown bone section: ") +
                                    std::to_string(boneSectionType));
                        }
                    }

                    delete[] boneSectionOffsets;

                    pBone->mName = strings[bh.nameStringIndex[0]];
                    pcMesh->mBones[c] = pBone;
                    boneIdDict[bh.id] = pBone;
                }

                delete[] boneOffsets;
                break;
            }

            case Section_VertexBlockHeader:
            {
                VertexBlockHeader vbh;

                io->Read(&vbh, sizeof(VertexBlockHeader), 1);
                if (!le) ByteSwap::Swap(&vbh.headerTotal);
                uint32_t *vertexDataOffsets = new uint32_t[vbh.headerTotal];

                for (uint32_t c = 0; c < vbh.headerTotal; c++)
                    vertexDataOffsets[c] = sr->GetU4();

                for (uint32_t c = 0; c < vbh.headerTotal; c++)
                {
                    io->Seek(vertexDataOffsets[c], aiOrigin_SET);
                    uint32_t vertexDataSectionType = sr->GetU4();

                    switch (vertexDataSectionType)
                    {
                        case Section_VertexHeaderHeader:
                        {
                            VertexHeaderHeader vhh;

                            io->Read(&vhh, sizeof(VertexHeaderHeader), 1);
                            if (!le) ByteSwap::Swap(&vhh.headerTotal);
                            uint32_t *vertexHeaderOffsets = new uint32_t[vhh.headerTotal];

                            for (uint32_t d = 0; d < vhh.headerTotal; d++)
                                vertexHeaderOffsets[d] = sr->GetU4();

                            for (uint32_t d = 0; d < vhh.headerTotal; d++)
                            {
                                io->Seek(vertexHeaderOffsets[c], aiOrigin_SET);
                                uint32_t vertexHeaderSectionType = sr->GetU4();

                                switch (vertexHeaderSectionType)
                                {
                                    case Section_Polygon:
                                    {
                                        PolygonBlockHeader pbh;

                                        io->Read(&pbh, sizeof(PolygonBlockHeader), 1);
                                        if (!le) {
                                            ByteSwap::Swap(&pbh.dataTotal);
                                            ByteSwap::Swap(&pbh.nameStringIndex);
                                        }
                                        uint32_t *polygonDataOffsets = new uint32_t[pbh.dataTotal];

                                        for (uint32_t e = 0; e < pbh.dataTotal; e++)
                                            polygonDataOffsets[e] = sr->GetU4();

                                        for (uint32_t e = 0; e < pbh.dataTotal; e++)
                                        {
                                            uint32_t polygonDataSectionType = sr->GetU4();

                                            switch (polygonDataSectionType)
                                            {
                                                case Section_PolygonBlock:
                                                {
                                                    PolygonHeader ph;

                                                    io->Read(&ph, sizeof(PolygonHeader), 1);
                                                    if (!le) {
                                                        ByteSwap::Swap(&ph.total);
                                                        ByteSwap::Swap(&ph.type[0]);
                                                    }

                                                    pcMesh->mNumFaces = ph.total / 3;
                                                    pcMesh->mFaces = new aiFace[pcMesh->mNumFaces];

                                                    switch (ph.type[0])
                                                    {
                                                        case 5:
                                                            for (uint32_t g = 0; g < pcMesh->mNumFaces; g++)
                                                            {
                                                                aiFace f;

                                                                f.mNumIndices = 3;
                                                                f.mIndices = new unsigned int[3];
                                                                f.mIndices[0] = sr->GetU2();
                                                                f.mIndices[1] = sr->GetU2();
                                                                f.mIndices[2] = sr->GetU2();
                                                                pcMesh->mFaces[g] = f;
                                                            }
                                                            break;
                                                        case 7:
                                                            for (uint32_t g = 0; g < pcMesh->mNumFaces; g++)
                                                            {
                                                                aiFace f;

                                                                f.mNumIndices = 3;
                                                                f.mIndices = new unsigned int[3];
                                                                f.mIndices[0] = sr->GetU4();
                                                                f.mIndices[1] = sr->GetU4();
                                                                f.mIndices[2] = sr->GetU4();
                                                                pcMesh->mFaces[g] = f;
                                                            }
                                                            break;
                                                        default:
                                                            DefaultLogger::get()->warn(std::string("Unsupported/unknown polygon type: ") +
                                                                std::to_string(ph.type[0]));
                                                    }

                                                    break;
                                                }
                                                default:
                                                    DefaultLogger::get()->warn(std::string("Unsupported/unknown polygon data section: ") +
                                                        std::to_string(polygonDataSectionType));
                                            }
                                        }

                                        delete[] polygonDataOffsets;
                                        break;
                                    }
                                    case Section_VertexBlock:
                                    {
                                        VertexHeader vh;

                                        io->Read(&vh, sizeof(VertexHeader), 1);
                                        if (!le) {
                                            ByteSwap::Swap(&vh.total);
                                            ByteSwap::Swap(&vh.count);
                                            ByteSwap::Swap(&vh.type[0]);
                                        }
                                        uint32_t *vertexOffsets = new uint32_t[vh.total];

                                        for (uint32_t e = 0; e < vh.total; e++)
                                            vertexOffsets[e] = sr->GetU4();

                                        VertexOffsetHeader *vertexOffsetHeaders = new VertexOffsetHeader[vh.total];
                                        for (uint32_t e = 0; e < vh.total; e++)
                                        {
                                            io->Read(&vertexOffsetHeaders[e], sizeof(VertexOffsetHeader), 1);
                                            if (!le) ByteSwap::Swap(&vertexOffsetHeaders[e].startOffset);
                                            io->Seek(vertexOffsetHeaders[e].startOffset, aiOrigin_SET);
                                        }

                                        for (uint32_t e = 0; e < vh.count; e++)
                                        {
                                            switch (vh.type[0])
                                            {
                                                case 1:
                                                {
                                                    Vertex v;

                                                    io->Read(&v, sizeof(Vertex), 1);
                                                    if (!le) {
                                                        ByteSwap::Swap(&v.position[0]);
                                                        ByteSwap::Swap(&v.position[1]);
                                                        ByteSwap::Swap(&v.position[2]);
                                                        ByteSwap::Swap(&v.normal1[0]);
                                                        ByteSwap::Swap(&v.normal1[1]);
                                                        ByteSwap::Swap(&v.normal1[2]);
                                                        ByteSwap::Swap(&v.textureCoordX);
                                                        ByteSwap::Swap(&v.textureCoordY);
                                                    }
                                                    vertices.push_back(v);

                                                    break;
                                                }
                                                default:
                                                    DefaultLogger::get()->warn(std::string("Unsupported/unknown vertex type: ") +
                                                                std::to_string(vh.type[0]));
                                            }
                                        }

                                        delete[] vertexOffsets;
                                        break;
                                    }
                                    default:
                                        DefaultLogger::get()->warn(std::string("Unsupported/unknown vertex header section: ") +
                                            std::to_string(vertexHeaderSectionType));
                                }
                            }

                            delete[] vertexHeaderOffsets;
                            break;
                        }
                        default:
                            DefaultLogger::get()->warn(std::string("Unsupported/unknown vertex data section: ") +
                                std::to_string(vertexDataSectionType));
                    }
                }

                delete[] vertexDataOffsets;

                pcMesh->mNumVertices = vertices.size();
                pcMesh->mVertices = new aiVector3D[vertices.size()];
                pcMesh->mTextureCoords[0] = new aiVector3D[vertices.size()];
                pcMesh->mColors[0] = new aiColor4D[vertices.size()];
                pcMesh->mNormals = new aiVector3D[vertices.size()];

                for (uint32_t i = 0; i < vertices.size(); i++)
                {
                    pcMesh->mVertices[i] = aiVector3D(vertices[i].position[0], vertices[i].position[1],
                        vertices[i].position[2]);
                    pcMesh->mTextureCoords[0][i] = aiVector3D(vertices[i].textureCoordX, vertices[i].textureCoordY, 0);
                    pcMesh->mColors[0][i] = aiColor4D(vertices[i].red / 255, vertices[i].green / 255,
                        vertices[i].blue / 255, vertices[i].alpha / 255);
                    pcMesh->mNormals[i] = aiVector3D(wtof(vertices[i].normal1[0]), wtof(vertices[i].normal1[1]),
                        wtof(vertices[i].normal1[2]));
                }

                break;
            }

            case Section_Strings:
            {
                StringHeader sh;

                io->Read(&sh, sizeof(StringHeader), 1);
                if (!le) ByteSwap::Swap(&sh.total);
                uint32_t *stringOffsets = new uint32_t[sh.total];

                for (uint32_t c = 0; c < sh.total; c++)
                    stringOffsets[c] = sr->GetU4();

                for (uint32_t c = 0; c < sh.total; c++)
                {
                    std::string s = "";
                    char txt = 0;

                    io->Seek(stringOffsets[c], aiOrigin_SET);
                    while ((txt = sr->GetI1()) != 0)
                        s += txt;
                    strings.push_back(aiString(s));
                }

                strings.push_back(aiString("Tex_c.dds")); // for materials with no submats

                delete[] stringOffsets;
                break;
            }

            // Seems redundant as material data also points to tex name, which is all we'd
            // grab from here for now, pending any more info on Texture struct being found
            case Section_Textures:
            {
                /*TextureHeader th;

                io->Read(&th, sizeof(TextureHeader), 1);
                if (!le) ByteSwap::Swap(&th.total);
                uint32_t *textureOffsets = new uint32_t[th.total];

                for (uint32_t c = 0; c < th.total; c++)
                    textureOffsets[c] = sr->GetU4();

                for (uint32_t c = 0; c < th.total; c++)
                {
                    Texture tex;

                    io->Seek(textureOffsets[c], aiOrigin_SET);
                    io->Read(&tex, sizeof(Texture), 1);
                    if (!le) ByteSwap::Swap(&tex.nameStringIndex);
                    textures.push_back(tex);
                }

                delete[] textureOffsets;*/
                break;
            }
            case Section_Materials:
            {
                MaterialHeader matH;

                io->Read(&matH, sizeof(MaterialHeader), 1);
                if (!le) ByteSwap::Swap(&matH.total);
                uint32_t *materialOffsets = new uint32_t[matH.total];
                pScene->mNumMaterials = matH.total;
                pScene->mMaterials = new aiMaterial*[matH.total];

                for (uint32_t z = 0; z < matH.total; z++)
                    materialOffsets[z] = sr->GetU4();

                for (uint32_t z = 0; z < matH.total; z++)
                {
                    MaterialA ma;
                    aiMaterial *pMat = new aiMaterial();

                    io->Seek(materialOffsets[z], aiOrigin_SET);
                    io->Read(&ma, sizeof(MaterialA), 1);
                    if (!le) {
                        ByteSwap::Swap(&ma.nameStringIndex);
                        ByteSwap::Swap(&ma.total);
                    }

                    pMat->AddProperty(&strings[ma.nameStringIndex], AI_MATKEY_NAME);

                    if (ma.total > 0)
                    {
                        MaterialB mb;
                        MaterialC mc;
                        MaterialD md;
                        MaterialE me;
                        uint32_t mbOffset = sr->GetU4();

                        io->Seek(mbOffset, aiOrigin_SET);
                        io->Read(&mb, sizeof(MaterialB), 1);
                        if (!le) ByteSwap::Swap(&mb.cOffset);

                        io->Seek(mb.cOffset, aiOrigin_SET);
                        io->Read(&mc, sizeof(MaterialC), 1);
                        if (!le) ByteSwap::Swap(&mc.dOffset);

                        io->Seek(mc.dOffset, aiOrigin_SET);
                        io->Read(&md, sizeof(MaterialD), 1);
                        if (!le) ByteSwap::Swap(&md.eOffset);

                        io->Seek(md.eOffset, aiOrigin_SET);
                        io->Read(&me, sizeof(MaterialE), 1);
                        if (!le) ByteSwap::Swap(&me.fOffset);

                        io->Seek(me.fOffset, aiOrigin_SET);
                        uint32_t matTexNameStringIndex = sr->GetU4();
                        pMat->AddProperty(&strings[matTexNameStringIndex],
                            AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
                    }
                    else
                        pMat->AddProperty(&strings[strings.size()-1], AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

                    pScene->mMaterials[z] = pMat;
                }

                delete[] materialOffsets;
                break;
            }

            default:
                DefaultLogger::get()->warn(std::string("Unsupported/unknown section: ") +
                    std::to_string(sectionTypes[b]));
        }
    }

    delete[] sectionOffsets;
    delete[] sectionTypes;
    delete sr;
}

#endif // ASSIMP_BUILD_NO_ISM2_IMPORTER
