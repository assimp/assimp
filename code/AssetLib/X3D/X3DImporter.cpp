/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team

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
/// \file   X3DImporter.cpp
/// \brief  X3D-format files importer for Assimp: main algorithm implementation.
/// \date   2015-2016
/// \author smal.root@gmail.com

#ifndef ASSIMP_BUILD_NO_X3D_IMPORTER

#include "X3DImporter.hpp"
#include <assimp/StringUtils.h>

// Header files, Assimp.
#include <assimp/DefaultIOSystem.h>
#include <assimp/fast_atof.h>

// Header files, stdlib.
#include <iterator>
#include <memory>
#include <string>

namespace Assimp {

/// Constant which holds the importer description
const aiImporterDesc X3DImporter::Description = {
    "Extensible 3D(X3D) Importer",
    "smalcom",
    "",
    "See documentation in source code. Chapter: Limitations.",
    aiImporterFlags_SupportTextFlavour | aiImporterFlags_SupportBinaryFlavour | aiImporterFlags_LimitedSupport | aiImporterFlags_Experimental,
    0,
    0,
    0,
    0,
    "x3d x3db"
};

struct WordIterator {
    using iterator_category = std::input_iterator_tag;
    using value_type = const char *;
    using difference_type = ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;

    static const char *whitespace;
    const char *mStart, *mEnd;

    WordIterator(const char *start, const char *end) :
            mStart(start),
            mEnd(end) {
        mStart = start + ::strspn(start, whitespace);
        if (mStart >= mEnd) {
            mStart = 0;
        }
    }
    WordIterator() :
            mStart(0),
            mEnd(0) {}
    WordIterator(const WordIterator &other) :
            mStart(other.mStart),
            mEnd(other.mEnd) {}
    WordIterator &operator=(const WordIterator &other) {
        mStart = other.mStart;
        mEnd = other.mEnd;
        return *this;
    }

    bool operator==(const WordIterator &other) const { return mStart == other.mStart; }

    bool operator!=(const WordIterator &other) const { return mStart != other.mStart; }

    WordIterator &operator++() {
        mStart += strcspn(mStart, whitespace);
        mStart += strspn(mStart, whitespace);
        if (mStart >= mEnd) {
            mStart = 0;
        }
        return *this;
    }
    WordIterator operator++(int) {
        WordIterator result(*this);
        ++(*this);
        return result;
    }
    const char *operator*() const { return mStart; }
};

const char *WordIterator::whitespace = ", \t\r\n";

X3DImporter::X3DImporter() :
        mNodeElementCur(nullptr),
        mXmlParser(nullptr) {
    // empty
}

X3DImporter::~X3DImporter() {
    // Clear() is accounting if data already is deleted. So, just check again if all data is deleted.
    Clear();
}

void X3DImporter::Clear() {
    mNodeElementCur = nullptr;
    // Delete all elements
    if (!NodeElement_List.empty()) {
        for (std::list<X3DNodeElementBase *>::iterator it = NodeElement_List.begin(); it != NodeElement_List.end(); ++it) {
            delete *it;
        }
        NodeElement_List.clear();
    }
}

void X3DImporter::ParseFile(const std::string &file, IOSystem *pIOHandler) {
    ai_assert(nullptr != pIOHandler);

    static const std::string mode = "rb";
    std::unique_ptr<IOStream> fileStream(pIOHandler->Open(file, mode));
    if (!fileStream.get()) {
        throw DeadlyImportError("Failed to open file " + file + ".");
    }
}

/*********************************************************************************************************************************************/
/************************************************************ Functions: find set ************************************************************/
/*********************************************************************************************************************************************/

bool X3DImporter::CanRead(const std::string &pFile, IOSystem *pIOHandler, bool pCheckSig) const {
    const std::string extension = GetExtension(pFile);

    if ((extension == "x3d") || (extension == "x3db")) {
        return true;
    }

    if (!extension.length() || pCheckSig) {
        const char *tokens[] = { "DOCTYPE X3D PUBLIC", "http://www.web3d.org/specifications/x3d" };

        return SearchFileHeaderForToken(pIOHandler, pFile, tokens, 2);
    }

    return false;
}

void X3DImporter::GetExtensionList(std::set<std::string> &pExtensionList) {
    pExtensionList.insert("x3d");
    pExtensionList.insert("x3db");
}

const aiImporterDesc *X3DImporter::GetInfo() const {
    return &Description;
}

void X3DImporter::InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler) {
    mpIOHandler = pIOHandler;

    Clear(); // delete old graph.
    std::string::size_type slashPos = pFile.find_last_of("\\/");
    pIOHandler->PushDirectory(slashPos == std::string::npos ? std::string() : pFile.substr(0, slashPos + 1));
    ParseFile(pFile, pIOHandler);
    pIOHandler->PopDirectory();
    if (NodeElement_List.empty())
        return;
    //
    // Assimp use static arrays of objects for fast speed of rendering. That's good, but need some additional operations/
    // We know that geometry objects(meshes) are stored in <Shape>, also in <Shape>-><Appearance> materials(in Assimp logical view)
    // are stored. So at first we need to count how meshes and materials are stored in scene graph.
    //
    // at first creating root node for aiScene.
    pScene->mRootNode = new aiNode;
    pScene->mRootNode->mParent = nullptr;
    pScene->mFlags |= AI_SCENE_FLAGS_ALLOW_SHARED;
    //search for root node element
    mNodeElementCur = NodeElement_List.front();
    while (mNodeElementCur->Parent != nullptr)
        mNodeElementCur = mNodeElementCur->Parent;

    { // fill aiScene with objects.
        std::list<aiMesh *> mesh_list;
        std::list<aiMaterial *> mat_list;
        std::list<aiLight *> light_list;

        // create nodes tree
//        Postprocess_BuildNode(*mNodeElementCur, *pScene->mRootNode, mesh_list, mat_list, light_list);
        // copy needed data to scene
        if (!mesh_list.empty()) {
            std::list<aiMesh *>::const_iterator it = mesh_list.begin();

            pScene->mNumMeshes = static_cast<unsigned int>(mesh_list.size());
            pScene->mMeshes = new aiMesh *[pScene->mNumMeshes];
            for (size_t i = 0; i < pScene->mNumMeshes; i++)
                pScene->mMeshes[i] = *it++;
        }

        if (!mat_list.empty()) {
            std::list<aiMaterial *>::const_iterator it = mat_list.begin();

            pScene->mNumMaterials = static_cast<unsigned int>(mat_list.size());
            pScene->mMaterials = new aiMaterial *[pScene->mNumMaterials];
            for (size_t i = 0; i < pScene->mNumMaterials; i++)
                pScene->mMaterials[i] = *it++;
        }

        if (!light_list.empty()) {
            std::list<aiLight *>::const_iterator it = light_list.begin();

            pScene->mNumLights = static_cast<unsigned int>(light_list.size());
            pScene->mLights = new aiLight *[pScene->mNumLights];
            for (size_t i = 0; i < pScene->mNumLights; i++)
                pScene->mLights[i] = *it++;
        }
    }
}

} // namespace Assimp

#endif // !ASSIMP_BUILD_NO_X3D_IMPORTER
