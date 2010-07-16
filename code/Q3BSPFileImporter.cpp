/*
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team
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

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

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

---------------------------------------------------------------------------------------------------
*/
#include "AssimpPCH.h"
#ifndef ASSIMP_BUILD_NO_Q3BSP_IMPORTER

#include "DefaultIOSystem.h"
#include "Q3BSPFileImporter.h"
#include "Q3BSPZipArchive.h"
#include "Q3BSPFileParser.h"
#include "Q3BSPFileData.h"

#	ifdef ASSIMP_BUILD_NO_OWN_ZLIB
#		include <zlib.h>
#	else
#		include "../contrib/zlib/zlib.h"
#	endif


#include "../include/aiTypes.h"
#include "../include/aiMesh.h"
#include <vector>

namespace Assimp
{

using namespace Q3BSP;

// ------------------------------------------------------------------------------------------------
static void createKey( int id1, int id2, std::string &rKey )
{
	std::stringstream str;
	str << id1 << "." << id2;
	rKey = str.str();
}

// ------------------------------------------------------------------------------------------------
Q3BSPFileImporter::Q3BSPFileImporter() :
	m_pCurrentMesh( NULL )
{
}

// ------------------------------------------------------------------------------------------------
Q3BSPFileImporter::~Q3BSPFileImporter()
{
}

// ------------------------------------------------------------------------------------------------
bool Q3BSPFileImporter::CanRead( const std::string& rFile, IOSystem* pIOHandler, bool checkSig ) const
{
	bool isBSPData = false;
	if ( checkSig )
		isBSPData = SimpleExtensionCheck( rFile, "pk3" );

	return isBSPData;
}

// ------------------------------------------------------------------------------------------------
void Q3BSPFileImporter::GetExtensionList(std::set<std::string>& extensions)
{
	extensions.insert( "pk3" );
}

// ------------------------------------------------------------------------------------------------
void Q3BSPFileImporter::InternReadFile(const std::string &rFile, aiScene* pScene, IOSystem* pIOHandler)
{
	Q3BSPZipArchive Archive( rFile );
	if ( !Archive.isOpen() )
	{
		throw new DeadlyImportError( "Failed to open file " + rFile + "." );
	}

	std::string archiveName( "" ), mapName( "" );
	separateMapName( rFile, archiveName, mapName );

	if ( mapName.empty() )
	{
		if ( !findFirstMapInArchive( Archive, mapName ) )
		{
			return;
		}
	}

	Q3BSPFileParser fileParser( mapName, &Archive );
	Q3BSPModel *pBSPModel = fileParser.getModel();
	if ( NULL != pBSPModel )
	{
		CreateDataFromImport( pBSPModel, pScene );
	}
}

// ------------------------------------------------------------------------------------------------
void Q3BSPFileImporter::separateMapName( const std::string &rImportName, std::string &rArchiveName, 
										std::string &rMapName )
{
	rArchiveName = "";
	rMapName = "";
	if ( rImportName.empty() )
		return;

	std::string::size_type pos = rImportName.rfind( "," );
	if ( std::string::npos == pos )
	{
		rArchiveName = rImportName;
		return;
	}

	rArchiveName = rImportName.substr( 0, pos );
	rMapName = rImportName.substr( pos, rImportName.size() - pos - 1 );
}

// ------------------------------------------------------------------------------------------------
bool Q3BSPFileImporter::findFirstMapInArchive( Q3BSPZipArchive &rArchive, std::string &rMapName )
{
	rMapName = "";
	std::vector<std::string> fileList;
	rArchive.getFileList( fileList );
	if ( fileList.empty() )  
		return false;

	for ( std::vector<std::string>::iterator it = fileList.begin(); it != fileList.end();
		++it )
	{
		std::string::size_type pos = (*it).find( "maps/" );
		if ( std::string::npos != pos )
		{
			std::string::size_type extPos = (*it).find( ".bsp" );
			if ( std::string::npos != extPos )
			{
				rMapName = *it;
				return true;
			}
		}
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
void Q3BSPFileImporter::CreateDataFromImport( const Q3BSP::Q3BSPModel *pModel, aiScene* pScene )
{
	if ( NULL == pModel || NULL == pScene )
		return;

	pScene->mRootNode = new aiNode;
	if ( !pModel->m_ModelName.empty() )
	{
		pScene->mRootNode->mName.Set( pModel->m_ModelName );
	}

	CreateNodes( pModel, pScene, pScene->mRootNode );
}

// ------------------------------------------------------------------------------------------------
void Q3BSPFileImporter::CreateNodes( const Q3BSP::Q3BSPModel *pModel, aiScene* pScene, aiNode *pParent )
{
	ai_assert( NULL != pModel );
	if ( NULL == pModel )
		return;

	FaceMap matLookupTable;
	std::string key( "" );
	std::vector<sQ3BSPFace*> *pCurFaceArray = NULL;
	FaceMap lookupMap;
	for ( size_t idx=0; idx < pModel->m_Faces.size(); idx++ )
	{
		Q3BSP::sQ3BSPFace *pQ3BSPFace = pModel->m_Faces[ idx ];
		int texId = pQ3BSPFace->iTextureID;
		int lightMapId = pQ3BSPFace->iLightmapID;
		createKey( texId, lightMapId, key );
		FaceMapIt it = lookupMap.find( key );
		if ( lookupMap.end() == it)
		{
			std::vector<Q3BSP::sQ3BSPFace*> *pArray = new std::vector<Q3BSP::sQ3BSPFace*>;
			pArray->push_back( pQ3BSPFace );
			lookupMap[ key ] = pArray;
		}
		else
		{
			std::vector<Q3BSP::sQ3BSPFace*> *pArray = (*it).second;
			ai_assert( NULL != pArray );
			if ( NULL != pArray )
			{
				pArray->push_back( pQ3BSPFace );
			}
		}
	}
	
	std::vector<aiMesh*> MeshArray;
	std::vector<aiNode*> NodeArray;
	for ( FaceMapIt it = lookupMap.begin(); it != lookupMap.end(); ++it )
	{
		std::vector<Q3BSP::sQ3BSPFace*> *pArray = (*it).second;
		size_t numVerts = countData( *pArray );
		if ( 0 != numVerts )
		{
			aiMesh* pMesh = new aiMesh;
			aiNode *pNode = CreateTopology( pModel, *pArray, pMesh );
			if ( NULL != pNode )
			{
				NodeArray.push_back( pNode );
				MeshArray.push_back( pMesh );
			}
			else
			{
				delete pMesh;
			}
		}
	}

	pScene->mNumMeshes = MeshArray.size();
	pScene->mMeshes = new aiMesh*[ pScene->mNumMeshes ];
	for ( size_t i=0; i<MeshArray.size(); ++i )
	{
		aiMesh *pMesh = MeshArray[ i ];
		for ( size_t j=0; j<pMesh->mNumFaces; j++ )
		{
			aiFace *face = &pMesh->mFaces[j];
			if ( NULL == face->mIndices )
			{
				printf("error\n");
			}
			ai_assert( NULL != face->mIndices );
		}
		pScene->mMeshes[ i ] = pMesh;
	}

	pParent->mNumChildren = MeshArray.size();
	pParent->mChildren = new aiNode*[ pScene->mRootNode->mNumChildren ];
	for ( size_t i=0; i<NodeArray.size(); i++ )
	{
		aiNode *pNode = NodeArray[ i ];
		pNode->mParent = pParent;
		pParent->mChildren[ i ] = pNode;
		pParent->mChildren[ i ]->mMeshes[ 0 ] = i;
	}
}

// ------------------------------------------------------------------------------------------------
aiNode *Q3BSPFileImporter::CreateTopology( const Q3BSP::Q3BSPModel *pModel,
										  std::vector<sQ3BSPFace*> &rArray, aiMesh* pMesh )
{
	size_t numVerts = countData( rArray );
	if ( 0 == numVerts )
		return NULL;
	
	size_t numFaces = countFaces( rArray );
	if ( 0 == numFaces )
		return NULL;

	pMesh->mFaces = new aiFace[ numFaces ];
	pMesh->mNumFaces = numFaces;
	pMesh->mNumVertices = numVerts;
	pMesh->mVertices = new aiVector3D[ numVerts ];
	pMesh->mNormals =  new aiVector3D[ numVerts ];
	pMesh->mTextureCoords[ 0 ] = new aiVector3D[ numVerts ];
	pMesh->mTextureCoords[ 1 ] = new aiVector3D[ numVerts ];

	unsigned int faceIdx = 0;
	unsigned int vertIdx = 0;
	aiVector3D *pVec3( NULL );
	aiVector3D normal;
	for ( std::vector<sQ3BSPFace*>::const_iterator it = rArray.begin(); it != rArray.end(); 
		++it )
	{
		Q3BSP::sQ3BSPFace *pQ3BSPFace = *it;
		ai_assert( NULL != pQ3BSPFace );
		if ( pQ3BSPFace->iNumOfMeshVerts == 0 )
		{
			continue;
		}

		if ( pQ3BSPFace->iType == 1 || pQ3BSPFace->iType == 3 )
		{
			createTriangleTopology( pModel, pQ3BSPFace, pMesh, faceIdx, vertIdx );
		}		
	}

	aiNode *pNode = new aiNode;
	pNode->mNumMeshes = 1;
	pNode->mMeshes = new unsigned int[ 1 ];

	return pNode;
}
// ------------------------------------------------------------------------------------------------
void Q3BSPFileImporter::createTriangleTopology( const Q3BSP::Q3BSPModel *pModel,
											  Q3BSP::sQ3BSPFace *pQ3BSPFace, 
											  aiMesh* pMesh, unsigned int &rFaceIdx, 
											  unsigned int &rVertIdx )
{
	ai_assert( rFaceIdx < pMesh->mNumFaces );
	aiFace *pFace = &pMesh->mFaces[ rFaceIdx ];
	ai_assert( NULL != pFace );
	rFaceIdx++;

	pFace->mNumIndices = pQ3BSPFace->iNumOfMeshVerts;
	pFace->mIndices = new unsigned int[ pFace->mNumIndices ];
	aiVector3D normal;
	normal.Set( pQ3BSPFace->vNormal.x, pQ3BSPFace->vNormal.y, pQ3BSPFace->vNormal.z );

	for ( int i = 0; i < pQ3BSPFace->iNumOfMeshVerts; ++i )
	{
		size_t idx =  pModel->m_Indices[ pQ3BSPFace->iMeshVertexIndex + i ];;
		const unsigned int index = pQ3BSPFace->iVertexIndex + pModel->m_Indices[idx];
		ai_assert( index < pModel->m_Vertices.size() );
		sQ3BSPVertex *pVertex = pModel->m_Vertices[ index ];
		ai_assert( NULL != pVertex );

		pMesh->mVertices[ rVertIdx ].Set( pVertex->vPosition.x, pVertex->vPosition.y, pVertex->vPosition.z );
		pMesh->mNormals[ rVertIdx ].Set( normal.x, normal.y, normal.z );
				
		pMesh->mTextureCoords[ 0 ][ rVertIdx ].Set( pVertex->vTexCoord.x, pVertex->vTexCoord.y, 0.0f );
		pMesh->mTextureCoords[ 1 ][ rVertIdx ].Set( pVertex->vLightmap.x, pVertex->vLightmap.y, 0.0f );
		pFace->mIndices[ i ] = rVertIdx;
		rVertIdx++;
	}
}

// ------------------------------------------------------------------------------------------------
void Q3BSPFileImporter::createMaterials()
{
	// TODO
}

// ------------------------------------------------------------------------------------------------
size_t Q3BSPFileImporter::countData( const std::vector<sQ3BSPFace*> &rArray ) const
{
	size_t numVerts = 0;
	for ( std::vector<sQ3BSPFace*>::const_iterator it = rArray.begin(); it != rArray.end(); 
		++it )
	{
		sQ3BSPFace *pQ3BSPFace = *it;
		if ( pQ3BSPFace->iType == 1 || pQ3BSPFace->iType == 3 )
		{
			Q3BSP::sQ3BSPFace *pQ3BSPFace = *it;
			ai_assert( NULL != pQ3BSPFace );
			numVerts += pQ3BSPFace->iNumOfMeshVerts;
		}
	}

	return numVerts;
}

// ------------------------------------------------------------------------------------------------
size_t Q3BSPFileImporter::countFaces( const std::vector<Q3BSP::sQ3BSPFace*> &rArray ) const
{
	size_t numFaces = 0;
	for ( std::vector<sQ3BSPFace*>::const_iterator it = rArray.begin(); it != rArray.end(); 
		++it )
	{
		Q3BSP::sQ3BSPFace *pQ3BSPFace = *it;
		if ( pQ3BSPFace->iNumOfMeshVerts > 0)
		{
			numFaces++;
		}
	}

	return numFaces;
}

// ------------------------------------------------------------------------------------------------

} // Namespace Assimp

#endif // ASSIMP_BUILD_NO_Q3BSP_IMPORTER
