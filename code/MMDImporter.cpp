/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2016, assimp team

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

#ifndef ASSIMP_BUILD_NO_MMD_IMPORTER

#include "DefaultIOSystem.h"
#include "MMDImporter.h"
#include "MMDPmxParser.h"
#include "MMDPmdParser.h"
#include "MMDVmdParser.h"
//#include "IOStreamBuffer.h"
#include "ConvertToLHProcess.h"
#include <memory>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/ai_assert.h>
#include <assimp/DefaultLogger.hpp>
#include <fstream>
#include <iomanip>

static const aiImporterDesc desc = {
    "MMD Importer",
    "",
    "",
    "surfaces supported?",
    aiImporterFlags_SupportTextFlavour,
    0,
    0,
    0,
    0,
    "pmx"
};

namespace Assimp {

using namespace std;

// ------------------------------------------------------------------------------------------------
//  Default constructor
MMDImporter::MMDImporter() :
    m_Buffer(),
    //m_pRootObject( NULL ),
    m_strAbsPath( "" )
{
    DefaultIOSystem io;
    m_strAbsPath = io.getOsSeparator();
}

// ------------------------------------------------------------------------------------------------
//  Destructor.
MMDImporter::~MMDImporter()
{
    //delete m_pRootObject;
    //m_pRootObject = NULL;
}

// ------------------------------------------------------------------------------------------------
//  Returns true, if file is an pmx file.
bool MMDImporter::CanRead( const std::string& pFile, IOSystem*  pIOHandler , bool checkSig ) const
{
    if(!checkSig) //Check File Extension
    {
        return SimpleExtensionCheck(pFile,"pmx");
    }
    else //Check file Header
    {
        static const char *pTokens[] = { "PMX " };
        return BaseImporter::SearchFileHeaderForToken(pIOHandler, pFile, pTokens, 1 );
    }
}

// ------------------------------------------------------------------------------------------------
const aiImporterDesc* MMDImporter::GetInfo () const
{
    return &desc;
}

// ------------------------------------------------------------------------------------------------
//  MMD import implementation
void MMDImporter::InternReadFile( const std::string &file, aiScene* pScene, IOSystem* pIOHandler)
{
    // Read file by istream
    std::filebuf fb;
    if( !fb.open(file, std::ios::in | std::ios::binary ) ) {
        throw DeadlyImportError( "Failed to open file " + file + "." );
    }

    std::istream fileStream( &fb );

    // Get the file-size and validate it, throwing an exception when fails
    fileStream.seekg(0, fileStream.end);
    size_t fileSize = fileStream.tellg();
    fileStream.seekg(0, fileStream.beg);

    if( fileSize < sizeof(pmx::PmxModel) ) {
        throw DeadlyImportError( file + " is too small." );
    }

    pmx::PmxModel model;
    model.Read(&fileStream);

    CreateDataFromImport(&model, pScene);
}

// ------------------------------------------------------------------------------------------------
void MMDImporter::CreateDataFromImport(const pmx::PmxModel* pModel, aiScene* pScene)
{
    if( pModel == NULL ) {
        return;
    }

    pScene->mRootNode = new aiNode;
    if ( !pModel->model_name.empty() ) {
        pScene->mRootNode->mName.Set(pModel->model_name);
    }
    else {
        ai_assert(false);
    }
    
    std::cout << pScene->mRootNode->mName.C_Str() << std::endl;

    pScene->mRootNode->mNumMeshes = 1;
    pScene->mRootNode->mMeshes = new unsigned int[1];
    pScene->mRootNode->mMeshes[0] = 0;

    //aiMesh *pMesh = new aiMesh;
    aiMaterial* mat = new aiMaterial;
    pScene->mNumMeshes = 1;
    pScene->mNumMaterials = 1;
    pScene->mMeshes = new aiMesh*[1];
    pScene->mMaterials = new aiMaterial*[1];
    pScene->mMeshes[0] = CreateMesh(pModel, pScene);
    pScene->mMaterials[0] = mat;

    /**
    pMesh->mNumVertices = 3;
    pMesh->mVertices = new aiVector3D[3];
    pMesh->mVertices[0] = aiVector3D(1.0, 0.0, 0.0);
    pMesh->mVertices[1] = aiVector3D(0.0, 1.0, 0.0);
    pMesh->mVertices[2] = aiVector3D(0.0, 0.0, 1.0);

    pMesh->mNumFaces= 1;
    pMesh->mFaces = new aiFace[1];
    pMesh->mFaces[0].mNumIndices = 3;
    pMesh->mFaces[0].mIndices = new unsigned int[3];
    pMesh->mFaces[0].mIndices[0] = 0;
    pMesh->mFaces[0].mIndices[1] = 1;
    pMesh->mFaces[0].mIndices[2] = 2;
    **/
 
    // Convert everything to OpenGL space... it's the same operation as the conversion back, so we can reuse the step directly
    MakeLeftHandedProcess convertProcess;
    convertProcess.Execute( pScene);

    FlipWindingOrderProcess flipper;
    flipper.Execute(pScene);

}

// ------------------------------------------------------------------------------------------------
aiMesh* MMDImporter::CreateMesh(const pmx::PmxModel* pModel, aiScene* pScene)
{
    aiMesh *pMesh = new aiMesh;

    pMesh->mNumVertices = pModel->vertex_count;
    std::vector<int> vertices_refID(pMesh->mNumVertices, -1);
    vertices_refID.reserve(3*pMesh->mNumVertices);

    pMesh->mNumFaces = pModel->index_count / 3;
    pMesh->mFaces = new aiFace[ pMesh->mNumFaces ];

    for( unsigned int index = 0; index < pMesh->mNumFaces; index++ ) {
        pMesh->mFaces[index].mNumIndices = 3;
        unsigned int *indices = new unsigned int[3];
        // here we change LH to RH coord
        indices[0] = pModel->indices[3*index];
        indices[1] = pModel->indices[3*index+1];
        indices[2] = pModel->indices[3*index+2];
        for( unsigned int j = 0; j < 3; j++ ) {
            if(vertices_refID[indices[j]] != -1) {
                vertices_refID.push_back(indices[j]);
                indices[j] = vertices_refID.size() - 1;
            }
            else {
                vertices_refID[indices[j]] = indices[j];
            }
        }
        pMesh->mFaces[index].mIndices = indices;
    }

    pMesh->mNumVertices = vertices_refID.size();
    pMesh->mVertices = new aiVector3D[ pMesh->mNumVertices ];

    for( unsigned int index = 0; index < pMesh->mNumVertices; index++ ) {
        const float* position = pModel->vertices[vertices_refID[index]].position;
        pMesh->mVertices[index].Set(position[0], position[1], position[2]);
    }

    return pMesh;
}

// ------------------------------------------------------------------------------------------------

}   // Namespace Assimp

#endif // !! ASSIMP_BUILD_NO_MMD_IMPORTER
