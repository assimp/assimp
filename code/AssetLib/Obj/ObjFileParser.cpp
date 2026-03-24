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
#ifndef ASSIMP_BUILD_NO_OBJ_IMPORTER

#include "ObjFileParser.h"
#include "ObjFileData.h"
#include "ObjFileMtlImporter.h"
#include "ObjTools.h"
#include <assimp/BaseImporter.h>
#include <assimp/DefaultIOSystem.h>
#include <assimp/ParsingUtils.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <cstdlib>
#include <memory>
#include <utility>

namespace Assimp {

constexpr const char ObjFileParser::DEFAULT_MATERIAL[];

// -------------------------------------------------------------------
static bool isDataDefinitionEnd(const char *tmp) {
	ai_assert(tmp != nullptr);
	
    if (*tmp == '\\') {
        ++tmp;
        if (IsLineEnd(*tmp)) {
            return true;
        }
    }
    return false;
}

// -------------------------------------------------------------------
static bool isNanOrInf(const char *in) {
	ai_assert(in != nullptr);
	
    // Look for "nan" or "inf", case insensitive
    return ((in[0] == 'N' || in[0] == 'n') && ASSIMP_strincmp(in, "nan", 3) == 0) ||
           ((in[0] == 'I' || in[0] == 'i') && ASSIMP_strincmp(in, "inf", 3) == 0);
}

// -------------------------------------------------------------------
ObjFileParser::ObjFileParser() :
        mDataIt(),
        mDataItEnd(),
        mBuffer(),
        mEnd(&mBuffer[Buffersize-1]+1) {
    std::fill_n(mBuffer, Buffersize, '\0');
}

// -------------------------------------------------------------------
ObjFileParser::ObjFileParser(IOStreamBuffer<char> &streamBuffer, const std::string &modelName,
        IOSystem *io, ProgressHandler *progress, const std::string &originalObjFileName) :
            mDataIt(),
            mDataItEnd(),
            mBuffer(),
            mIO(io),
            mProgress(progress),
            mOriginalObjFileName(originalObjFileName) { 
    std::fill_n(mBuffer, Buffersize, '\0');

    // Create the model instance to store all the data
    mModel.reset(new ObjFile::Model());
    mModel->mModelName = modelName;

    // create default material and store it
    mModel->mDefaultMaterial = new ObjFile::Material;
    mModel->mDefaultMaterial->MaterialName.Set(DEFAULT_MATERIAL);
    mModel->mMaterialLib.emplace_back(DEFAULT_MATERIAL);
    mModel->mMaterialMap[DEFAULT_MATERIAL] = mModel->mDefaultMaterial;

    // Start parsing the file
    parseFile(streamBuffer);
}

void ObjFileParser::setBuffer(std::vector<char> &buffer) {
    mDataIt = buffer.begin();
    mDataItEnd = buffer.end();
    ai_assert(mDataIt < mDataItEnd);
	if (!buffer.empty()) {
    	mEnd = &buffer[buffer.size() - 1] + 1;
	}
}

// -------------------------------------------------------------------
ObjFile::Model *ObjFileParser::GetModel() const {
    return mModel.get();
}

// -------------------------------------------------------------------
void ObjFileParser::parseFile(IOStreamBuffer<char> &streamBuffer) {
    // only update every 100KB or it'll be too slow
    // const unsigned int updateProgressEveryBytes = 100 * 1024;
    const unsigned int bytesToProcess = static_cast<unsigned int>(streamBuffer.size());
    const unsigned int progressTotal = bytesToProcess;
    unsigned int processed = 0u;
    size_t lastFilePos = 0u;

    bool insideCstype = false;
    std::vector<char> buffer;
    while (streamBuffer.getNextDataLine(buffer, '\\')) {
        mDataIt = buffer.begin();
        mDataItEnd = buffer.end();
        mEnd = &buffer[buffer.size() - 1] + 1;

        if (processed == 0 && std::distance(mDataIt, mDataItEnd) >= 3 &&
            	static_cast<unsigned char>(*mDataIt) == 0xEF &&
            	static_cast<unsigned char>(*(mDataIt + 1)) == 0xBB &&
            	static_cast<unsigned char>(*(mDataIt + 2)) == 0xBF) {
            mDataIt += 3; // skip BOM
        }

        // Handle progress reporting
        const size_t filePos = streamBuffer.getFilePos();
        if (lastFilePos < filePos) {
            processed = static_cast<unsigned int>(filePos);
            lastFilePos = filePos;
			if (mProgress != nullptr) {
				mProgress->UpdateFileRead(processed, progressTotal);
			}
        }

        // handle c-stype section end (http://paulbourke.net/dataformats/obj/)
        if (insideCstype) {
            switch (*mDataIt) {
            case 'e': {
                std::string name;
                getNameNoSpace(mDataIt, mDataItEnd, name);
                insideCstype = name != "end";
            } break;
            }
            goto pf_skip_line;
        }

        // parse line
        switch (*mDataIt) {
        case 'v': // Parse a vertex texture coordinate
        {
            ++mDataIt;
            if (*mDataIt == ' ' || *mDataIt == '\t') {
                size_t numComponents = getNumComponentsInDataDefinition();
                if (numComponents == 3) {
                    // read in vertex definition
                    getVector3(mModel->mVertices);
                } else if (numComponents == 4) {
                    // read in vertex definition (homogeneous coords)
                    getHomogeneousVector3(mModel->mVertices);
                } else if (numComponents == 6) {
                    // fill previous omitted vertex-colors by default
                    if (mModel->mVertexColors.size() < mModel->mVertices.size()) {
                        mModel->mVertexColors.resize(mModel->mVertices.size(), aiVector3D(0, 0, 0));
                    }
                    // read vertex and vertex-color
                    getTwoVectors3(mModel->mVertices, mModel->mVertexColors);
                }
                // append omitted vertex-colors as default for the end if any vertex-color exists
                if (!mModel->mVertexColors.empty() && mModel->mVertexColors.size() < mModel->mVertices.size()) {
                    mModel->mVertexColors.resize(mModel->mVertices.size(), aiVector3D(0, 0, 0));
                }
            } else if (*mDataIt == 't') {
                // read in texture coordinate ( 2D or 3D )
                ++mDataIt;
                size_t dim = getTexCoordVector(mModel->mTextureCoord);
                mModel->mTextureCoordDim = std::max(mModel->mTextureCoordDim, (unsigned int)dim);
            } else if (*mDataIt == 'n') {
                // Read in normal vector definition
                ++mDataIt;
                getVector3(mModel->mNormals);
            }
        } break;

        case 'p': // Parse a face, line or point statement
        case 'l':
        case 'f': {
            getFace(*mDataIt == 'f' ? aiPrimitiveType_POLYGON : (*mDataIt == 'l' ? aiPrimitiveType_LINE : aiPrimitiveType_POINT));
        } break;

        case '#': // Parse a comment
        {
            skipComment();
        } break;

        case 'u': // Parse a material desc. setter
        {
            std::string name;
            getNameNoSpace(mDataIt, mDataItEnd, name);

            size_t nextSpace = name.find(' ');
            if (nextSpace != std::string::npos)
                name = name.substr(0, nextSpace);

            if (name == "usemtl") {
                getMaterialDesc();
            }
        } break;

        case 'm': // Parse a material library or merging group ('mg')
        {
            std::string name;

            getNameNoSpace(mDataIt, mDataItEnd, name);

            size_t nextSpace = name.find(' ');
            if (nextSpace != std::string::npos)
                name = name.substr(0, nextSpace);

            if (name == "mg")
                skipGroupNumberAndResolution();
            else if (name == "mtllib")
                getMaterialLib();
            else
                goto pf_skip_line;
        } break;

        case 'g': // Parse group name
        {
            getGroupName();
        } break;

        case 's': // Parse group number
        {
            skipGroupNumber();
        } break;

        case 'o': // Parse object name
        {
            getObjectName();
        } break;

        case 'c': // handle cstype section start
        {
            std::string name;
            getNameNoSpace(mDataIt, mDataItEnd, name);
            insideCstype = name == "cstype";
            goto pf_skip_line;
        }

        default: {
        pf_skip_line:
            mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
        } break;
        }
    }
}

// -------------------------------------------------------------------
void ObjFileParser::copyNextWord(char *pBuffer, size_t length) {
    size_t index = 0;
    mDataIt = getNextWord<DataArrayIt>(mDataIt, mDataItEnd);
    if (*mDataIt == '\\') {
        ++mDataIt;
        ++mDataIt;
        mDataIt = getNextWord<DataArrayIt>(mDataIt, mDataItEnd);
    }
    while (mDataIt != mDataItEnd && !IsSpaceOrNewLine(*mDataIt)) {
        pBuffer[index] = *mDataIt;
        index++;
        if (index == length - 1) {
            break;
        }
        ++mDataIt;
    }

    ai_assert(index < length);
    pBuffer[index] = '\0';
}

// -------------------------------------------------------------------
size_t ObjFileParser::getNumComponentsInDataDefinition() {
    size_t numComponents(0);
    const char *tmp = &mDataIt[0];
    bool end_of_definition = false;
    while (!end_of_definition) {
        if (isDataDefinitionEnd(tmp)) {
            tmp += 2;
        } else if (IsLineEnd(*tmp)) {
            end_of_definition = true;
        }
        if (!SkipSpaces(&tmp, mEnd) || *tmp == '#') {
            break;
        }
        const bool isNum(IsNumeric(*tmp) || isNanOrInf(tmp));
        SkipToken(tmp, mEnd);
        if (isNum) {
            ++numComponents;
        }
        if (!SkipSpaces(&tmp, mEnd) || *tmp == '#') {
            break;
        }
    }

    return numComponents;
}

// -------------------------------------------------------------------
size_t ObjFileParser::getTexCoordVector(std::vector<aiVector3D> &point3d_array) {
    size_t numComponents = getNumComponentsInDataDefinition();
    ai_real x, y, z;
    if (2 == numComponents) {
        copyNextWord(mBuffer, Buffersize);
        x = (ai_real)fast_atof(mBuffer);

        copyNextWord(mBuffer, Buffersize);
        y = (ai_real)fast_atof(mBuffer);
        z = 0.0;
    } else if (3 == numComponents) {
        copyNextWord(mBuffer, Buffersize);
        x = (ai_real) fast_atof(mBuffer);

        copyNextWord(mBuffer, Buffersize);
        y = (ai_real)fast_atof(mBuffer);

        copyNextWord(mBuffer, Buffersize);
        z = (ai_real)fast_atof(mBuffer);
    } else {
        throw DeadlyImportError("OBJ: Invalid number of components");
    }

    // Coerce nan and inf to 0 as is the OBJ default value
    if (!std::isfinite(x))
        x = 0;

    if (!std::isfinite(y))
        y = 0;

    if (!std::isfinite(z))
        z = 0;

    point3d_array.emplace_back(x, y, z);
    mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
    return numComponents;
}

// -------------------------------------------------------------------
void ObjFileParser::getVector3(std::vector<aiVector3D> &point3d_array) {
    ai_real x, y, z;
    copyNextWord(mBuffer, Buffersize);
    x = (ai_real)fast_atof(mBuffer);

    copyNextWord(mBuffer, Buffersize);
    y = (ai_real)fast_atof(mBuffer);

    copyNextWord(mBuffer, Buffersize);
    z = (ai_real)fast_atof(mBuffer);

    point3d_array.emplace_back(x, y, z);
    mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
}

// -------------------------------------------------------------------
void ObjFileParser::getHomogeneousVector3(std::vector<aiVector3D> &point3d_array) {
    ai_real x, y, z, w;
    copyNextWord(mBuffer, Buffersize);
    x = (ai_real)fast_atof(mBuffer);

    copyNextWord(mBuffer, Buffersize);
    y = (ai_real)fast_atof(mBuffer);

    copyNextWord(mBuffer, Buffersize);
    z = (ai_real)fast_atof(mBuffer);

    copyNextWord(mBuffer, Buffersize);
    w = (ai_real)fast_atof(mBuffer);

    if (w == 0)
        throw DeadlyImportError("OBJ: Invalid component in homogeneous vector (Division by zero)");

    point3d_array.emplace_back(x / w, y / w, z / w);
    mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
}

// -------------------------------------------------------------------
void ObjFileParser::getTwoVectors3(std::vector<aiVector3D> &point3d_array_a, std::vector<aiVector3D> &point3d_array_b) {
    ai_real x, y, z;
    copyNextWord(mBuffer, Buffersize);
    x = (ai_real)fast_atof(mBuffer);

    copyNextWord(mBuffer, Buffersize);
    y = (ai_real)fast_atof(mBuffer);

    copyNextWord(mBuffer, Buffersize);
    z = (ai_real)fast_atof(mBuffer);

    point3d_array_a.emplace_back(x, y, z);

    copyNextWord(mBuffer, Buffersize);
    x = (ai_real)fast_atof(mBuffer);

    copyNextWord(mBuffer, Buffersize);
    y = (ai_real)fast_atof(mBuffer);

    copyNextWord(mBuffer, Buffersize);
    z = (ai_real)fast_atof(mBuffer);

    point3d_array_b.emplace_back(x, y, z);

    mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
}

// -------------------------------------------------------------------
void ObjFileParser::getVector2(std::vector<aiVector2D> &point2d_array) {
    ai_real x, y;
    copyNextWord(mBuffer, Buffersize);
    x = (ai_real)fast_atof(mBuffer);

    copyNextWord(mBuffer, Buffersize);
    y = (ai_real)fast_atof(mBuffer);

    point2d_array.emplace_back(x, y);

    mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
}

static constexpr char DefaultObjName[] = "defaultobject";

// -------------------------------------------------------------------
void ObjFileParser::getFace(aiPrimitiveType type) {
    mDataIt = getNextToken<DataArrayIt>(mDataIt, mDataItEnd);
    if (mDataIt == mDataItEnd || *mDataIt == '\0') {
        return;
    }

    ObjFile::Face *face = new ObjFile::Face(type);
    bool hasNormal = false;

    const int vSize = static_cast<unsigned int>(mModel->mVertices.size());
    const int vtSize = static_cast<unsigned int>(mModel->mTextureCoord.size());
    const int vnSize = static_cast<unsigned int>(mModel->mNormals.size());

    const bool vt = (!mModel->mTextureCoord.empty());
    const bool vn = (!mModel->mNormals.empty());
    int iPos = 0;
    while (mDataIt < mDataItEnd) {
        int iStep = 1;

        if (IsLineEnd(*mDataIt) || *mDataIt == '#') {
            break;
        }

        if (*mDataIt == '/') {
            if (type == aiPrimitiveType_POINT) {
                ASSIMP_LOG_ERROR("Obj: Separator unexpected in point statement");
            }
            ++iPos;
        } else if (IsSpaceOrNewLine(*mDataIt) || *mDataIt == '\v') {
            iPos = 0;
        } else {
            //OBJ USES 1 Base ARRAYS!!!!
            const int iVal = ::atoi(&(*mDataIt));

            // increment iStep position based off of the sign and # of digits
            int tmp = iVal;
            if (iVal < 0) {
                ++iStep;
            }
            while ((tmp = tmp / 10) != 0) {
                ++iStep;
            }

            if (iPos == 1 && !vt && vn) {
                iPos = 2; // skip texture coords for normals if there are no tex coords
            }

            if (iVal > 0) {
                // Store parsed index
                if (0 == iPos) {
                    face->m_vertices.push_back(iVal - 1);
                } else if (1 == iPos) {
                    face->m_texturCoords.push_back(iVal - 1);
                } else if (2 == iPos) {
                    face->m_normals.push_back(iVal - 1);
                    hasNormal = true;
                } else {
                    reportErrorTokenInFace();
                }
            } else if (iVal < 0) {
                // Store relatively index
                if (0 == iPos) {
                    face->m_vertices.push_back(vSize + iVal);
                } else if (1 == iPos) {
                    face->m_texturCoords.push_back(vtSize + iVal);
                } else if (2 == iPos) {
                    face->m_normals.push_back(vnSize + iVal);
                    hasNormal = true;
                } else {
                    reportErrorTokenInFace();
                }
            } else {
                //On error, std::atoi will return 0 which is not a valid value
                delete face;
                throw DeadlyImportError("OBJ: Invalid face index.");
            }
        }
        mDataIt += iStep;
    }

    if (face->m_vertices.empty()) {
        ASSIMP_LOG_ERROR("Obj: Ignoring empty face");
        // skip line and clean up
        mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
        delete face;
        return;
    }

    // Set active material, if one set
    if (nullptr != mModel->mCurrentMaterial) {
        face->m_pMaterial = mModel->mCurrentMaterial;
    } else {
        face->m_pMaterial = mModel->mDefaultMaterial;
    }

    // Create a default object, if nothing is there
    if (nullptr == mModel->mCurrentObject) {
        createObject(DefaultObjName);
    }

    // Assign face to mesh
    if (nullptr == mModel->mCurrentMesh) {
        createMesh(DefaultObjName);
    }

    // Store the face
    mModel->mCurrentMesh->m_Faces.emplace_back(face);
    mModel->mCurrentMesh->m_uiNumIndices += static_cast<unsigned int>(face->m_vertices.size());
    mModel->mCurrentMesh->m_uiUVCoordinates[0] += static_cast<unsigned int>(face->m_texturCoords.size());
    if (!mModel->mCurrentMesh->m_hasNormals && hasNormal) {
        mModel->mCurrentMesh->m_hasNormals = true;
    }
    // Skip the rest of the line
    mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
}

// -------------------------------------------------------------------
void ObjFileParser::getMaterialDesc() {
    // Get next data for material data
    mDataIt = getNextToken<DataArrayIt>(mDataIt, mDataItEnd);
    if (mDataIt == mDataItEnd) {
        return;
    }

    char *pStart = &(*mDataIt);
    while (mDataIt != mDataItEnd && !IsLineEnd(*mDataIt)) {
        ++mDataIt;
    }

    // In some cases we should ignore this 'usemtl' command, this variable helps us to do so
    bool skip = false;

    // Get name
    std::string strName(pStart, &(*mDataIt));
    strName = ai_trim(strName);
    if (strName.empty()) {
        skip = true;
    }

    // If the current mesh has the same material, we will ignore that 'usemtl' command
    // There is no need to create another object or even mesh here
    if (!skip) {
        if (mModel->mCurrentMaterial && mModel->mCurrentMaterial->MaterialName == aiString(strName)) {
            skip = true;
        }
    }

    if (!skip) {
        // Search for material
        std::map<std::string, ObjFile::Material *>::iterator it = mModel->mMaterialMap.find(strName);
        if (it == mModel->mMaterialMap.end()) {
            // Not found, so we don't know anything about the material except for its name.
            // This may be the case if the material library is missing. We don't want to lose all
            // materials if that happens, so create a new named material instead of discarding it
            // completely.
            ASSIMP_LOG_ERROR("OBJ: failed to locate material ", strName, ", creating new material");
            mModel->mCurrentMaterial = new ObjFile::Material();
            mModel->mCurrentMaterial->MaterialName.Set(strName);
            mModel->mMaterialLib.push_back(strName);
            mModel->mMaterialMap[strName] = mModel->mCurrentMaterial;
        } else {
            // Found, using detected material
            mModel->mCurrentMaterial = it->second;
        }

        if (needsNewMesh(strName)) {
            auto newMeshName = mModel->mActiveGroup.empty() ? strName : mModel->mActiveGroup;
            createMesh(newMeshName);
        }

        mModel->mCurrentMesh->m_uiMaterialIndex = getMaterialIndex(strName);
    }

    // Skip rest of line
    mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
}

// -------------------------------------------------------------------
//  Get a comment, values will be skipped
void ObjFileParser::skipComment() {
    mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
}

// -------------------------------------------------------------------
void ObjFileParser::getMaterialLib() {
    // Translate tuple
    mDataIt = getNextToken<DataArrayIt>(mDataIt, mDataItEnd);
    if (mDataIt == mDataItEnd) {
        return;
    }

    char *pStart = &(*mDataIt);
    while (mDataIt != mDataItEnd && !IsLineEnd(*mDataIt)) {
        ++mDataIt;
    }

    // Check for existence
    const std::string strMatName(pStart, &(*mDataIt));
    std::string absName;

    // Check if directive is valid.
    if (0 == strMatName.length()) {
        ASSIMP_LOG_WARN("OBJ: no name for material library specified.");
        return;
    }

    if (mIO->StackSize() > 0) {
        std::string path = mIO->CurrentDirectory();
        if ('/' != *path.rbegin()) {
            path += '/';
        }
        absName += path;
        absName += strMatName;
    } else {
        absName = strMatName;
    }
	
	std::unique_ptr<IOStream> pFile(mIO->Open(absName));
    if (nullptr == pFile) {
        ASSIMP_LOG_ERROR("OBJ: Unable to locate material file ", strMatName);
        std::string strMatFallbackName = mOriginalObjFileName.substr(0, mOriginalObjFileName.length() - 3) + "mtl";
        ASSIMP_LOG_INFO("OBJ: Opening fallback material file ", strMatFallbackName);
        pFile.reset(mIO->Open(strMatFallbackName));
        if (!pFile) {
            ASSIMP_LOG_ERROR("OBJ: Unable to locate fallback material file ", strMatFallbackName);
            mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
            return;
        }
    }

    // Import material library data from file.
    // Some exporters (e.g. Silo) will happily write out empty
    // material files if the model doesn't use any materials, so we
    // allow that.
    std::vector<char> buffer;
    BaseImporter::TextFileToBuffer(pFile.get(), buffer, BaseImporter::ALLOW_EMPTY);
    //m_pIO->Close(pFile);

    // Importing the material library
    ObjFileMtlImporter mtlImporter(buffer, strMatName, mModel.get());
}

// -------------------------------------------------------------------
void ObjFileParser::getNewMaterial() {
    mDataIt = getNextToken<DataArrayIt>(mDataIt, mDataItEnd);
    mDataIt = getNextWord<DataArrayIt>(mDataIt, mDataItEnd);
    if (mDataIt == mDataItEnd) {
        return;
    }

    char *pStart = &(*mDataIt);
    std::string strMat(pStart, *mDataIt);
    while (mDataIt != mDataItEnd && IsSpaceOrNewLine(*mDataIt)) {
        ++mDataIt;
    }
    auto it = mModel->mMaterialMap.find(strMat);
    if (it == mModel->mMaterialMap.end()) {
        // Show a warning, if material was not found
        ASSIMP_LOG_WARN("OBJ: Unsupported material requested: ", strMat);
        mModel->mCurrentMaterial = mModel->mDefaultMaterial;
    } else {
        // Set new material
        if (needsNewMesh(strMat)) {
            createMesh(strMat);
        }
        mModel->mCurrentMesh->m_uiMaterialIndex = getMaterialIndex(strMat);
    }

    mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
}

static constexpr int InvalidMaterialIndex = -1;

// -------------------------------------------------------------------
int ObjFileParser::getMaterialIndex(const std::string &strMaterialName) {
    int mat_index = InvalidMaterialIndex;
    if (strMaterialName.empty()) {
        return mat_index;
    }
    for (size_t index = 0; index < mModel->mMaterialLib.size(); ++index) {
        if (strMaterialName == mModel->mMaterialLib[index]) {
            mat_index = (int)index;
            break;
        }
    }
    return mat_index;
}

// -------------------------------------------------------------------
//  Getter for a group name.
void ObjFileParser::getGroupName() {
    std::string groupName;

    // here we skip 'g ' from line
    mDataIt = getNextToken<DataArrayIt>(mDataIt, mDataItEnd);
    mDataIt = getName<DataArrayIt>(mDataIt, mDataItEnd, groupName);
    if (isEndOfBuffer(mDataIt, mDataItEnd)) {
        return;
    }

    // Change active group, if necessary
    if (mModel->mActiveGroup != groupName) {
        // Search for already existing entry
        ObjFile::Model::ConstGroupMapIt it = mModel->mGroups.find(groupName);

        // We are mapping groups into the object structure
        createObject(groupName);

        // New group name, creating a new entry
        if (it == mModel->mGroups.end()) {
            std::vector<unsigned int> *pFaceIDArray = new std::vector<unsigned int>;
            mModel->mGroups[groupName] = pFaceIDArray;
            mModel->mGroupFaceIDs = (pFaceIDArray);
        } else {
            mModel->mGroupFaceIDs = (*it).second;
        }
        mModel->mActiveGroup = groupName;
    }
    mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
}

// -------------------------------------------------------------------
void ObjFileParser::skipGroupNumber() {
    // Not used

    mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
}

// -------------------------------------------------------------------
void ObjFileParser::skipGroupNumberAndResolution() {
    // Not used

    mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
}

// -------------------------------------------------------------------
//  Stores values for a new object instance, name will be used to
//  identify it.
void ObjFileParser::getObjectName() {
    mDataIt = getNextToken<DataArrayIt>(mDataIt, mDataItEnd);
    if (mDataIt == mDataItEnd) {
        return;
    }
    char *pStart = &(*mDataIt);
    while (mDataIt != mDataItEnd && !IsSpaceOrNewLine(*mDataIt)) {
        ++mDataIt;
    }

    std::string strObjectName(pStart, &(*mDataIt));
    if (!strObjectName.empty()) {
        // Reset current object
        mModel->mCurrentObject = nullptr;

        // Search for actual object
        for (auto it = mModel->mObjects.begin(); it != mModel->mObjects.end(); ++it) {
            if ((*it)->m_strObjName == strObjectName) {
                mModel->mCurrentObject = *it;
                break;
            }
        }

        // Allocate a new object, if current one was not found before
        if (mModel->mCurrentObject == nullptr) {
            createObject(strObjectName);
        }
    }
    mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
}

// -------------------------------------------------------------------
//  Creates a new object instance
void ObjFileParser::createObject(const std::string &objName) {
    ai_assert(nullptr != mModel);

    mModel->mCurrentObject = new ObjFile::Object;
    mModel->mCurrentObject->m_strObjName = objName;
    mModel->mObjects.push_back(mModel->mCurrentObject);

    createMesh(objName);

    if (mModel->mCurrentMaterial) {
        mModel->mCurrentMesh->m_uiMaterialIndex =
                getMaterialIndex(mModel->mCurrentMaterial->MaterialName.data);
        mModel->mCurrentMesh->m_pMaterial = mModel->mCurrentMaterial;
    }
}
// -------------------------------------------------------------------
//  Creates a new mesh
void ObjFileParser::createMesh(const std::string &meshName) {
    ai_assert(nullptr != mModel);

    mModel->mCurrentMesh = new ObjFile::Mesh(meshName);
    mModel->mMeshes.push_back(mModel->mCurrentMesh);
    auto meshId = static_cast<unsigned int>(mModel->mMeshes.size() - 1);
    if (mModel->mCurrentObject != nullptr) {
        mModel->mCurrentObject->m_Meshes.push_back(meshId);
    } else {
        ASSIMP_LOG_ERROR("OBJ: No object detected to attach a new mesh instance.");
    }
}

// -------------------------------------------------------------------
//  Returns true, if a new mesh must be created.
bool ObjFileParser::needsNewMesh(const std::string &materialName) {
    // If no mesh data yet
    if (mModel->mCurrentMesh == nullptr) {
        return true;
    }
    bool newMat = false;
    int matIdx = getMaterialIndex(materialName);
    int curMatIdx = mModel->mCurrentMesh->m_uiMaterialIndex;
    if (curMatIdx != int(ObjFile::Mesh::NoMaterial) && curMatIdx != matIdx
            // no need create a new mesh if no faces in current
            // lets say 'usemtl' goes straight after 'g'
            && !mModel->mCurrentMesh->m_Faces.empty()) {
        // New material -> only one material per mesh, so we need to create a new
        // material
        newMat = true;
    }
    return newMat;
}

// -------------------------------------------------------------------
//  Shows an error in parsing process.
void ObjFileParser::reportErrorTokenInFace() {
    mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
    ASSIMP_LOG_ERROR("OBJ: Not supported token in face description detected");
}

// -------------------------------------------------------------------

} // Namespace Assimp

#endif // !! ASSIMP_BUILD_NO_OBJ_IMPORTER
