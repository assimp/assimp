/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2015, assimp team
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
#ifndef ASSIMP_BUILD_NO_GLTF_EXPORTER

#include "glTFExporter.h"
#include "Exceptional.h"
#include "StringComparison.h"
#include "ByteSwapper.h"

#include <assimp/version.h>
#include <assimp/IOSystem.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/material.h>
#include <assimp/scene.h>

#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "glTFFileData.h"
#include "glTFUtil.h"

using namespace rapidjson;

using namespace Assimp;
using namespace Assimp::glTF;

namespace Assimp {

    // ------------------------------------------------------------------------------------------------
    // Worker function for exporting a scene to GLTF. Prototyped and registered in Exporter.cpp
    void ExportSceneGLTF(const char* pFile, IOSystem* pIOSystem, const aiScene* pScene, const ExportProperties* pProperties)
    {
        // invoke the exporter
        glTFExporter exporter(pFile, pIOSystem, pScene, pProperties, false);
    }

    // ------------------------------------------------------------------------------------------------
    // Worker function for exporting a scene to GLB. Prototyped and registered in Exporter.cpp
    void ExportSceneGLB(const char* pFile, IOSystem* pIOSystem, const aiScene* pScene, const ExportProperties* pProperties)
    {
        // invoke the exporter
        glTFExporter exporter(pFile, pIOSystem, pScene, pProperties, true);
    }

} // end of namespace Assimp



class glTFSceneExporter
{
    typedef std::gltf_unordered_map<std::string, int> IdMap;

    Document& mDoc;
    MemoryPoolAllocator<>& mAl;

    const aiScene* mScene;

    std::string mRootNodeId;

    std::vector<std::string> mMeshIds;

    IdMap mUsedIds;

public:
    glTFSceneExporter(Document& doc, const aiScene* pScene)
        : mDoc(doc)
        , mAl(doc.GetAllocator())
        , mScene(pScene)
    {
        doc.SetObject();

        for (unsigned int i = 0; i < pScene->mNumAnimations; ++i) {

        }

        for (unsigned int i = 0; i < pScene->mNumCameras; ++i) {

        }

        for (unsigned int i = 0; i < pScene->mNumLights; ++i) {

        }

        for (unsigned int i = 0; i < pScene->mNumLights; ++i) {

        }

        for (unsigned int i = 0; i < pScene->mNumMaterials; ++i) {

        }


        AddMeshes();

        for (unsigned int i = 0; i < pScene->mNumTextures; ++i) {

        }

        AddNodes();

        CreateScene();
    }

    inline void Pushf(Value& val, float f)
    {
        val.PushBack(Value(f).Move(), mAl);
    }

    inline void SetMatrix(Value& v, const aiMatrix4x4& m)
    {
        v.SetArray();
        v.Reserve(16, mAl);

        Pushf(v, m.a1); Pushf(v, m.b1); Pushf(v, m.c1); Pushf(v, m.d1);
        Pushf(v, m.a2); Pushf(v, m.b2); Pushf(v, m.c2); Pushf(v, m.d2);
        Pushf(v, m.a3); Pushf(v, m.b3); Pushf(v, m.c3); Pushf(v, m.d3);
        Pushf(v, m.a4); Pushf(v, m.b4); Pushf(v, m.c4); Pushf(v, m.d4);
    }

    void AddMeshes()
    {
        if (mScene->mNumMeshes == 0) return;

        Value meshes;
        meshes.SetObject();

        mMeshIds.reserve(mScene->mNumMeshes);
        for (unsigned int i = 0; i < mScene->mNumMeshes; ++i) {
            aiMesh* m = mScene->mMeshes[i];
            std::string meshId = FindID(m->mName, "mesh");
            mMeshIds.push_back(meshId);

            Value mesh;
            mesh.SetObject();
            {
                Value primitives;
                primitives.SetObject();

                
                mesh.AddMember("primitives", primitives, mAl);
            }

            meshes.AddMember(StringRef(mMeshIds.back()), mesh, mAl);
        }

        mDoc.AddMember("meshes", meshes, mAl);
    }

    void AddNodes()
    {
        if (!mScene->mRootNode) return;

        Value nodes;
        nodes.SetObject();

        mRootNodeId = AddNode(nodes, mScene->mRootNode);

        mDoc.AddMember("nodes", nodes, mAl);
    }


    std::string AddNode(Value& nodes, const aiNode* n)
    {
        std::string nodeId = FindID(n->mName, "node");

        Value node;
        node.SetObject();

        if (!n->mTransformation.IsIdentity()) {
            Value matrix;
            SetMatrix(matrix, n->mTransformation);
            node.AddMember("matrix", matrix, mAl);
        }

        if (n->mNumMeshes > 0) {
            Value meshes;
            meshes.SetArray();
            for (unsigned int i = 0; i < n->mNumMeshes; ++i) {
                meshes.PushBack(StringRef(mMeshIds[n->mMeshes[i]]), mAl);
            }
            node.AddMember("meshes", meshes, mAl);
        }

        if (n->mNumChildren > 0) {
            Value children;
            children.SetArray();
            for (unsigned int i = 0; i < n->mNumChildren; ++i) {
                std::string id = AddNode(nodes, n->mChildren[i]);
                children.PushBack(Value(id, mAl).Move(), mAl);
            }
            node.AddMember("children", children, mAl);
        }

        nodes.AddMember(Value(nodeId, mAl).Move(), node, mAl);

        return nodeId;
    }

    void CreateScene()
    {
        const char* sceneName = "defaultScene";

        mDoc.AddMember("scene", Value(sceneName, mAl).Move(), mAl);

        Value scenes;
        scenes.SetObject();
        {
            Value scene;
            scene.SetObject();
            {
                Value nodes;
                nodes.SetArray();

                if (!mRootNodeId.empty()) {
                    nodes.PushBack(StringRef(mRootNodeId), mAl);
                }

                scene.AddMember("nodes", nodes, mAl);
            }
            scenes.AddMember(Value(sceneName, mAl).Move(), scene, mAl);
        }
        mDoc.AddMember("scenes", scenes, mAl);
    }

    std::string FindID(const aiString& str, const char* suffix)
    {
        std::string id = str.C_Str();

        IdMap::iterator it;

        do {
            if (!id.empty()) {
                it = mUsedIds.find(id);
                if (it == mUsedIds.end()) break;

                id += "-";
            }

            id += suffix;

            it = mUsedIds.find(id);
            if (it == mUsedIds.end()) break;

            char buffer[256];
            int offset = sprintf(buffer, "%s-", id.c_str());
            for (int i = 0; it == mUsedIds.end(); ++i) {
                ASSIMP_itoa10(buffer + offset, sizeof(buffer), i);

                id = buffer;
                it = mUsedIds.find(id);
            }
        } while (false); // fake loop to allow using "break"

        mUsedIds[id] = true;
        return id;
    }
};



glTFExporter::glTFExporter(const char* filename, IOSystem* pIOSystem, const aiScene* pScene,
                           const ExportProperties* pProperties, bool isBinary)
    : mFilename(filename)
    , mIOSystem(pIOSystem)
    , mScene(pScene)
    , mProperties(pProperties)
    , mIsBinary(isBinary)
{
    boost::scoped_ptr<IOStream> outfile(pIOSystem->Open(mFilename, "wt"));
    if (outfile == 0) {
        throw DeadlyExportError("Could not open output file: " + std::string(mFilename));
    }

    if (isBinary) {
        // we will write the header later, skip its size
        outfile->Seek(sizeof(GLB_Header), aiOrigin_SET);
    }


    Document doc;
    StringBuffer docBuffer;
    {
        glTFSceneExporter exportScene(doc, mScene);

        bool pretty = true; 
        if (!isBinary && pretty) {
            PrettyWriter<StringBuffer> writer(docBuffer);
            doc.Accept(writer);
        }
        else {
            Writer<StringBuffer> writer(docBuffer);
            doc.Accept(writer);
        }
    }

    if (outfile->Write(docBuffer.GetString(), docBuffer.GetSize(), 1) != 1) {
        throw DeadlyExportError("Failed to write scene data!"); 
    }

    if (isBinary) {
        WriteBinaryData(outfile.get(), docBuffer.GetSize());
    }
}


void glTFExporter::WriteBinaryData(IOStream* outfile, std::size_t sceneLength)
{
    //
    // write the body data
    //

    if (!mBodyData.empty()) {
        std::size_t bodyOffset = sizeof(GLB_Header) + sceneLength;
        bodyOffset = (bodyOffset + 3) & ~3; // Round up to next multiple of 4

        outfile->Seek(bodyOffset, aiOrigin_SET);

        if (outfile->Write(&mBodyData[0], mBodyData.size(), 1) != 1) {
            throw DeadlyExportError("Failed to write body data!");
        }
    }


    //
    // write the header
    //

    outfile->Seek(0, aiOrigin_SET);

    GLB_Header header;
    memcpy(header.magic, AI_GLB_MAGIC_NUMBER, sizeof(header.magic));

    header.version = 1;
    AI_SWAP4(header.version);

    header.length = sizeof(header) + sceneLength + mBodyData.size();
    AI_SWAP4(header.length);

    header.sceneLength = sceneLength;
    AI_SWAP4(header.sceneLength);

    header.sceneFormat = SceneFormat_JSON;
    AI_SWAP4(header.sceneFormat);

    if (outfile->Write(&header, sizeof(header), 1) != 1) {
        throw DeadlyExportError("Failed to write the header!");
    }
}




#endif // ASSIMP_BUILD_NO_GLTF_EXPORTER
#endif // ASSIMP_BUILD_NO_EXPORT
