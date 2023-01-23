/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2023, assimp team


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
#ifndef ASSIMP_BUILD_NO_S3O_IMPORTER

#include "S3OFileParser.h"
#include "S3OHelper.hpp"

#include <assimp/DefaultLogger.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/IOStreamBuffer.h>

#include <memory>
#include <string>

namespace Assimp {

// -----------------------------------------------------------------------------------
template <>
aiString S3OFileParser::Read<aiString>(std::size_t pOffset) {
    if (pOffset >= mBuffer.size()) {
        throw DeadlyImportError("S3O: file is either empty or corrupt at reading a string: ", mFile);
    }

    size_t result = 0;
    for (size_t i = pOffset; i < mBuffer.size(); i++) {
        if (mBuffer[i] == '\0') {
            result = i;
            break;
        }
    }

    auto cStr = std::string(mBuffer.begin()+pOffset, mBuffer.begin()+result);
    return aiString(cStr.c_str());
}

// -----------------------------------------------------------------------------------
template <>
S3ODataHeader *S3OFileParser::Read<S3ODataHeader *>(std::size_t pOffset) {
    // We should have a header in buffer
    if ((pOffset + sizeof(S3ODataHeader)) > mBuffer.size()) {
        throw DeadlyImportError("S3O: file is either empty or corrupt at reading file header: ", mFile);
    }

    // auto myData = std::vector<char>(mBuffer.first()+pOffset, mBuffer.begin()+pOffset+sizeof(S3ODataHeader));
    S3ODataHeader *header = reinterpret_cast<S3ODataHeader *>(&mBuffer[pOffset]);

	if (strcmp(header->magic, S3OToken)) {
		throw DeadlyImportError("S3O: header magic token is wrong ", mFile);
	}

	if (header->version != 0) {
		throw DeadlyImportError("S3O: header version is wrong ", mFile);
	}

// TODO(rene): I hate these #ifdefs's here.
#ifdef AI_BUILD_BIG_ENDIAN
    header->swap();
#endif 

    return header;
}

// -----------------------------------------------------------------------------------
template <>
S3ODataPiece *S3OFileParser::Read<S3ODataPiece *>(std::size_t pOffset) {
    // We should have a header in buffer
    if ((pOffset + sizeof(S3ODataPiece)) > mBuffer.size()) {
        throw DeadlyImportError("S3O: file is either empty or corrupt at reading piece header: ", mFile);
    }

    // auto myData = std::vector<char>(mBuffer.begin()+pOffset, mBuffer.begin()+pOffset+sizeof(S3ODataPiece));
    S3ODataPiece *header = reinterpret_cast<S3ODataPiece *>(&mBuffer[pOffset]);
#ifdef AI_BUILD_BIG_ENDIAN
    header->swap();
#endif

	if (header->numVertices >= AI_MAX_VERTICES) {
		throw DeadlyImportError("S3O: file is either empty or corrupt at reading piece header (to much Vertices): ", mFile);
	}

    return header;
}

S3OFileParser::S3OFileParser(const std::string& pFile, aiScene *pScene, IOSystem* pIOHandler) :
    mBuffer(),
    mFile(pFile),
    mIOHandler(pIOHandler),
    mScene(pScene),
    mMeshMap()
{};

S3OFileParser::~S3OFileParser() {
    mMeshMap.clear();
};

void S3OFileParser::Parse() {
    auto file = mIOHandler->Open(mFile, "rb");
    if (!file) {
        throw DeadlyImportError("S3O: Could not open ", mFile);
    }

    mBuffer = std::vector<char>(file->FileSize());
    size_t tmp = file->Read(&mBuffer.front(), sizeof(char), mBuffer.size());
    if (tmp != mBuffer.size()) {
        throw DeadlyImportError("S3O: Could not read ", mFile);
    }

    S3ODataHeader *header = Read<S3ODataHeader *>(0);

	LoadNode(header->rootPiece, nullptr);

    mScene->mNumMeshes = (unsigned int)mMeshMap.size();
    mScene->mMeshes = new aiMesh *[mScene->mNumMeshes]();
    for (unsigned int a = 0; a < mScene->mNumMeshes; ++a) {
        mScene->mMeshes[a] = mMeshMap[a];
    }

    mIOHandler->Close(file);
}

// ------------------------------------------------------------------------------------------------
// Load S3O Object
void S3OFileParser::LoadNode(size_t offset, aiNode *parent) {
    S3ODataPiece *ph = Read<S3ODataPiece *>(offset);

	aiNode *node = new aiNode();
	node->mName = Read<aiString>(ph->name);
	node->mParent = parent;

    if (ph->numVertices > 0) {
        auto s3oMesh = S3OMesh(ph);
        s3oMesh.Load(reinterpret_cast<S3ODataVertex *>(&mBuffer[ph->vertices]), reinterpret_cast<uint32_t *>(&mBuffer[ph->vertexTable]));
        s3oMesh.Trianglize();

	    aiMesh *mesh = new aiMesh();
        mesh->mName = node->mName;
        mesh->mPrimitiveTypes = aiPrimitiveType::aiPrimitiveType_TRIANGLE;

        mesh->mVertices = new aiVector3D[s3oMesh.mVertices.size() - 1];
        mesh->mNormals = new aiVector3D[s3oMesh.mVertices.size() - 1];
        mesh->mTextureCoords[0] = new aiVector3D[s3oMesh.mVertices.size() - 1];
        mesh->mTextureCoords[1] = new aiVector3D[s3oMesh.mVertices.size() - 1];

        for (mesh->mNumVertices = 0; mesh->mNumVertices < s3oMesh.mVertices.size(); ++mesh->mNumVertices) {
            mesh->mVertices[mesh->mNumVertices] = s3oMesh.mVertices[mesh->mNumVertices].mPos;
            mesh->mNormals[mesh->mNumVertices] = s3oMesh.mVertices[mesh->mNumVertices].mNormal;
            mesh->mTextureCoords[0][mesh->mNumVertices] = s3oMesh.mVertices[mesh->mNumVertices].mTc[0];
            mesh->mTextureCoords[1][mesh->mNumVertices] = s3oMesh.mVertices[mesh->mNumVertices].mTc[1];
        }

        mesh->mFaces = new aiFace[s3oMesh.mIndices.size() / 3];
        for (mesh->mNumFaces = 0; mesh->mNumFaces < s3oMesh.mIndices.size() / 3; ++mesh->mNumFaces) {
            auto face = aiFace();
            face.mIndices = new uint[3];

            auto first = mesh->mNumFaces * 3;
            for (face.mNumIndices = 0; face.mNumIndices < 3; ++face.mNumIndices) {
                face.mIndices[face.mNumIndices] = s3oMesh.mIndices[first+face.mNumIndices];
            }

            mesh->mFaces[mesh->mNumFaces] = face;
        }

        // Add Mesh
        node->mMeshes = new unsigned int[1]{(unsigned int)mMeshMap.size()};
        mMeshMap.push_back(mesh);
        node->mNumMeshes += 1;
    }

	node->mChildren = new aiNode *[ph->numChilds];

    // load children
	const uint32_t *childList = reinterpret_cast<uint32_t *>(&mBuffer[ph->childs]);
    for (uint32_t i = 0; i < ph->numChilds; i++) {
        uint32_t childOffset = *(childList++);
#ifdef AI_BUILD_BIG_ENDIAN
        ByteSwap::Swap4(&childOffset);
#endif

		LoadNode(childOffset, node);
    }

	if (parent != nullptr) {
		parent->mChildren[parent->mNumChildren] = node;
		parent->mNumChildren++;
	} else {
		mScene->mRootNode = node;
	}
}

} // namespace Assimp

#endif // ASSIMP_BUILD_NO_S3O_IMPORTER
#endif // ASSIMP_BUILD_NO_EXPORT
