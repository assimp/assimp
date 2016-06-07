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

/** @file  PlyLoader.cpp
 *  @brief Implementation of the PLY importer class
 */

#ifndef ASSIMP_BUILD_NO_PLY_IMPORTER

// internal headers
#include "PlyLoader.h"
#include "Macros.h"
#include <memory>
#include <assimp/IOSystem.hpp>
#include <assimp/scene.h>


using namespace Assimp;

static const aiImporterDesc desc = {
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


// ------------------------------------------------------------------------------------------------
// Internal stuff
namespace
{
    // ------------------------------------------------------------------------------------------------
    // Checks that property index is within range
    template <class T>
    const T &GetProperty(const std::vector<T> &props, int idx)
    {
        if( static_cast< size_t >( idx ) >= props.size() ) {
            throw DeadlyImportError( "Invalid .ply file: Property index is out of range." );
        }

        return props[idx];
    }
}


// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
PLYImporter::PLYImporter()
    : mBuffer(),
    pcDOM()
{}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
PLYImporter::~PLYImporter()
{}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file.
bool PLYImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler, bool checkSig) const
{
    const std::string extension = GetExtension(pFile);

    if (extension == "ply")
        return true;
    else if (!extension.length() || checkSig)
    {
        if (!pIOHandler)return true;
        const char* tokens[] = {"ply"};
        return SearchFileHeaderForToken(pIOHandler,pFile,tokens,1);
    }
    return false;
}

// ------------------------------------------------------------------------------------------------
const aiImporterDesc* PLYImporter::GetInfo () const
{
    return &desc;
}

// ------------------------------------------------------------------------------------------------
static bool isBigEndian( const char* szMe ) {
    ai_assert( NULL != szMe );

    // binary_little_endian
    // binary_big_endian
    bool isBigEndian( false );
#if (defined AI_BUILD_BIG_ENDIAN)
    if ( 'l' == *szMe || 'L' == *szMe ) {
        isBigEndian = true;
}
#else
    if ( 'b' == *szMe || 'B' == *szMe ) {
        isBigEndian = true;
    }
#endif // ! AI_BUILD_BIG_ENDIAN

    return isBigEndian;
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure.
void PLYImporter::InternReadFile( const std::string& pFile,
    aiScene* pScene, IOSystem* pIOHandler)
{
    std::unique_ptr<IOStream> file( pIOHandler->Open( pFile));

    // Check whether we can read from the file
    if( file.get() == NULL) {
        throw DeadlyImportError( "Failed to open PLY file " + pFile + ".");
    }

    // allocate storage and copy the contents of the file to a memory buffer
    std::vector<char> mBuffer2;
    TextFileToBuffer(file.get(),mBuffer2);
    mBuffer = (unsigned char*)&mBuffer2[0];

    // the beginning of the file must be PLY - magic, magic
    if ((mBuffer[0] != 'P' && mBuffer[0] != 'p') ||
        (mBuffer[1] != 'L' && mBuffer[1] != 'l') ||
        (mBuffer[2] != 'Y' && mBuffer[2] != 'y'))   {
        throw DeadlyImportError( "Invalid .ply file: Magic number \'ply\' is no there");
    }

    char* szMe = (char*)&this->mBuffer[3];
    SkipSpacesAndLineEnd(szMe,(const char**)&szMe);

    // determine the format of the file data
    PLY::DOM sPlyDom;
    if (TokenMatch(szMe,"format",6)) {
        if (TokenMatch(szMe,"ascii",5)) {
            SkipLine(szMe,(const char**)&szMe);
            if(!PLY::DOM::ParseInstance(szMe,&sPlyDom))
                throw DeadlyImportError( "Invalid .ply file: Unable to build DOM (#1)");
        } else if (!::strncmp(szMe,"binary_",7))
        {
            szMe += 7;
            const bool bIsBE( isBigEndian( szMe ) );

            // skip the line, parse the rest of the header and build the DOM
            SkipLine(szMe,(const char**)&szMe);
            if ( !PLY::DOM::ParseInstanceBinary( szMe, &sPlyDom, bIsBE ) ) {
                throw DeadlyImportError( "Invalid .ply file: Unable to build DOM (#2)" );
            }
        } else {
            throw DeadlyImportError( "Invalid .ply file: Unknown file format" );
        }
    }
    else
    {
        AI_DEBUG_INVALIDATE_PTR(this->mBuffer);
        throw DeadlyImportError( "Invalid .ply file: Missing format specification");
    }
    this->pcDOM = &sPlyDom;

    // now load a list of vertices. This must be successfully in order to procedure
    std::vector<aiVector3D> avPositions;
    this->LoadVertices(&avPositions,false);

    if ( avPositions.empty() ) {
        throw DeadlyImportError( "Invalid .ply file: No vertices found. "
            "Unable to parse the data format of the PLY file." );
    }

    // now load a list of normals.
    std::vector<aiVector3D> avNormals;
    LoadVertices(&avNormals,true);

    // load the face list
    std::vector<PLY::Face> avFaces;
    LoadFaces(&avFaces);

    // if no face list is existing we assume that the vertex
    // list is containing a list of triangles
    if (avFaces.empty())
    {
        if (avPositions.size() < 3)
        {
            throw DeadlyImportError( "Invalid .ply file: Not enough "
                "vertices to build a proper face list. ");
        }

        const unsigned int iNum = (unsigned int)avPositions.size() / 3;
        for (unsigned int i = 0; i< iNum;++i)
        {
            PLY::Face sFace;
            sFace.mIndices.push_back((iNum*3));
            sFace.mIndices.push_back((iNum*3)+1);
            sFace.mIndices.push_back((iNum*3)+2);
            avFaces.push_back(sFace);
        }
    }

    // now load a list of all materials
    std::vector<aiMaterial*> avMaterials;
    LoadMaterial(&avMaterials);

    // now load a list of all vertex color channels
    std::vector<aiColor4D> avColors;
    avColors.reserve(avPositions.size());
    LoadVertexColor(&avColors);

    // now try to load texture coordinates
    std::vector<aiVector2D> avTexCoords;
    avTexCoords.reserve(avPositions.size());
    LoadTextureCoordinates(&avTexCoords);

    // now replace the default material in all faces and validate all material indices
    ReplaceDefaultMaterial(&avFaces,&avMaterials);

    // now convert this to a list of aiMesh instances
    std::vector<aiMesh*> avMeshes;
    avMeshes.reserve(avMaterials.size()+1);
    ConvertMeshes(&avFaces,&avPositions,&avNormals,
        &avColors,&avTexCoords,&avMaterials,&avMeshes);

    if ( avMeshes.empty() ) {
        throw DeadlyImportError( "Invalid .ply file: Unable to extract mesh data " );
    }

    // now generate the output scene object. Fill the material list
    pScene->mNumMaterials = (unsigned int)avMaterials.size();
    pScene->mMaterials = new aiMaterial*[pScene->mNumMaterials];
    for ( unsigned int i = 0; i < pScene->mNumMaterials; ++i ) {
        pScene->mMaterials[ i ] = avMaterials[ i ];
    }

    // fill the mesh list
    pScene->mNumMeshes = (unsigned int)avMeshes.size();
    pScene->mMeshes = new aiMesh*[pScene->mNumMeshes];
    for ( unsigned int i = 0; i < pScene->mNumMeshes; ++i ) {
        pScene->mMeshes[ i ] = avMeshes[ i ];
    }

    // generate a simple node structure
    pScene->mRootNode = new aiNode();
    pScene->mRootNode->mNumMeshes = pScene->mNumMeshes;
    pScene->mRootNode->mMeshes = new unsigned int[pScene->mNumMeshes];

    for ( unsigned int i = 0; i < pScene->mRootNode->mNumMeshes; ++i ) {
        pScene->mRootNode->mMeshes[ i ] = i;
    }
}

// ------------------------------------------------------------------------------------------------
// Split meshes by material IDs
void PLYImporter::ConvertMeshes(std::vector<PLY::Face>* avFaces,
    const std::vector<aiVector3D>*          avPositions,
    const std::vector<aiVector3D>*          avNormals,
    const std::vector<aiColor4D>*           avColors,
    const std::vector<aiVector2D>*          avTexCoords,
    const std::vector<aiMaterial*>*     avMaterials,
    std::vector<aiMesh*>* avOut)
{
    ai_assert(NULL != avFaces);
    ai_assert(NULL != avPositions);
    ai_assert(NULL != avMaterials);

    // split by materials
    std::vector<unsigned int>* aiSplit = new std::vector<unsigned int>[avMaterials->size()];

    unsigned int iNum = 0;
    for (std::vector<PLY::Face>::const_iterator i = avFaces->begin();i != avFaces->end();++i,++iNum)
        aiSplit[(*i).iMaterialIndex].push_back(iNum);

    // now generate sub-meshes
    for (unsigned int p = 0; p < avMaterials->size();++p)
    {
        if (aiSplit[p].size() != 0)
        {
            // allocate the mesh object
            aiMesh* p_pcOut = new aiMesh();
            p_pcOut->mMaterialIndex = p;

            p_pcOut->mNumFaces = (unsigned int)aiSplit[p].size();
            p_pcOut->mFaces = new aiFace[aiSplit[p].size()];

            // at first we need to determine the size of the output vector array
            unsigned int iNum = 0;
            for (unsigned int i = 0; i < aiSplit[p].size();++i)
            {
                iNum += (unsigned int)(*avFaces)[aiSplit[p][i]].mIndices.size();
            }
            p_pcOut->mNumVertices = iNum;
            if( 0 == iNum ) {     // nothing to do 
                delete[] aiSplit; // cleanup
                delete p_pcOut;
                return;
            }
            p_pcOut->mVertices = new aiVector3D[iNum];

            if (!avColors->empty())
                p_pcOut->mColors[0] = new aiColor4D[iNum];
            if (!avTexCoords->empty())
            {
                p_pcOut->mNumUVComponents[0] = 2;
                p_pcOut->mTextureCoords[0] = new aiVector3D[iNum];
            }
            if (!avNormals->empty())
                p_pcOut->mNormals = new aiVector3D[iNum];

            // add all faces
            iNum = 0;
            unsigned int iVertex = 0;
            for (std::vector<unsigned int>::const_iterator i =  aiSplit[p].begin();
                i != aiSplit[p].end();++i,++iNum)
            {
                p_pcOut->mFaces[iNum].mNumIndices = (unsigned int)(*avFaces)[*i].mIndices.size();
                p_pcOut->mFaces[iNum].mIndices = new unsigned int[p_pcOut->mFaces[iNum].mNumIndices];

                // build an unique set of vertices/colors for this face
                for (unsigned int q = 0; q <  p_pcOut->mFaces[iNum].mNumIndices;++q)
                {
                    p_pcOut->mFaces[iNum].mIndices[q] = iVertex;
                    const size_t idx = ( *avFaces )[ *i ].mIndices[ q ];
                    if( idx >= ( *avPositions ).size() ) {
                        // out of border
                        continue;
                    }
                    p_pcOut->mVertices[ iVertex ] = ( *avPositions )[ idx ];

                    if (!avColors->empty())
                        p_pcOut->mColors[ 0 ][ iVertex ] = ( *avColors )[ idx ];

                    if (!avTexCoords->empty())
                    {
                        const aiVector2D& vec = ( *avTexCoords )[ idx ];
                        p_pcOut->mTextureCoords[0][iVertex].x = vec.x;
                        p_pcOut->mTextureCoords[0][iVertex].y = vec.y;
                    }

                    if (!avNormals->empty())
                        p_pcOut->mNormals[ iVertex ] = ( *avNormals )[ idx ];
                    iVertex++;
                }

            }
            // add the mesh to the output list
            avOut->push_back(p_pcOut);
        }
    }
    delete[] aiSplit; // cleanup
}

// ------------------------------------------------------------------------------------------------
// Generate a default material if none was specified and apply it to all vanilla faces
void PLYImporter::ReplaceDefaultMaterial(std::vector<PLY::Face>* avFaces,
    std::vector<aiMaterial*>* avMaterials)
{
    bool bNeedDefaultMat = false;

    for (std::vector<PLY::Face>::iterator i =  avFaces->begin();i != avFaces->end();++i)    {
        if (0xFFFFFFFF == (*i).iMaterialIndex)  {
            bNeedDefaultMat = true;
            (*i).iMaterialIndex = (unsigned int)avMaterials->size();
        }
        else if ((*i).iMaterialIndex >= avMaterials->size() )   {
            // clamp the index
            (*i).iMaterialIndex = (unsigned int)avMaterials->size()-1;
        }
    }

    if (bNeedDefaultMat)    {
        // generate a default material
        aiMaterial* pcHelper = new aiMaterial();

        // fill in a default material
        int iMode = (int)aiShadingMode_Gouraud;
        pcHelper->AddProperty<int>(&iMode, 1, AI_MATKEY_SHADING_MODEL);

        aiColor3D clr;
        clr.b = clr.g = clr.r = 0.6f;
        pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_DIFFUSE);
        pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_SPECULAR);

        clr.b = clr.g = clr.r = 0.05f;
        pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_AMBIENT);

        // The face order is absolutely undefined for PLY, so we have to
        // use two-sided rendering to be sure it's ok.
        const int two_sided = 1;
        pcHelper->AddProperty(&two_sided,1,AI_MATKEY_TWOSIDED);

        avMaterials->push_back(pcHelper);
    }
}

// ------------------------------------------------------------------------------------------------
void PLYImporter::LoadTextureCoordinates(std::vector<aiVector2D>* pvOut)
{
    ai_assert(NULL != pvOut);

    unsigned int aiPositions[2] = {0xFFFFFFFF,0xFFFFFFFF};
    PLY::EDataType aiTypes[2] = {EDT_Char,EDT_Char};
    PLY::ElementInstanceList* pcList = NULL;
    unsigned int cnt = 0;

    // serach in the DOM for a vertex entry
    unsigned int _i = 0;
    for (std::vector<PLY::Element>::const_iterator i = pcDOM->alElements.begin();
        i != pcDOM->alElements.end();++i,++_i)
    {
        if (PLY::EEST_Vertex == (*i).eSemantic)
        {
            pcList = &this->pcDOM->alElementData[_i];

            // now check whether which normal components are available
            unsigned int _a = 0;
            for (std::vector<PLY::Property>::const_iterator a =  (*i).alProperties.begin();
                a != (*i).alProperties.end();++a,++_a)
            {
                if ((*a).bIsList)continue;
                if (PLY::EST_UTextureCoord == (*a).Semantic)
                {
                    cnt++;
                    aiPositions[0] = _a;
                    aiTypes[0] = (*a).eType;
                }
                else if (PLY::EST_VTextureCoord == (*a).Semantic)
                {
                    cnt++;
                    aiPositions[1] = _a;
                    aiTypes[1] = (*a).eType;
                }
            }
        }
    }
    // check whether we have a valid source for the texture coordinates data
    if (NULL != pcList && 0 != cnt)
    {
        pvOut->reserve(pcList->alInstances.size());
        for (std::vector<ElementInstance>::const_iterator i = pcList->alInstances.begin();
            i != pcList->alInstances.end();++i)
        {
            // convert the vertices to sp floats
            aiVector2D vOut;

            if (0xFFFFFFFF != aiPositions[0])
            {
                vOut.x = PLY::PropertyInstance::ConvertTo<float>(
                    GetProperty((*i).alProperties, aiPositions[0]).avList.front(),aiTypes[0]);
            }

            if (0xFFFFFFFF != aiPositions[1])
            {
                vOut.y = PLY::PropertyInstance::ConvertTo<float>(
                    GetProperty((*i).alProperties, aiPositions[1]).avList.front(),aiTypes[1]);
            }
            // and add them to our nice list
            pvOut->push_back(vOut);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Try to extract vertices from the PLY DOM
void PLYImporter::LoadVertices(std::vector<aiVector3D>* pvOut, bool p_bNormals)
{
    ai_assert(NULL != pvOut);

    unsigned int aiPositions[3] = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};
    PLY::EDataType aiTypes[3] = {EDT_Char,EDT_Char,EDT_Char};
    PLY::ElementInstanceList* pcList = NULL;
    unsigned int cnt = 0;

    // search in the DOM for a vertex entry
    unsigned int _i = 0;
    for (std::vector<PLY::Element>::const_iterator i =  pcDOM->alElements.begin();
        i != pcDOM->alElements.end();++i,++_i)
    {
        if (PLY::EEST_Vertex == (*i).eSemantic)
        {
            pcList = &pcDOM->alElementData[_i];

            // load normal vectors?
            if (p_bNormals)
            {
                // now check whether which normal components are available
                unsigned int _a = 0;
                for (std::vector<PLY::Property>::const_iterator a = (*i).alProperties.begin();
                    a != (*i).alProperties.end();++a,++_a)
                {
                    if ((*a).bIsList)continue;
                    if (PLY::EST_XNormal == (*a).Semantic)
                    {
                        cnt++;
                        aiPositions[0] = _a;
                        aiTypes[0] = (*a).eType;
                    }
                    else if (PLY::EST_YNormal == (*a).Semantic)
                    {
                        cnt++;
                        aiPositions[1] = _a;
                        aiTypes[1] = (*a).eType;
                    }
                    else if (PLY::EST_ZNormal == (*a).Semantic)
                    {
                        cnt++;
                        aiPositions[2] = _a;
                        aiTypes[2] = (*a).eType;
                    }
                }
            }
            // load vertex coordinates
            else
            {
                // now check whether which coordinate sets are available
                unsigned int _a = 0;
                for (std::vector<PLY::Property>::const_iterator a = (*i).alProperties.begin();
                    a != (*i).alProperties.end();++a,++_a)
                {
                    if ((*a).bIsList)continue;
                    if (PLY::EST_XCoord == (*a).Semantic)
                    {
                        cnt++;
                        aiPositions[0] = _a;
                        aiTypes[0] = (*a).eType;
                    }
                    else if (PLY::EST_YCoord == (*a).Semantic)
                    {
                        cnt++;
                        aiPositions[1] = _a;
                        aiTypes[1] = (*a).eType;
                    }
                    else if (PLY::EST_ZCoord == (*a).Semantic)
                    {
                        cnt++;
                        aiPositions[2] = _a;
                        aiTypes[2] = (*a).eType;
                    }
                    if (3 == cnt)break;
                }
            }
            break;
        }
    }
    // check whether we have a valid source for the vertex data
    if (NULL != pcList && 0 != cnt)
    {
        pvOut->reserve(pcList->alInstances.size());
        for (std::vector<ElementInstance>::const_iterator
            i =  pcList->alInstances.begin();
            i != pcList->alInstances.end();++i)
        {
            // convert the vertices to sp floats
            aiVector3D vOut;

            if (0xFFFFFFFF != aiPositions[0])
            {
                vOut.x = PLY::PropertyInstance::ConvertTo<float>(
                    GetProperty((*i).alProperties, aiPositions[0]).avList.front(),aiTypes[0]);
            }

            if (0xFFFFFFFF != aiPositions[1])
            {
                vOut.y = PLY::PropertyInstance::ConvertTo<float>(
                    GetProperty((*i).alProperties, aiPositions[1]).avList.front(),aiTypes[1]);
            }

            if (0xFFFFFFFF != aiPositions[2])
            {
                vOut.z = PLY::PropertyInstance::ConvertTo<float>(
                    GetProperty((*i).alProperties, aiPositions[2]).avList.front(),aiTypes[2]);
            }

            // and add them to our nice list
            pvOut->push_back(vOut);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Convert a color component to [0...1]
float PLYImporter::NormalizeColorValue (PLY::PropertyInstance::ValueUnion val,
    PLY::EDataType eType)
{
    switch (eType)
    {
    case EDT_Float:
        return val.fFloat;
    case EDT_Double:
        return (float)val.fDouble;

    case EDT_UChar:
        return (float)val.iUInt / (float)0xFF;
    case EDT_Char:
        return (float)(val.iInt+(0xFF/2)) / (float)0xFF;
    case EDT_UShort:
        return (float)val.iUInt / (float)0xFFFF;
    case EDT_Short:
        return (float)(val.iInt+(0xFFFF/2)) / (float)0xFFFF;
    case EDT_UInt:
        return (float)val.iUInt / (float)0xFFFF;
    case EDT_Int:
        return ((float)val.iInt / (float)0xFF) + 0.5f;
    default: ;
    };
    return 0.0f;
}

// ------------------------------------------------------------------------------------------------
// Try to extract proper vertex colors from the PLY DOM
void PLYImporter::LoadVertexColor(std::vector<aiColor4D>* pvOut)
{
    ai_assert(NULL != pvOut);

    unsigned int aiPositions[4] = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};
    PLY::EDataType aiTypes[4] = {EDT_Char, EDT_Char, EDT_Char, EDT_Char}; // silencing gcc
    unsigned int cnt = 0;
    PLY::ElementInstanceList* pcList = NULL;

    // serach in the DOM for a vertex entry
    unsigned int _i = 0;
    for (std::vector<PLY::Element>::const_iterator i = pcDOM->alElements.begin();
        i != pcDOM->alElements.end();++i,++_i)
    {
        if (PLY::EEST_Vertex == (*i).eSemantic)
        {
            pcList = &this->pcDOM->alElementData[_i];

            // now check whether which coordinate sets are available
            unsigned int _a = 0;
            for (std::vector<PLY::Property>::const_iterator
                a =  (*i).alProperties.begin();
                a != (*i).alProperties.end();++a,++_a)
            {
                if ((*a).bIsList)continue;
                if (PLY::EST_Red == (*a).Semantic)
                {
                    cnt++;
                    aiPositions[0] = _a;
                    aiTypes[0] = (*a).eType;
                }
                else if (PLY::EST_Green == (*a).Semantic)
                {
                    cnt++;
                    aiPositions[1] = _a;
                    aiTypes[1] = (*a).eType;
                }
                else if (PLY::EST_Blue == (*a).Semantic)
                {
                    cnt++;
                    aiPositions[2] = _a;
                    aiTypes[2] = (*a).eType;
                }
                else if (PLY::EST_Alpha == (*a).Semantic)
                {
                    cnt++;
                    aiPositions[3] = _a;
                    aiTypes[3] = (*a).eType;
                }
                if (4 == cnt)break;
            }
            break;
        }
    }
    // check whether we have a valid source for the vertex data
    if (NULL != pcList && 0 != cnt)
    {
        pvOut->reserve(pcList->alInstances.size());
        for (std::vector<ElementInstance>::const_iterator i = pcList->alInstances.begin();
            i != pcList->alInstances.end();++i)
        {
            // convert the vertices to sp floats
            aiColor4D vOut;

            if (0xFFFFFFFF != aiPositions[0])
            {
                vOut.r = NormalizeColorValue(GetProperty((*i).alProperties,
                    aiPositions[0]).avList.front(),aiTypes[0]);
            }

            if (0xFFFFFFFF != aiPositions[1])
            {
                vOut.g = NormalizeColorValue(GetProperty((*i).alProperties,
                    aiPositions[1]).avList.front(),aiTypes[1]);
            }

            if (0xFFFFFFFF != aiPositions[2])
            {
                vOut.b = NormalizeColorValue(GetProperty((*i).alProperties,
                    aiPositions[2]).avList.front(),aiTypes[2]);
            }

            // assume 1.0 for the alpha channel ifit is not set
            if (0xFFFFFFFF == aiPositions[3])vOut.a = 1.0f;
            else
            {
                vOut.a = NormalizeColorValue(GetProperty((*i).alProperties,
                    aiPositions[3]).avList.front(),aiTypes[3]);
            }

            // and add them to our nice list
            pvOut->push_back(vOut);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Try to extract proper faces from the PLY DOM
void PLYImporter::LoadFaces(std::vector<PLY::Face>* pvOut)
{
    ai_assert(NULL != pvOut);

    PLY::ElementInstanceList* pcList = NULL;
    bool bOne = false;

    // index of the vertex index list
    unsigned int iProperty = 0xFFFFFFFF;
    PLY::EDataType eType = EDT_Char;
    bool bIsTristrip = false;

    // index of the material index property
    unsigned int iMaterialIndex = 0xFFFFFFFF;
    PLY::EDataType eType2 = EDT_Char;

    // serach in the DOM for a face entry
    unsigned int _i = 0;
    for (std::vector<PLY::Element>::const_iterator i =  pcDOM->alElements.begin();
        i != pcDOM->alElements.end();++i,++_i)
    {
        // face = unique number of vertex indices
        if (PLY::EEST_Face == (*i).eSemantic)
        {
            pcList = &pcDOM->alElementData[_i];
            unsigned int _a = 0;
            for (std::vector<PLY::Property>::const_iterator a =  (*i).alProperties.begin();
                a != (*i).alProperties.end();++a,++_a)
            {
                if (PLY::EST_VertexIndex == (*a).Semantic)
                {
                    // must be a dynamic list!
                    if (!(*a).bIsList)continue;
                    iProperty   = _a;
                    bOne        = true;
                    eType       = (*a).eType;
                }
                else if (PLY::EST_MaterialIndex == (*a).Semantic)
                {
                    if ((*a).bIsList)continue;
                    iMaterialIndex  = _a;
                    bOne            = true;
                    eType2      = (*a).eType;
                }
            }
            break;
        }
        // triangle strip
        // TODO: triangle strip and material index support???
        else if (PLY::EEST_TriStrip == (*i).eSemantic)
        {
            // find a list property in this ...
            pcList = &this->pcDOM->alElementData[_i];
            unsigned int _a = 0;
            for (std::vector<PLY::Property>::const_iterator a =  (*i).alProperties.begin();
                a != (*i).alProperties.end();++a,++_a)
            {
                // must be a dynamic list!
                if (!(*a).bIsList)continue;
                iProperty   = _a;
                bOne        = true;
                bIsTristrip = true;
                eType       = (*a).eType;
                break;
            }
            break;
        }
    }
    // check whether we have at least one per-face information set
    if (pcList && bOne)
    {
        if (!bIsTristrip)
        {
            pvOut->reserve(pcList->alInstances.size());
            for (std::vector<ElementInstance>::const_iterator i =  pcList->alInstances.begin();
                i != pcList->alInstances.end();++i)
            {
                PLY::Face sFace;

                // parse the list of vertex indices
                if (0xFFFFFFFF != iProperty)
                {
                    const unsigned int iNum = (unsigned int)GetProperty((*i).alProperties, iProperty).avList.size();
                    sFace.mIndices.resize(iNum);

                    std::vector<PLY::PropertyInstance::ValueUnion>::const_iterator p =
                        GetProperty((*i).alProperties, iProperty).avList.begin();

                    for (unsigned int a = 0; a < iNum;++a,++p)
                    {
                        sFace.mIndices[a] = PLY::PropertyInstance::ConvertTo<unsigned int>(*p,eType);
                    }
                }

                // parse the material index
                if (0xFFFFFFFF != iMaterialIndex)
                {
                    sFace.iMaterialIndex = PLY::PropertyInstance::ConvertTo<unsigned int>(
                        GetProperty((*i).alProperties, iMaterialIndex).avList.front(),eType2);
                }
                pvOut->push_back(sFace);
            }
        }
        else // triangle strips
        {
            // normally we have only one triangle strip instance where
            // a value of -1 indicates a restart of the strip
            bool flip = false;
            for (std::vector<ElementInstance>::const_iterator i = pcList->alInstances.begin();i != pcList->alInstances.end();++i) {
                const std::vector<PLY::PropertyInstance::ValueUnion>& quak = GetProperty((*i).alProperties, iProperty).avList;
                pvOut->reserve(pvOut->size() + quak.size() + (quak.size()>>2u));

                int aiTable[2] = {-1,-1};
                for (std::vector<PLY::PropertyInstance::ValueUnion>::const_iterator a =  quak.begin();a != quak.end();++a)  {
                    const int p = PLY::PropertyInstance::ConvertTo<int>(*a,eType);

                    if (-1 == p)    {
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

                    pvOut->push_back(PLY::Face());
                    PLY::Face& sFace = pvOut->back();
                    sFace.mIndices[0] = aiTable[0];
                    sFace.mIndices[1] = aiTable[1];
                    sFace.mIndices[2] = p;
                    if ((flip = !flip)) {
                        std::swap(sFace.mIndices[0],sFace.mIndices[1]);
                    }

                    aiTable[0] = aiTable[1];
                    aiTable[1] = p;
                }
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Get a RGBA color in [0...1] range
void PLYImporter::GetMaterialColor(const std::vector<PLY::PropertyInstance>& avList,
    unsigned int aiPositions[4],
    PLY::EDataType aiTypes[4],
     aiColor4D* clrOut)
{
    ai_assert(NULL != clrOut);

    if (0xFFFFFFFF == aiPositions[0])clrOut->r = 0.0f;
    else
    {
        clrOut->r = NormalizeColorValue(GetProperty(avList,
            aiPositions[0]).avList.front(),aiTypes[0]);
    }

    if (0xFFFFFFFF == aiPositions[1])clrOut->g = 0.0f;
    else
    {
        clrOut->g = NormalizeColorValue(GetProperty(avList,
            aiPositions[1]).avList.front(),aiTypes[1]);
    }

    if (0xFFFFFFFF == aiPositions[2])clrOut->b = 0.0f;
    else
    {
        clrOut->b = NormalizeColorValue(GetProperty(avList,
            aiPositions[2]).avList.front(),aiTypes[2]);
    }

    // assume 1.0 for the alpha channel ifit is not set
    if (0xFFFFFFFF == aiPositions[3])clrOut->a = 1.0f;
    else
    {
        clrOut->a = NormalizeColorValue(GetProperty(avList,
            aiPositions[3]).avList.front(),aiTypes[3]);
    }
}

// ------------------------------------------------------------------------------------------------
// Extract a material from the PLY DOM
void PLYImporter::LoadMaterial(std::vector<aiMaterial*>* pvOut)
{
    ai_assert(NULL != pvOut);

    // diffuse[4], specular[4], ambient[4]
    // rgba order
    unsigned int aaiPositions[3][4] = {

        {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF},
        {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF},
        {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF},
    };

    PLY::EDataType aaiTypes[3][4] = {
        {EDT_Char,EDT_Char,EDT_Char,EDT_Char},
        {EDT_Char,EDT_Char,EDT_Char,EDT_Char},
        {EDT_Char,EDT_Char,EDT_Char,EDT_Char}
    };
    PLY::ElementInstanceList* pcList = NULL;

    unsigned int iPhong = 0xFFFFFFFF;
    PLY::EDataType ePhong = EDT_Char;

    unsigned int iOpacity = 0xFFFFFFFF;
    PLY::EDataType eOpacity = EDT_Char;

    // serach in the DOM for a vertex entry
    unsigned int _i = 0;
    for (std::vector<PLY::Element>::const_iterator i =  this->pcDOM->alElements.begin();
        i != this->pcDOM->alElements.end();++i,++_i)
    {
        if (PLY::EEST_Material == (*i).eSemantic)
        {
            pcList = &this->pcDOM->alElementData[_i];

            // now check whether which coordinate sets are available
            unsigned int _a = 0;
            for (std::vector<PLY::Property>::const_iterator
                a =  (*i).alProperties.begin();
                a != (*i).alProperties.end();++a,++_a)
            {
                if ((*a).bIsList)continue;

                // pohng specularity      -----------------------------------
                if (PLY::EST_PhongPower == (*a).Semantic)
                {
                    iPhong      = _a;
                    ePhong      = (*a).eType;
                }

                // general opacity        -----------------------------------
                if (PLY::EST_Opacity == (*a).Semantic)
                {
                    iOpacity        = _a;
                    eOpacity        = (*a).eType;
                }

                // diffuse color channels -----------------------------------
                if (PLY::EST_DiffuseRed == (*a).Semantic)
                {
                    aaiPositions[0][0]  = _a;
                    aaiTypes[0][0]      = (*a).eType;
                }
                else if (PLY::EST_DiffuseGreen == (*a).Semantic)
                {
                    aaiPositions[0][1]  = _a;
                    aaiTypes[0][1]      = (*a).eType;
                }
                else if (PLY::EST_DiffuseBlue == (*a).Semantic)
                {
                    aaiPositions[0][2]  = _a;
                    aaiTypes[0][2]      = (*a).eType;
                }
                else if (PLY::EST_DiffuseAlpha == (*a).Semantic)
                {
                    aaiPositions[0][3]  = _a;
                    aaiTypes[0][3]      = (*a).eType;
                }
                // specular color channels -----------------------------------
                else if (PLY::EST_SpecularRed == (*a).Semantic)
                {
                    aaiPositions[1][0]  = _a;
                    aaiTypes[1][0]      = (*a).eType;
                }
                else if (PLY::EST_SpecularGreen == (*a).Semantic)
                {
                    aaiPositions[1][1]  = _a;
                    aaiTypes[1][1]      = (*a).eType;
                }
                else if (PLY::EST_SpecularBlue == (*a).Semantic)
                {
                    aaiPositions[1][2]  = _a;
                    aaiTypes[1][2]      = (*a).eType;
                }
                else if (PLY::EST_SpecularAlpha == (*a).Semantic)
                {
                    aaiPositions[1][3]  = _a;
                    aaiTypes[1][3]      = (*a).eType;
                }
                // ambient color channels -----------------------------------
                else if (PLY::EST_AmbientRed == (*a).Semantic)
                {
                    aaiPositions[2][0]  = _a;
                    aaiTypes[2][0]      = (*a).eType;
                }
                else if (PLY::EST_AmbientGreen == (*a).Semantic)
                {
                    aaiPositions[2][1]  = _a;
                    aaiTypes[2][1]      = (*a).eType;
                }
                else if (PLY::EST_AmbientBlue == (*a).Semantic)
                {
                    aaiPositions[2][2]  = _a;
                    aaiTypes[2][2]      = (*a).eType;
                }
                else if (PLY::EST_AmbientAlpha == (*a).Semantic)
                {
                    aaiPositions[2][3]  = _a;
                    aaiTypes[2][3]      = (*a).eType;
                }
            }
            break;
        }
    }
    // check whether we have a valid source for the material data
    if (NULL != pcList) {
        for (std::vector<ElementInstance>::const_iterator i =  pcList->alInstances.begin();i != pcList->alInstances.end();++i)  {
            aiColor4D clrOut;
            aiMaterial* pcHelper = new aiMaterial();

            // build the diffuse material color
            GetMaterialColor((*i).alProperties,aaiPositions[0],aaiTypes[0],&clrOut);
            pcHelper->AddProperty<aiColor4D>(&clrOut,1,AI_MATKEY_COLOR_DIFFUSE);

            // build the specular material color
            GetMaterialColor((*i).alProperties,aaiPositions[1],aaiTypes[1],&clrOut);
            pcHelper->AddProperty<aiColor4D>(&clrOut,1,AI_MATKEY_COLOR_SPECULAR);

            // build the ambient material color
            GetMaterialColor((*i).alProperties,aaiPositions[2],aaiTypes[2],&clrOut);
            pcHelper->AddProperty<aiColor4D>(&clrOut,1,AI_MATKEY_COLOR_AMBIENT);

            // handle phong power and shading mode
            int iMode;
            if (0xFFFFFFFF != iPhong)   {
                float fSpec = PLY::PropertyInstance::ConvertTo<float>(GetProperty((*i).alProperties, iPhong).avList.front(),ePhong);

                // if shininess is 0 (and the pow() calculation would therefore always
                // become 1, not depending on the angle), use gouraud lighting
                if (fSpec)  {
                    // scale this with 15 ... hopefully this is correct
                    fSpec *= 15;
                    pcHelper->AddProperty<float>(&fSpec, 1, AI_MATKEY_SHININESS);

                    iMode = (int)aiShadingMode_Phong;
                }
                else iMode = (int)aiShadingMode_Gouraud;
            }
            else iMode = (int)aiShadingMode_Gouraud;
            pcHelper->AddProperty<int>(&iMode, 1, AI_MATKEY_SHADING_MODEL);

            // handle opacity
            if (0xFFFFFFFF != iOpacity) {
                float fOpacity = PLY::PropertyInstance::ConvertTo<float>(GetProperty((*i).alProperties, iPhong).avList.front(),eOpacity);
                pcHelper->AddProperty<float>(&fOpacity, 1, AI_MATKEY_OPACITY);
            }

            // The face order is absolutely undefined for PLY, so we have to
            // use two-sided rendering to be sure it's ok.
            const int two_sided = 1;
            pcHelper->AddProperty(&two_sided,1,AI_MATKEY_TWOSIDED);

            // add the newly created material instance to the list
            pvOut->push_back(pcHelper);
        }
    }
}

#endif // !! ASSIMP_BUILD_NO_PLY_IMPORTER
