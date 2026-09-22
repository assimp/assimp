/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2026, assimp team

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

/** @file  PlyLoader.cpp
 *  @brief Implementation of the PLY importer class
 */

#ifndef ASSIMP_BUILD_NO_PLY_IMPORTER

// internal headers
#include "PlyLoader.h"

// standard headers
#include <assimp/IOStreamBuffer.h>
#include <assimp/importerdesc.h>
#include <assimp/scene.h>
#include <assimp/IOSystem.hpp>
#include <assimp/metadata.h>
#include <assimp/gaussian.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogAux.h>

#include "Common/ScenePrivate.h"

// other headers
#include <memory>
#include <cstdlib>
#include <cstring>

namespace Assimp {

    // ------------------------------------------------------------------------------------------------
    static constexpr ai_uint NotSet = 0xFFFFFFFF;

    static constexpr aiImporterDesc desc = {
        "Stanford Polygon Library (PLY) Importer",
        "",
        "",
        "",
        aiImporterFlags_SupportBinaryFlavour | aiImporterFlags_SupportTextFlavour,
        0,
        0,
        0,
        0,
        "ply"
    };

    namespace { // Internal stuff
        // ------------------------------------------------------------------------------------------------
        // Checks that property index is within range
        template <class T>
        const T &GetProperty(const std::vector<T> &props, int idx) {
            if (static_cast<size_t>(idx) >= props.size()) {
                throw DeadlyImportError("Invalid .ply file: Property index is out of range.");
            }

            return props[idx];
        }

        // ------------------------------------------------------------------------------------------------
        bool isBigEndian(const char *szMe) {
            ai_assert(nullptr != szMe);

            // binary_little_endian
            // binary_big_endian
            bool isBigEndian{ false };
    #if (defined AI_BUILD_BIG_ENDIAN)
            if ('l' == *szMe || 'L' == *szMe) {
                isBigEndian = true;
            }
    #else
            if ('b' == *szMe || 'B' == *szMe) {
                isBigEndian = true;
            }
    #endif // ! AI_BUILD_BIG_ENDIAN

            return isBigEndian;
        }

        // ------------------------------------------------------------------------------------------------
        // Convert a color component to [0...1]
        ai_real NormalizeColorValue(PropertyInstance::ValueUnion val, EDataType eType) {
            switch (eType) {
                case EDT_Float:
                    return val.fFloat;
                case EDT_Double:
                    return static_cast<ai_real>(val.fDouble);
                case EDT_UChar:
                    return static_cast<ai_real>(val.iUInt) / static_cast<ai_real>(0xFF);
                case EDT_Char:
                    return static_cast<ai_real>(val.iInt + (0xFF / 2)) / static_cast<ai_real>(0xFF);
                case EDT_UShort:
                    return static_cast<ai_real>(val.iUInt) / static_cast<ai_real>(0xFFFF);
                case EDT_Short:
                    return static_cast<ai_real>(val.iInt + (0xFFFF / 2)) / static_cast<ai_real>(0xFFFF);
                case EDT_UInt:
                    return static_cast<ai_real>(val.iUInt) / static_cast<ai_real>(0xFFFF);
                case EDT_Int:
                    return (static_cast<ai_real>(val.iInt) / static_cast<ai_real>(0xFF)) + 0.5f;
                default:
                    break;
            }

            return 0.0f;
        }

        // ------------------------------------------------------------------------------------------------
        // Get a RGBA color in [0...1] range
        void GetMaterialColor(const std::vector<PLY::PropertyInstance> &avList, unsigned int positions[4], EDataType types[4],
                aiColor4D *clrOut) {
            ai_assert(nullptr != clrOut);

            if (NotSet == positions[0]) {
                clrOut->r = 0.0f;
            } else {
                clrOut->r = NormalizeColorValue(GetProperty(avList, positions[0]).avList.front(), types[0]);
            }

            if (NotSet == positions[1]) {
                clrOut->g = 0.0f;
            } else {
                clrOut->g = NormalizeColorValue(GetProperty(avList, positions[1]).avList.front(), types[1]);
            }

            if (NotSet == positions[2]) {
                clrOut->b = 0.0f;
            } else {
                clrOut->b = NormalizeColorValue(GetProperty(avList, positions[2]).avList.front(), types[2]);
            }

            // assume 1.0 for the alpha channel ifit is not set
            if (NotSet == positions[3]) {
                clrOut->a = 1.0f;
            } else {
                clrOut->a = NormalizeColorValue(GetProperty(avList, positions[3]).avList.front(), types[3]);
            }
        }

    } // namespace

    // ------------------------------------------------------------------------------------------------
    PLYImporter::~PLYImporter() {
        delete mGeneratedMesh;
        delete mGaussianSplat;
    }

    // ------------------------------------------------------------------------------------------------
    // Returns whether the class can handle the format of the given file.
    bool PLYImporter::CanRead(const std::string &pFile, IOSystem *pIOHandler, bool) const {
        static const char *tokens[] = { "ply" };
        return SearchFileHeaderForToken(pIOHandler, pFile, tokens, AI_COUNT_OF(tokens));
    }

    // ------------------------------------------------------------------------------------------------
    const aiImporterDesc *PLYImporter::GetInfo() const {
        return &desc;
    }

    // ------------------------------------------------------------------------------------------------
    // Imports the given file into the given scene structure.
    void PLYImporter::InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler) {
        // Reset per-import Gaussian state (importer instances are reused).
        mGaussianChecked = false;
        mGaussianActive = false;
        delete mGaussianSplat;
        mGaussianSplat = nullptr;
        mGsRest.clear();
        mGsOpacity = -1;
        for (int i = 0; i < 3; ++i) {
            mGsDc[i] = -1;
            mGsScale[i] = -1;
        }
        for (int i = 0; i < 4; ++i) {
            mGsRot[i] = -1;
        }

        const std::string mode = "rb";
        std::unique_ptr<IOStream> fileStream(pIOHandler->Open(pFile, mode));
        if (!fileStream) {
            throw DeadlyImportError("Failed to open file ", pFile, ".");
        }

        // Get the file-size
        const size_t fileSize = fileStream->FileSize();
        if (0 == fileSize) {
            throw DeadlyImportError("File ", pFile, " is empty.");
        }

        IOStreamBuffer<char> streamedBuffer(1024 * 1024);
        streamedBuffer.open(fileStream.get());

        // the beginning of the file must be PLY - magic, magic
        std::vector<char> headerCheck;
        streamedBuffer.getNextLine(headerCheck);

        if ((headerCheck.size() < 3) ||
                (headerCheck[0] != 'P' && headerCheck[0] != 'p') ||
                (headerCheck[1] != 'L' && headerCheck[1] != 'l') ||
                (headerCheck[2] != 'Y' && headerCheck[2] != 'y')) {
            streamedBuffer.close();
            throw DeadlyImportError("Invalid .ply file: Incorrect magic number (expected 'ply' or 'PLY').");
        }

        std::vector<char> mBuffer2;
        streamedBuffer.getNextLine(mBuffer2);
        mBuffer = (unsigned char *)&mBuffer2[0];

        auto szMe = (char *)&this->mBuffer[0];
        const char *end = &mBuffer2[0] + mBuffer2.size();
        SkipSpacesAndLineEnd(szMe, (const char **)&szMe, end);

        // determine the format of the file data and construct the aiMesh
        DOM sPlyDom;
        this->pcDOM = &sPlyDom;

        if (TokenMatch(szMe, "format", 6)) {
            if (TokenMatch(szMe, "ascii", 5)) {
                SkipLine(szMe, (const char **)&szMe, end);
                if (!DOM::ParseInstance(streamedBuffer, &sPlyDom, this)) {
                    if (mGeneratedMesh != nullptr) {
                        delete (mGeneratedMesh);
                        mGeneratedMesh = nullptr;
                    }

                    streamedBuffer.close();
                    throw DeadlyImportError("Invalid .ply file: Unable to build DOM (#1)");
                }
            } else if (!strncmp(szMe, "binary_", 7)) {
                szMe += 7;

                // skip the line, parse the rest of the header and build the DOM
                if (const bool bIsBE = isBigEndian(szMe); !PLY::DOM::ParseInstanceBinary(streamedBuffer, &sPlyDom, this, bIsBE)) {
                    if (mGeneratedMesh != nullptr) {
                        delete (mGeneratedMesh);
                        mGeneratedMesh = nullptr;
                    }

                    streamedBuffer.close();
                    throw DeadlyImportError("Invalid .ply file: Unable to build DOM (#2)");
                }
            } else {
                if (mGeneratedMesh != nullptr) {
                    delete mGeneratedMesh;
                    mGeneratedMesh = nullptr;
                }

                streamedBuffer.close();
                throw DeadlyImportError("Invalid .ply file: Unknown file format");
            }
        } else {
            AI_DEBUG_INVALIDATE_PTR(this->mBuffer);
            if (mGeneratedMesh != nullptr) {
                delete (mGeneratedMesh);
                mGeneratedMesh = nullptr;
            }

            streamedBuffer.close();
            throw DeadlyImportError("Invalid .ply file: Missing format specification");
        }

        // free the file buffer
        streamedBuffer.close();

        if (mGeneratedMesh == nullptr) {
            throw DeadlyImportError("Invalid .ply file: Unable to extract mesh data ");
        }

        // if no face list is existing we assume that the vertex
        // list is containing a list of points (issue #623: do not invent faces).
        // 3DGS PLY is the same layout; ValidateDataStructure will reject it —
        // load with flags == 0 and read aiGetGaussianSplat().
        bool pointsOnly = mGeneratedMesh->mFaces == nullptr ? true : false;
        if (pointsOnly) {
            mGeneratedMesh->mPrimitiveTypes = aiPrimitiveType::aiPrimitiveType_POINT;
        }

        // now load a list of all materials
        std::vector<aiMaterial *> avMaterials;
        std::string defaultTexture;
        LoadMaterial(&avMaterials, defaultTexture, pointsOnly);

        // now generate the output scene object. Fill the material list
        pScene->mNumMaterials = static_cast<unsigned int>(avMaterials.size());
        pScene->mMaterials = new aiMaterial *[pScene->mNumMaterials];
        for (unsigned int i = 0; i < pScene->mNumMaterials; ++i) {
            pScene->mMaterials[i] = avMaterials[i];
        }

        // fill the mesh list
        pScene->mNumMeshes = 1;
        pScene->mMeshes = new aiMesh *[pScene->mNumMeshes];
        pScene->mMeshes[0] = mGeneratedMesh;

        // Move the mesh ownership into the scene instance
        mGeneratedMesh = nullptr;

        // generate a simple node structure
        pScene->mRootNode = new aiNode();
        pScene->mRootNode->mNumMeshes = pScene->mNumMeshes;
        pScene->mRootNode->mMeshes = new unsigned int[pScene->mNumMeshes];

        for (unsigned int i = 0; i < pScene->mRootNode->mNumMeshes; ++i) {
            pScene->mRootNode->mMeshes[i] = i;
        }

        // Attach 3DGS side data (ABI-safe: scene-private, not aiMesh fields).
        if (mGaussianActive && mGaussianSplat != nullptr) {
            const int32_t restCount = static_cast<int32_t>(mGaussianSplat->mNumRestCoeffs);
            AttachGaussianSplat(pScene, 0, mGaussianSplat);
            mGaussianSplat = nullptr; // ownership moved

            if (pScene->mMetaData == nullptr) {
                pScene->mMetaData = new aiMetadata();
            }
            pScene->mMetaData->Add(AI_METADATA_GAUSSIAN_SPLAT, true);
            pScene->mMetaData->Add(AI_METADATA_GAUSSIAN_SH_REST_COUNT, restCount);

            if (pScene->mRootNode->mMetaData == nullptr) {
                pScene->mRootNode->mMetaData = new aiMetadata();
            }
            pScene->mRootNode->mMetaData->Add(AI_METADATA_GAUSSIAN_SPLAT, true);
        }
    }

    // ------------------------------------------------------------------------------------------------
    void PLYImporter::PrepareGaussianIfNeeded(const Element *pcElement) {
        if (mGaussianChecked || pcElement == nullptr) {
            return;
        }
        mGaussianChecked = true;

        int dc[3] = { -1, -1, -1 };
        int scale[3] = { -1, -1, -1 };
        int rot[4] = { -1, -1, -1, -1 };
        int opacity = -1;
        int maxRest = -1;
        std::vector<int> restSlots;

        auto matchPrefixedIndex = [](const std::string &name, const char *prefix) -> int {
            const size_t plen = std::strlen(prefix);
            if (name.size() <= plen || name.compare(0, plen, prefix) != 0) {
                return -1;
            }
            char *end = nullptr;
            const long idx = std::strtol(name.c_str() + plen, &end, 10);
            if (end == name.c_str() + plen || *end != '\0' || idx < 0 || idx > 64) {
                return -1;
            }
            return static_cast<int>(idx);
        };

        unsigned int propIndex = 0;
        for (const Property &prop : pcElement->alProperties) {
            if (prop.bIsList) {
                ++propIndex;
                continue;
            }

            if (prop.Semantic == EST_Opacity || prop.szName == "opacity") {
                opacity = static_cast<int>(propIndex);
            } else if (!prop.szName.empty()) {
                int idx = matchPrefixedIndex(prop.szName, "f_dc_");
                if (idx >= 0 && idx < 3) {
                    dc[idx] = static_cast<int>(propIndex);
                }
                idx = matchPrefixedIndex(prop.szName, "scale_");
                if (idx >= 0 && idx < 3) {
                    scale[idx] = static_cast<int>(propIndex);
                }
                idx = matchPrefixedIndex(prop.szName, "rot_");
                if (idx >= 0 && idx < 4) {
                    rot[idx] = static_cast<int>(propIndex);
                }
                idx = matchPrefixedIndex(prop.szName, "f_rest_");
                if (idx >= 0) {
                    if (idx > maxRest) {
                        maxRest = idx;
                        restSlots.resize(static_cast<size_t>(maxRest) + 1u, -1);
                    }
                    restSlots[static_cast<size_t>(idx)] = static_cast<int>(propIndex);
                }
            }
            ++propIndex;
        }

        const bool haveCore =
                dc[0] >= 0 && dc[1] >= 0 && dc[2] >= 0 &&
                scale[0] >= 0 && scale[1] >= 0 && scale[2] >= 0 &&
                rot[0] >= 0 && rot[1] >= 0 && rot[2] >= 0 && rot[3] >= 0 &&
                opacity >= 0;

        if (!haveCore) {
            return;
        }

        // Require contiguous f_rest_0 .. f_rest_max when any rest is present.
        const unsigned int restCount = (maxRest >= 0) ? static_cast<unsigned int>(maxRest + 1) : 0u;
        for (unsigned int i = 0; i < restCount; ++i) {
            if (restSlots[i] < 0) {
                ASSIMP_LOG_WARN("PLY: incomplete f_rest_* set; skipping Gaussian side data");
                return;
            }
        }

        mGsDc[0] = dc[0];
        mGsDc[1] = dc[1];
        mGsDc[2] = dc[2];
        mGsScale[0] = scale[0];
        mGsScale[1] = scale[1];
        mGsScale[2] = scale[2];
        mGsRot[0] = rot[0];
        mGsRot[1] = rot[1];
        mGsRot[2] = rot[2];
        mGsRot[3] = rot[3];
        mGsOpacity = opacity;
        mGsRest = std::move(restSlots);

        mGaussianSplat = new aiGaussianSplat();
        mGaussianSplat->mNumPoints = pcElement->NumOccur;
        mGaussianSplat->mNumRestCoeffs = restCount;
        mGaussianSplat->mDC = new aiVector3D[pcElement->NumOccur];
        mGaussianSplat->mScale = new aiVector3D[pcElement->NumOccur];
        mGaussianSplat->mRotation = new aiColor4D[pcElement->NumOccur];
        mGaussianSplat->mOpacity = new ai_real[pcElement->NumOccur];
        if (restCount > 0) {
            const size_t restTotal = static_cast<size_t>(pcElement->NumOccur) * static_cast<size_t>(restCount);
            mGaussianSplat->mRest = new ai_real[restTotal];
        }

        mGaussianActive = true;
        ASSIMP_LOG_INFO("PLY: detected 3D Gaussian Splatting vertex layout");
    }

    // ------------------------------------------------------------------------------------------------
    void PLYImporter::LoadGaussianVertex(const ElementInstance *instElement, unsigned int pos) {
        if (!mGaussianActive || mGaussianSplat == nullptr || instElement == nullptr) {
            return;
        }
        if (pos >= mGaussianSplat->mNumPoints) {
            throw DeadlyImportError("Invalid .ply file: Too many Gaussian vertices");
        }

        auto readFloat = [&](int propIndex) -> ai_real {
            const PropertyInstance &pi = GetProperty(instElement->alProperties, propIndex);
            return PropertyInstance::ConvertTo<ai_real>(pi.avList.front(), EDT_Float);
        };

        mGaussianSplat->mDC[pos].x = readFloat(mGsDc[0]);
        mGaussianSplat->mDC[pos].y = readFloat(mGsDc[1]);
        mGaussianSplat->mDC[pos].z = readFloat(mGsDc[2]);

        mGaussianSplat->mScale[pos].x = readFloat(mGsScale[0]);
        mGaussianSplat->mScale[pos].y = readFloat(mGsScale[1]);
        mGaussianSplat->mScale[pos].z = readFloat(mGsScale[2]);

        mGaussianSplat->mRotation[pos].r = readFloat(mGsRot[0]);
        mGaussianSplat->mRotation[pos].g = readFloat(mGsRot[1]);
        mGaussianSplat->mRotation[pos].b = readFloat(mGsRot[2]);
        mGaussianSplat->mRotation[pos].a = readFloat(mGsRot[3]);

        mGaussianSplat->mOpacity[pos] = readFloat(mGsOpacity);

        if (mGaussianSplat->mNumRestCoeffs > 0 && mGaussianSplat->mRest != nullptr) {
            ai_real *dst = mGaussianSplat->mRest +
                           static_cast<size_t>(pos) * static_cast<size_t>(mGaussianSplat->mNumRestCoeffs);
            for (unsigned int i = 0; i < mGaussianSplat->mNumRestCoeffs; ++i) {
                dst[i] = readFloat(mGsRest[static_cast<size_t>(i)]);
            }
        }
    }

    void PLYImporter::LoadVertex(const Element *pcElement, const ElementInstance *instElement, unsigned int pos) {
        ai_assert(nullptr != pcElement);
        ai_assert(nullptr != instElement);

        PrepareGaussianIfNeeded(pcElement);

        ai_uint aiPositions[3] = { NotSet, NotSet, NotSet };
        EDataType aiTypes[3] = { EDT_Char, EDT_Char, EDT_Char };

        ai_uint aiNormal[3] = { NotSet, NotSet, NotSet };
        EDataType aiNormalTypes[3] = { EDT_Char, EDT_Char, EDT_Char };

        unsigned int aiColors[4] = { NotSet, NotSet, NotSet, NotSet };
        EDataType aiColorsTypes[4] = { EDT_Char, EDT_Char, EDT_Char, EDT_Char };

        unsigned int aiTexcoord[2] = { NotSet, NotSet };
        EDataType aiTexcoordTypes[2] = { EDT_Char, EDT_Char };

        // now check whether which normal components are available
        unsigned int _a(0), cnt(0);
        for (auto a = pcElement->alProperties.begin(); a != pcElement->alProperties.end(); ++a, ++_a) {
            if (a->bIsList) {
                continue;
            }

            // Positions
            if (EST_XCoord == a->Semantic) {
                ++cnt;
                aiPositions[0] = _a;
                aiTypes[0] = a->eType;
            } else if (EST_YCoord == a->Semantic) {
                ++cnt;
                aiPositions[1] = _a;
                aiTypes[1] = a->eType;
            } else if (EST_ZCoord == a->Semantic) {
                ++cnt;
                aiPositions[2] = _a;
                aiTypes[2] = a->eType;
            } else if (EST_XNormal == a->Semantic) {
                // Normals
                ++cnt;
                aiNormal[0] = _a;
                aiNormalTypes[0] = a->eType;
            } else if (EST_YNormal == a->Semantic) {
                ++cnt;
                aiNormal[1] = _a;
                aiNormalTypes[1] = a->eType;
            } else if (EST_ZNormal == a->Semantic) {
                ++cnt;
                aiNormal[2] = _a;
                aiNormalTypes[2] = a->eType;
            } else if (EST_Red == a->Semantic) {
                // Colors
                ++cnt;
                aiColors[0] = _a;
                aiColorsTypes[0] = a->eType;
            } else if (EST_Green == a->Semantic) {
                ++cnt;
                aiColors[1] = _a;
                aiColorsTypes[1] = a->eType;
            } else if (EST_Blue == a->Semantic) {
                ++cnt;
                aiColors[2] = _a;
                aiColorsTypes[2] = a->eType;
            } else if (EST_Alpha == a->Semantic) {
                ++cnt;
                aiColors[3] = _a;
                aiColorsTypes[3] = a->eType;
            } else if (EST_UTextureCoord == a->Semantic) {
                // Texture coordinates
                ++cnt;
                aiTexcoord[0] = _a;
                aiTexcoordTypes[0] = a->eType;
            } else if (EST_VTextureCoord == a->Semantic) {
                ++cnt;
                aiTexcoord[1] = _a;
                aiTexcoordTypes[1] = a->eType;
            }
        }

        // check whether we have a valid source for the vertex data
        if (0 != cnt) {
            // Position
            aiVector3D vOut;
            if (NotSet != aiPositions[0]) {
                vOut.x = PropertyInstance::ConvertTo<ai_real>(
                        GetProperty(instElement->alProperties, aiPositions[0]).avList.front(), aiTypes[0]);
            }

            if (NotSet != aiPositions[1]) {
                vOut.y = PropertyInstance::ConvertTo<ai_real>(
                        GetProperty(instElement->alProperties, aiPositions[1]).avList.front(), aiTypes[1]);
            }

            if (NotSet != aiPositions[2]) {
                vOut.z = PropertyInstance::ConvertTo<ai_real>(
                        GetProperty(instElement->alProperties, aiPositions[2]).avList.front(), aiTypes[2]);
            }

            // Normals
            aiVector3D nOut;
            bool haveNormal = false;
            if (NotSet != aiNormal[0]) {
                nOut.x = PropertyInstance::ConvertTo<ai_real>(
                        GetProperty(instElement->alProperties, aiNormal[0]).avList.front(), aiNormalTypes[0]);
                haveNormal = true;
            }

            if (NotSet != aiNormal[1]) {
                nOut.y = PropertyInstance::ConvertTo<ai_real>(
                        GetProperty(instElement->alProperties, aiNormal[1]).avList.front(), aiNormalTypes[1]);
                haveNormal = true;
            }

            if (NotSet != aiNormal[2]) {
                nOut.z = PropertyInstance::ConvertTo<ai_real>(
                        GetProperty(instElement->alProperties, aiNormal[2]).avList.front(), aiNormalTypes[2]);
                haveNormal = true;
            }

            // Colors
            aiColor4D cOut;
            bool haveColor = false;
            if (NotSet != aiColors[0]) {
                cOut.r = NormalizeColorValue(GetProperty(instElement->alProperties,
                                                    aiColors[0])
                                                    .avList.front(),
                        aiColorsTypes[0]);
                haveColor = true;
            }

            if (NotSet != aiColors[1]) {
                cOut.g = NormalizeColorValue(GetProperty(instElement->alProperties,
                                                    aiColors[1])
                                                    .avList.front(),
                        aiColorsTypes[1]);
                haveColor = true;
            }

            if (NotSet != aiColors[2]) {
                cOut.b = NormalizeColorValue(GetProperty(instElement->alProperties,
                                                    aiColors[2])
                                                    .avList.front(),
                        aiColorsTypes[2]);
                haveColor = true;
            }

            // assume 1.0 for the alpha channel if it is not set
            if (NotSet == aiColors[3]) {
                cOut.a = 1.0;
            } else {
                cOut.a = NormalizeColorValue(GetProperty(instElement->alProperties,
                                                    aiColors[3])
                                                    .avList.front(),
                        aiColorsTypes[3]);

                haveColor = true;
            }

            // Texture coordinates
            aiVector3D tOut;
            tOut.z = 0;
            bool haveTextureCoords = false;
            if (NotSet != aiTexcoord[0]) {
                tOut.x = PropertyInstance::ConvertTo<ai_real>(
                        GetProperty(instElement->alProperties, aiTexcoord[0]).avList.front(), aiTexcoordTypes[0]);
                haveTextureCoords = true;
            }

            if (NotSet != aiTexcoord[1]) {
                tOut.y = PropertyInstance::ConvertTo<ai_real>(
                        GetProperty(instElement->alProperties, aiTexcoord[1]).avList.front(), aiTexcoordTypes[1]);
                haveTextureCoords = true;
            }

            // create aiMesh if needed
            if (nullptr == mGeneratedMesh) {
                mGeneratedMesh = new aiMesh();
                mGeneratedMesh->mMaterialIndex = 0;
            }

            if (nullptr == mGeneratedMesh->mVertices) {
                mGeneratedMesh->mNumVertices = pcElement->NumOccur;
                mGeneratedMesh->mVertices = new aiVector3D[mGeneratedMesh->mNumVertices];
            }
            if (pos >= mGeneratedMesh->mNumVertices) {
                throw DeadlyImportError("Invalid .ply file: Too many vertices");
            }

            mGeneratedMesh->mVertices[pos] = vOut;

            if (haveNormal) {
                if (nullptr == mGeneratedMesh->mNormals)
                    mGeneratedMesh->mNormals = new aiVector3D[mGeneratedMesh->mNumVertices];
                mGeneratedMesh->mNormals[pos] = nOut;
            }

            if (haveColor) {
                if (nullptr == mGeneratedMesh->mColors[0])
                    mGeneratedMesh->mColors[0] = new aiColor4D[mGeneratedMesh->mNumVertices];
                mGeneratedMesh->mColors[0][pos] = cOut;
            }

            if (haveTextureCoords) {
                if (nullptr == mGeneratedMesh->mTextureCoords[0]) {
                    mGeneratedMesh->mNumUVComponents[0] = 2;
                    mGeneratedMesh->mTextureCoords[0] = new aiVector3D[mGeneratedMesh->mNumVertices];
                }
                mGeneratedMesh->mTextureCoords[0][pos] = tOut;
            }

            LoadGaussianVertex(instElement, pos);
        } else if (mGaussianActive) {
            // Positions missing but splat layout present — still fill side data if possible.
            LoadGaussianVertex(instElement, pos);
        }
    }

    // ------------------------------------------------------------------------------------------------
    // Try to extract proper faces from the PLY DOM
    void PLYImporter::LoadFace(const Element *pcElement, const ElementInstance *instElement,
            unsigned int pos) {
        ai_assert(nullptr != pcElement);
        ai_assert(nullptr != instElement);

        if (mGeneratedMesh == nullptr) {
            throw DeadlyImportError("Invalid .ply file: Vertices should be declared before faces");
        }

        bool bOne = false;

        // index of the vertex index list
        unsigned int iProperty = NotSet;
        EDataType eType = EDT_Char;
        bool bIsTriStrip = false;

        // texture coordinates
        unsigned int iTextureCoord = NotSet;
        EDataType eType3 = EDT_Char;

        // face = unique number of vertex indices
        if (EEST_Face == pcElement->eSemantic) {
            unsigned int _a = 0;
            for (std::vector<Property>::const_iterator a = pcElement->alProperties.begin();
                    a != pcElement->alProperties.end(); ++a, ++_a) {
                if (EST_VertexIndex == a->Semantic) {
                    // must be a dynamic list!
                    if (!a->bIsList) {
                        continue;
                    }

                    iProperty = _a;
                    bOne = true;
                    eType = a->eType;
                } else if (EST_TextureCoordinates == a->Semantic) {
                    // must be a dynamic list!
                    if (!a->bIsList) {
                        continue;
                    }
                    iTextureCoord = _a;
                    bOne = true;
                    eType3 = a->eType;
                }
            }
        }
        // triangle strip
        // TODO: material index support???
        else if (EEST_TriStrip == pcElement->eSemantic) {
            unsigned int _a = 0;
            for (auto a = pcElement->alProperties.begin(); a != pcElement->alProperties.end(); ++a, ++_a) {
                // must be a dynamic list!
                if (!a->bIsList) {
                    continue;
                }
                iProperty = _a;
                bOne = true;
                bIsTriStrip = true;
                eType = a->eType;
                break;
            }
        }

        // check whether we have at least one per-face information set
        if (bOne) {
            if (mGeneratedMesh->mFaces == nullptr) {
                mGeneratedMesh->mNumFaces = pcElement->NumOccur;
                mGeneratedMesh->mFaces = new aiFace[mGeneratedMesh->mNumFaces];
            } else {
                if (mGeneratedMesh->mNumFaces < pcElement->NumOccur) {
                    throw DeadlyImportError("Invalid .ply file: Too many faces");
                }
            }

            if (!bIsTriStrip) {
                // parse the list of vertex indices
                if (NotSet != iProperty) {
                    const unsigned int iNum = static_cast<unsigned int>(GetProperty(instElement->alProperties, iProperty).avList.size());
                    mGeneratedMesh->mFaces[pos].mNumIndices = iNum;
                    mGeneratedMesh->mFaces[pos].mIndices = new unsigned int[iNum];

                    std::vector<PropertyInstance::ValueUnion>::const_iterator p =
                            GetProperty(instElement->alProperties, iProperty).avList.begin();

                    for (unsigned int a = 0; a < iNum; ++a, ++p) {
                        mGeneratedMesh->mFaces[pos].mIndices[a] = PropertyInstance::ConvertTo<unsigned int>(*p, eType);
                    }
                }

                if (NotSet != iTextureCoord) {
                    const auto iNum = static_cast<unsigned int>(GetProperty(instElement->alProperties, iTextureCoord).avList.size());

                    // should be 6 coords
                    auto p = GetProperty(instElement->alProperties, iTextureCoord).avList.begin();
                    if ((iNum / 3) == 2) { // X Y coord
                        for (unsigned int a = 0; a < iNum; ++a, ++p) {
                            unsigned int vindex = mGeneratedMesh->mFaces[pos].mIndices[a / 2];
                            if (vindex < mGeneratedMesh->mNumVertices) {
                                if (mGeneratedMesh->mTextureCoords[0] == nullptr) {
                                    mGeneratedMesh->mNumUVComponents[0] = 2;
                                    mGeneratedMesh->mTextureCoords[0] = new aiVector3D[mGeneratedMesh->mNumVertices];
                                }

                                if (a % 2 == 0) {
                                    mGeneratedMesh->mTextureCoords[0][vindex].x = PLY::PropertyInstance::ConvertTo<ai_real>(*p, eType3);
                                } else {
                                    mGeneratedMesh->mTextureCoords[0][vindex].y = PLY::PropertyInstance::ConvertTo<ai_real>(*p, eType3);
                                }

                                mGeneratedMesh->mTextureCoords[0][vindex].z = 0;
                            }
                        }
                    }
                }
            } else { // triangle strips
                // normally we have only one triangle strip instance where
                // a value of -1 indicates a restart of the strip
                createFromeTriStrip(instElement, iProperty, eType);
            }
        }
    }

    // ------------------------------------------------------------------------------------------------
    void PLYImporter::createFromeTriStrip(const ElementInstance *instElement, unsigned int iProperty, EDataType eType) {
        bool flip = false;
        const auto &quak = GetProperty(instElement->alProperties, iProperty).avList;

        std::vector<aiFace> cache;
        int aiTable[2] = { -1, -1 };
        for (auto a = quak.begin(); a != quak.end(); ++a) {
            const int p = PropertyInstance::ConvertTo<int>(*a, eType);

            if (-1 == p) {
                // restart the strip ...
                aiTable[0] = aiTable[1] = -1;
                flip = false;
                continue;
            }
            
            if (-1 == aiTable[0]) {
                aiTable[0] = p;
                continue;
            }
            
            if (-1 == aiTable[1]) {
                aiTable[1] = p;
                continue;
            }
            aiFace face;
            face.mNumIndices = 3;
            face.mIndices = new unsigned int[3];
            face.mIndices[0] = aiTable[0];
            face.mIndices[1] = aiTable[1];
            face.mIndices[2] = p;
            if (flip) {
                std::swap(face.mIndices[0], face.mIndices[1]);
            }
            cache.push_back(face);
            // every second pass swap the indices.
            flip = !flip;

            aiTable[0] = aiTable[1];
            aiTable[1] = p;
        }
        if (mGeneratedMesh->mFaces != nullptr) {
            delete[] mGeneratedMesh->mFaces;
        }
        mGeneratedMesh->mNumFaces = static_cast<unsigned int>(cache.size());
        mGeneratedMesh->mFaces = new aiFace[mGeneratedMesh->mNumFaces];
        std::copy(cache.begin(), cache.end(), mGeneratedMesh->mFaces);
    }

    // ------------------------------------------------------------------------------------------------
    // Extract a material from the PLY DOM
    void PLYImporter::LoadMaterial(std::vector<aiMaterial *> *pvOut, std::string &defaultTexture, const bool pointsOnly) {
        ai_assert(nullptr != pvOut);

        // diffuse[4], specular[4], ambient[4]
        // rgba order
        unsigned int aaiPositions[3][4] = {
            { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF },
            { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF },
            { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF },
        };

        EDataType aaiTypes[3][4] = {
            { EDT_Char, EDT_Char, EDT_Char, EDT_Char },
            { EDT_Char, EDT_Char, EDT_Char, EDT_Char },
            { EDT_Char, EDT_Char, EDT_Char, EDT_Char }
        };
        ElementInstanceList *pcList = nullptr;

        unsigned int iPhong = 0xFFFFFFFF;
        EDataType ePhong = EDT_Char;

        unsigned int iOpacity = 0xFFFFFFFF;
        EDataType eOpacity = EDT_Char;

        // search in the DOM for a vertex entry
        unsigned int _i = 0;
        for (std::vector<Element>::const_iterator i = this->pcDOM->alElements.begin();
                i != this->pcDOM->alElements.end(); ++i, ++_i) {
            if (EEST_Material == i->eSemantic) {
                pcList = &this->pcDOM->alElementData[_i];

                // now check whether which coordinate sets are available
                unsigned int _a = 0;
                for (std::vector<Property>::const_iterator a = i->alProperties.begin(); a != i->alProperties.end(); ++a, ++_a) {
                    if (a->bIsList) {
                        continue;
                    }

                    // pohng specularity      -----------------------------------
                    if (EST_PhongPower == (*a).Semantic) {
                        iPhong = _a;
                        ePhong = (*a).eType;
                    }

                    // general opacity        -----------------------------------
                    if (EST_Opacity == (*a).Semantic) {
                        iOpacity = _a;
                        eOpacity = (*a).eType;
                    }

                    // diffuse color channels -----------------------------------
                    if (EST_DiffuseRed == (*a).Semantic) {
                        aaiPositions[0][0] = _a;
                        aaiTypes[0][0] = (*a).eType;
                    } else if (EST_DiffuseGreen == (*a).Semantic) {
                        aaiPositions[0][1] = _a;
                        aaiTypes[0][1] = (*a).eType;
                    } else if (EST_DiffuseBlue == (*a).Semantic) {
                        aaiPositions[0][2] = _a;
                        aaiTypes[0][2] = (*a).eType;
                    } else if (EST_DiffuseAlpha == (*a).Semantic) {
                        aaiPositions[0][3] = _a;
                        aaiTypes[0][3] = (*a).eType;
                    }
                    // specular color channels -----------------------------------
                    else if (EST_SpecularRed == (*a).Semantic) {
                        aaiPositions[1][0] = _a;
                        aaiTypes[1][0] = (*a).eType;
                    } else if (EST_SpecularGreen == (*a).Semantic) {
                        aaiPositions[1][1] = _a;
                        aaiTypes[1][1] = (*a).eType;
                    } else if (EST_SpecularBlue == (*a).Semantic) {
                        aaiPositions[1][2] = _a;
                        aaiTypes[1][2] = (*a).eType;
                    } else if (EST_SpecularAlpha == (*a).Semantic) {
                        aaiPositions[1][3] = _a;
                        aaiTypes[1][3] = (*a).eType;
                    }
                    // ambient color channels -----------------------------------
                    else if (EST_AmbientRed == (*a).Semantic) {
                        aaiPositions[2][0] = _a;
                        aaiTypes[2][0] = (*a).eType;
                    } else if (EST_AmbientGreen == (*a).Semantic) {
                        aaiPositions[2][1] = _a;
                        aaiTypes[2][1] = (*a).eType;
                    } else if (EST_AmbientBlue == (*a).Semantic) {
                        aaiPositions[2][2] = _a;
                        aaiTypes[2][2] = (*a).eType;
                    } else if (EST_AmbientAlpha == (*a).Semantic) {
                        aaiPositions[2][3] = _a;
                        aaiTypes[2][3] = (*a).eType;
                    }
                }
                break;
            } else if (EEST_TextureFile == i->eSemantic) {
                defaultTexture = i->szName;
            }
        }
        // check whether we have a valid source for the material data
        if (nullptr != pcList) {
            for (std::vector<ElementInstance>::const_iterator i = pcList->alInstances.begin(); i != pcList->alInstances.end(); ++i) {
                aiColor4D clrOut;
                aiMaterial *pcHelper = new aiMaterial();

                // build the diffuse material color
                GetMaterialColor((*i).alProperties, aaiPositions[0], aaiTypes[0], &clrOut);
                pcHelper->AddProperty<aiColor4D>(&clrOut, 1, AI_MATKEY_COLOR_DIFFUSE);

                // build the specular material color
                GetMaterialColor((*i).alProperties, aaiPositions[1], aaiTypes[1], &clrOut);
                pcHelper->AddProperty<aiColor4D>(&clrOut, 1, AI_MATKEY_COLOR_SPECULAR);

                // build the ambient material color
                GetMaterialColor((*i).alProperties, aaiPositions[2], aaiTypes[2], &clrOut);
                pcHelper->AddProperty<aiColor4D>(&clrOut, 1, AI_MATKEY_COLOR_AMBIENT);

                // handle phong power and shading mode
                int iMode = aiShadingMode_Gouraud;
                if (0xFFFFFFFF != iPhong) {
                    ai_real fSpec = PropertyInstance::ConvertTo<ai_real>(GetProperty((*i).alProperties, iPhong).avList.front(), ePhong);

                    // if shininess is 0 (and the pow() calculation would therefore always
                    // become 1, not depending on the angle), use gouraud lighting
                    if (fSpec) {
                        // scale this with 15 ... hopefully this is correct
                        fSpec *= 15;
                        pcHelper->AddProperty<ai_real>(&fSpec, 1, AI_MATKEY_SHININESS);

                        iMode = static_cast<int>(aiShadingMode_Phong);
                    }
                }
                pcHelper->AddProperty<int>(&iMode, 1, AI_MATKEY_SHADING_MODEL);

                // handle opacity
                if (0xFFFFFFFF != iOpacity) {
                    ai_real fOpacity = PropertyInstance::ConvertTo<ai_real>(GetProperty((*i).alProperties, iPhong).avList.front(), eOpacity);
                    pcHelper->AddProperty<ai_real>(&fOpacity, 1, AI_MATKEY_OPACITY);
                }

                // The face order is absolutely undefined for PLY, so we have to
                // use two-sided rendering to be sure it's ok.
                const int two_sided = 1;
                pcHelper->AddProperty(&two_sided, 1, AI_MATKEY_TWOSIDED);

                // default texture
                if (!defaultTexture.empty()) {
                    const aiString name(defaultTexture.c_str());
                    pcHelper->AddProperty(&name, _AI_MATKEY_TEXTURE_BASE, aiTextureType_DIFFUSE, 0);
                }

                if (!pointsOnly) {
                    pcHelper->AddProperty(&two_sided, 1, AI_MATKEY_TWOSIDED);
                }

                // set to wireframe, so when using this material info we can switch to points rendering
                if (pointsOnly) {
                    constexpr int wireframe = 1;
                    pcHelper->AddProperty(&wireframe, 1, AI_MATKEY_ENABLE_WIREFRAME);
                }

                // add the newly created material instance to the list
                pvOut->push_back(pcHelper);
            }
        } else {
            // generate a default material
            aiMaterial *pcHelper = new aiMaterial();

            // fill in a default material
            int iMode = aiShadingMode_Gouraud;
            pcHelper->AddProperty<int>(&iMode, 1, AI_MATKEY_SHADING_MODEL);

            // generate white material most 3D engine just multiply ambient / diffuse color with actual ambient / light color
            aiColor3D clr;
            clr.b = clr.g = clr.r = 1.0f;
            pcHelper->AddProperty<aiColor3D>(&clr, 1, AI_MATKEY_COLOR_DIFFUSE);
            pcHelper->AddProperty<aiColor3D>(&clr, 1, AI_MATKEY_COLOR_SPECULAR);

            clr.b = clr.g = clr.r = 1.0f;
            pcHelper->AddProperty<aiColor3D>(&clr, 1, AI_MATKEY_COLOR_AMBIENT);

            // The face order is absolutely undefined for PLY, so we have to
            // use two-sided rendering to be sure it's ok.
            if (!pointsOnly) {
                const int two_sided = 1;
                pcHelper->AddProperty(&two_sided, 1, AI_MATKEY_TWOSIDED);
            }

            // default texture
            if (!defaultTexture.empty()) {
                const aiString name(defaultTexture.c_str());
                pcHelper->AddProperty(&name, _AI_MATKEY_TEXTURE_BASE, aiTextureType_DIFFUSE, 0);
            }

            // set to wireframe, so when using this material info we can switch to points rendering
            if (pointsOnly) {
                constexpr int wireframe = 1;
                pcHelper->AddProperty(&wireframe, 1, AI_MATKEY_ENABLE_WIREFRAME);
            }

            pvOut->push_back(pcHelper);
        }
    }

} // namespace Assimp

#endif // !! ASSIMP_BUILD_NO_PLY_IMPORTER
