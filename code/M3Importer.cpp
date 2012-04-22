/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team
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

#include "AssimpPCH.h"
#ifndef ASSIMP_BUILD_NO_M3_IMPORTER

#include "M3Importer.h"
#include <sstream>

namespace Assimp {
namespace M3 {

static const aiImporterDesc desc = {
	"StarCraft M3 Importer",
	"",
	"",
	"",
	aiImporterFlags_SupportBinaryFlavour,
	0,
	0,
	0,
	0,
	"m3"
};

// ------------------------------------------------------------------------------------------------
//	Constructor.
M3Importer::M3Importer() :
	m_pHead( NULL ),
	m_pRefs( NULL ),
	m_Buffer()
{
	// empty
}

// ------------------------------------------------------------------------------------------------
//	Destructor.
M3Importer::~M3Importer()
{
	m_pHead = NULL;
	m_pRefs = NULL;
}

// ------------------------------------------------------------------------------------------------
//	Check for readable file format.
bool M3Importer::CanRead( const std::string &rFile, IOSystem* /*pIOHandler*/, bool checkSig ) const
{
	if ( !checkSig ) {
		return SimpleExtensionCheck( rFile, "m3" );
	}

	return false;
}

// ------------------------------------------------------------------------------------------------
const aiImporterDesc* M3Importer::GetInfo () const
{
	return &desc;
}

// ------------------------------------------------------------------------------------------------
void M3Importer::InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler )
{
	ai_assert( !pFile.empty() );

	const std::string mode = "rb";
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile, mode ) );
	if ( NULL == file.get() ) {
		throw DeadlyImportError( "Failed to open file " + pFile + ".");
	}

	// Get the file-size and validate it, throwing an exception when it fails
	const size_t filesize = file->FileSize();
	if( filesize  < 1 ) {
		throw DeadlyImportError( "M3-file is too small.");
	}

	m_Buffer.resize( filesize );
	size_t readsize = file->Read( &m_Buffer[ 0 ], sizeof( unsigned char ), filesize );
	ai_assert( readsize == filesize );

	m_pHead = reinterpret_cast<MD33*>( &m_Buffer[ 0 ] );
	m_pRefs = reinterpret_cast<ReferenceEntry*>( &m_Buffer[ 0 ] + m_pHead->ofsRefs );

	MODL20* pMODL20( NULL );
	MODL23* pMODL23( NULL );

	VertexExt* pVerts1( NULL );
	Vertex* pVerts2( NULL );

	DIV *pViews( NULL );
	Region* regions( NULL );
	uint16* faces( NULL );

	uint32 nVertices = 0;

	bool ok = true;
	switch( m_pRefs[ m_pHead->MODL.ref ].type )	{
	case 20:
		pMODL20 = GetEntries<MODL20>( m_pHead->MODL );
		if ( ( pMODL20->flags & 0x20000) != 0 ) { // Has vertices
			if( (pMODL20->flags & 0x40000) != 0 ) { // Has extra 4 byte
				pVerts1 = GetEntries<VertexExt>( pMODL20->vertexData );
				nVertices = pMODL20->vertexData.nEntries/sizeof(VertexExt);
			}
			else {
				pVerts2 = GetEntries<Vertex>( pMODL20->vertexData );
				nVertices = pMODL20->vertexData.nEntries / sizeof( Vertex );
			}
		}
		pViews = GetEntries<DIV>( pMODL20->views );
		break;

	case 23:
		pMODL23 = GetEntries<MODL23>(m_pHead->MODL );
		if( (pMODL23->flags & 0x20000) != 0 ) { // Has vertices
			if( (pMODL23->flags & 0x40000) != 0 ) { // Has extra 4 byte
				pVerts1 = GetEntries<VertexExt>( pMODL23->vertexData );
				nVertices = pMODL23->vertexData.nEntries/sizeof( VertexExt );
			}
			else {
				pVerts2 = GetEntries<Vertex>( pMODL23->vertexData );
				nVertices = pMODL23->vertexData.nEntries/sizeof( Vertex );
			}
		}
		pViews = GetEntries<DIV>( pMODL23->views );
		break;

	default:
		ok = false;
		break;
	}
	
	// Everything ok, if not throw an exception
	if ( !ok ) {
		throw DeadlyImportError( "Failed to open file " + pFile + ".");
	}

	// Get all region data
	regions = GetEntries<Region>( pViews->regions );
	
	// Get the face data
	faces = GetEntries<uint16>( pViews->faces );

	// Convert the vertices
	std::vector<aiVector3D> vertices;
	vertices.resize( nVertices );
	unsigned int offset = 0;
	for ( unsigned int i = 0; i < nVertices; i++ ) {
		if ( pVerts1 ) {
			vertices[ offset ].Set( pVerts1[ i ].pos.x, pVerts1[ i ].pos.y, pVerts1[ i ].pos.z );
			++offset;
		}

		if ( pVerts2 ) {
			vertices[ offset ].Set( pVerts2[ i ].pos.x, pVerts2[ i ].pos.y, pVerts2[ i ].pos.z );
			++offset;
		}
	}

	// Write the UV coordinates
	offset = 0;
	std::vector<aiVector3D> uvCoords;
	uvCoords.resize( nVertices );
	for( unsigned int i = 0; i < nVertices; ++i ) {
		if( pVerts1 ) {
			float u = (float) pVerts1[ i ].uv[ 0 ] / 2048;
			float v = (float) pVerts1[ i ].uv[ 1 ] / 2048;
			uvCoords[ offset ].Set( u, v, 0.0f );
			++offset;
		}

		if( pVerts2 ) {
			float u = (float) pVerts2[ i ].uv[ 0 ] / 2048;
			float v = (float) pVerts2[ i ].uv[ 1 ] / 2048;
			uvCoords[ offset ].Set( u, v, 0.0f );
			++offset;
		}
	}

	// Compute the normals  
	std::vector<aiVector3D> normals;
	normals.resize( nVertices );
	float w = 0.0f;
	Vec3D norm;
	offset = 0;
	for( unsigned int i = 0; i < nVertices; i++ ) {
		w = 0.0f;
		if( pVerts1 ) {
			norm.x = (float) 2*pVerts1[ i ].normal[ 0 ]/255.0f - 1;
			norm.y = (float) 2*pVerts1[ i ].normal[ 1 ]/255.0f - 1;
			norm.z = (float) 2*pVerts1[ i ].normal[ 2 ]/255.0f - 1;
			w = (float) pVerts1[ i ].normal[ 3 ]/255.0f;
		}

		if( pVerts2 ) {
			norm.x = (float) 2*pVerts2[ i ].normal[ 0 ]/255.0f - 1;
			norm.y = (float) 2*pVerts2[ i ].normal[ 1 ]/255.0f - 1;
			norm.z = (float) 2*pVerts2[ i ].normal[ 2 ]/255.0f - 1;
			w = (float) pVerts2[ i ].normal[ 3 ] / 255.0f;
		}

		if ( w ) {
			const float invW = 1.0f / w;
			norm.x = norm.x * invW;
			norm.y = norm.y * invW;
			norm.z = norm.z * invW;
			normals[ offset ].Set( norm.x, norm.y, norm.z );
			++offset;
		}
	}

	// Convert the data into the assimp specific data structures
	convertToAssimp( pFile, pScene, pViews, regions, faces, vertices, uvCoords, normals );
}

// ------------------------------------------------------------------------------------------------
//
void M3Importer::convertToAssimp( const std::string& pFile, aiScene* pScene, DIV *pViews, 
								 Region *pRegions, uint16 *pFaces, 
								 const std::vector<aiVector3D> &vertices,
								 const std::vector<aiVector3D> &uvCoords,
								 const std::vector<aiVector3D> &normals )
{
	std::vector<aiMesh*> MeshArray;

	// Create the root node
	pScene->mRootNode = createNode( NULL );
	
	// Set the name of the scene
	pScene->mRootNode->mName.Set( pFile );

	aiNode *pRootNode = pScene->mRootNode;
	aiNode *pCurrentNode = NULL;

	// Lets create the nodes
	pRootNode->mNumChildren = pViews->regions.nEntries;
	if ( pRootNode->mNumChildren > 0 ) {
		pRootNode->mChildren = new aiNode*[ pRootNode->mNumChildren ];
	}

	for ( unsigned int i=0; i<pRootNode->mNumChildren; ++i ) {
		//pRegions[ i ].
		// Create a new node
		pCurrentNode = createNode( pRootNode );
		std::stringstream stream;
		stream << "Node_" << i;
		pCurrentNode->mName.Set( stream.str().c_str() );
		pRootNode->mChildren[ i ] = pCurrentNode;
		
		// Loop over the faces of the nodes
		unsigned int numFaces = ( ( pRegions[ i ].ofsIndices + pRegions[ i ].nIndices ) -  pRegions[ i ].ofsIndices ) / 3;
		aiMesh *pMesh = new aiMesh;
		MeshArray.push_back( pMesh );
		pMesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

		pMesh->mNumFaces = numFaces;
		pMesh->mFaces = new aiFace[ pMesh->mNumFaces ];
		aiFace *pCurrentFace = NULL;
		unsigned int faceIdx = 0;
		for ( unsigned int j = pRegions[ i ].ofsIndices; j < ( pRegions[ i ].ofsIndices + pRegions[ i ].nIndices ); j += 3 ) {
			pCurrentFace = &( pMesh->mFaces[ faceIdx ] );
			faceIdx++;
			pCurrentFace->mNumIndices = 3;
			pCurrentFace->mIndices = new unsigned int[ 3 ];
			pCurrentFace->mIndices[ 0 ] = pFaces[ j   ];
			pCurrentFace->mIndices[ 1 ] = pFaces[ j+1 ];
			pCurrentFace->mIndices[ 2 ] = pFaces[ j+2 ];
		}
		// Now we can create the vertex data itself
		pCurrentNode->mNumMeshes = 1;
		pCurrentNode->mMeshes = new unsigned int[ 1 ];
		const unsigned int meshIdx = MeshArray.size() - 1;
		pCurrentNode->mMeshes[ 0 ] = meshIdx;
		createVertexData( pMesh, vertices, uvCoords, normals );
	}

	// Copy the meshes into the scene
	pScene->mNumMeshes = MeshArray.size();
	pScene->mMeshes = new aiMesh*[ MeshArray.size() ];
	unsigned int pos = 0;
	for ( std::vector<aiMesh*>::iterator it = MeshArray.begin(); it != MeshArray.end(); ++it ) {
		pScene->mMeshes[ pos ] = *it;
		++pos;
	}
}

// ------------------------------------------------------------------------------------------------
//
void M3Importer::createVertexData( aiMesh *pMesh, const std::vector<aiVector3D> &vertices,
								  const std::vector<aiVector3D> &uvCoords,
								  const std::vector<aiVector3D> &normals )
{
	pMesh->mNumVertices = pMesh->mNumFaces * 3;
	pMesh->mVertices = new aiVector3D[ pMesh->mNumVertices ];
	pMesh->mNumUVComponents[ 0 ] = 2;
	pMesh->mTextureCoords[ 0 ] = new aiVector3D[ pMesh->mNumVertices ];
	pMesh->mNormals = new aiVector3D[ pMesh->mNumVertices ];
	unsigned int pos = 0;
	for ( unsigned int currentFace = 0; currentFace < pMesh->mNumFaces; currentFace++ )	{
		aiFace *pFace = &( pMesh->mFaces[ currentFace ] );
		for ( unsigned int currentIdx=0; currentIdx<pFace->mNumIndices; currentIdx++ ) {
			const unsigned int idx = pFace->mIndices[ currentIdx ];
			if ( vertices.size() > idx ) {
				pMesh->mVertices[ pos ] = vertices[ idx ];
				pMesh->mNormals[ pos ] = normals[ idx ];
				pMesh->mTextureCoords[ 0 ]->x = uvCoords[ idx ].x;
				pMesh->mTextureCoords[ 0 ]->y = uvCoords[ idx ].y;
				pFace->mIndices[ currentIdx ] = pos;
				pos++;
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
//
aiNode *M3Importer::createNode( aiNode *pParent )
{
	aiNode *pNode = new aiNode;
	if ( pParent )
		pNode->mParent = pParent;
	else
		pNode->mParent = NULL;

	return pNode;
}

// ------------------------------------------------------------------------------------------------

} // Namespace M3
} // Namespace Assimp

#endif // ASSIMP_BUILD_NO_M3_IMPORTER
