/** Implementation of the Collada loader */
/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

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
---------------------------------------------------------------------------
*/

#include "AssimpPCH.h"
#include "../include/aiAnim.h"
#include "ColladaLoader.h"
#include "ColladaParser.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
ColladaLoader::ColladaLoader()
{
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
ColladaLoader::~ColladaLoader()
{
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool ColladaLoader::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
{
	// check file extension 
	std::string::size_type pos = pFile.find_last_of('.');
	// no file extension - can't read
	if( pos == std::string::npos)
		return false;
	std::string extension = pFile.substr( pos);
	for( std::string::iterator it = extension.begin(); it != extension.end(); ++it)
		*it = tolower( *it);

	if( extension == ".dae")
		return true;

	return false;
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void ColladaLoader::InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler)
{
	mFileName = pFile;

	// parse the input file
	ColladaParser parser( pFile);

	// build the node hierarchy from it
	pScene->mRootNode = BuildHierarchy( parser, parser.mRootNode);

	// Convert to Z_UP, if different orientation
	if( parser.mUpDirection == ColladaParser::UP_X)
		pScene->mRootNode->mTransformation *= aiMatrix4x4( 
			 0, -1,  0,  0, 
			 0,  0, -1,  0,
			 1,  0,  0,  0,
			 0,  0,  0,  1);
	else if( parser.mUpDirection == ColladaParser::UP_Y)
		pScene->mRootNode->mTransformation *= aiMatrix4x4( 
			 1,  0,  0,  0, 
			 0,  0, -1,  0,
			 0,  1,  0,  0,
			 0,  0,  0,  1);


	// store all meshes
	StoreSceneMeshes( pScene);

	// create dummy material
	Assimp::MaterialHelper* mat = new Assimp::MaterialHelper;
	aiString name( std::string( "dummy"));
	mat->AddProperty( &name, AI_MATKEY_NAME);

	int shadeMode = aiShadingMode_Phong;
	mat->AddProperty<int>( &shadeMode, 1, AI_MATKEY_SHADING_MODEL);
	aiColor4D colAmbient( 0.2f, 0.2f, 0.2f, 1.0f), colDiffuse( 0.8f, 0.8f, 0.8f, 1.0f), colSpecular( 0.5f, 0.5f, 0.5f, 0.5f);
	mat->AddProperty( &colAmbient, 1, AI_MATKEY_COLOR_AMBIENT);
	mat->AddProperty( &colDiffuse, 1, AI_MATKEY_COLOR_DIFFUSE);
	mat->AddProperty( &colSpecular, 1, AI_MATKEY_COLOR_SPECULAR);
	float specExp = 5.0f;
	mat->AddProperty( &specExp, 1, AI_MATKEY_SHININESS);
	pScene->mNumMaterials = 1;
	pScene->mMaterials = new aiMaterial*[1];
	pScene->mMaterials[0] = mat;
}

// ------------------------------------------------------------------------------------------------
// Recursively constructs a scene node for the given parser node and returns it.
aiNode* ColladaLoader::BuildHierarchy( const ColladaParser& pParser, const Collada::Node* pNode)
{
	// create a node for it
	aiNode* node = new aiNode( pNode->mName);
	
	// calculate the transformation matrix for it
	node->mTransformation = pParser.CalculateResultTransform( pNode->mTransforms);

	// add children
	node->mNumChildren = pNode->mChildren.size();
	node->mChildren = new aiNode*[node->mNumChildren];
	for( unsigned int a = 0; a < pNode->mChildren.size(); a++)
	{
		node->mChildren[a] = BuildHierarchy( pParser, pNode->mChildren[a]);
		node->mChildren[a]->mParent = node;
	}

	// construct meshes
	BuildMeshesForNode( pParser, pNode, node);

	return node;
}

// ------------------------------------------------------------------------------------------------
// Builds meshes for the given node and references them
void ColladaLoader::BuildMeshesForNode( const ColladaParser& pParser, const Collada::Node* pNode, aiNode* pTarget)
{
	// accumulated mesh references by this node
	std::vector<size_t> newMeshRefs;

	// for the moment we simply ignore all material tags and transfer the meshes one by one
	BOOST_FOREACH( const Collada::MeshInstance& mid, pNode->mMeshes)
	{
		// find the referred mesh
		ColladaParser::MeshLibrary::const_iterator srcMeshIt = pParser.mMeshLibrary.find( mid.mMesh);
		if( srcMeshIt == pParser.mMeshLibrary.end())
		{
			DefaultLogger::get()->warn( boost::str( boost::format( "Unable to find geometry for ID \"%s\". Skipping.") % mid.mMesh));
			continue;
		}

		// if we already have the mesh at the library, just add its index to the node's array
		std::map<std::string, size_t>::const_iterator dstMeshIt = mMeshIndexbyID.find( mid.mMesh);
		if( dstMeshIt != mMeshIndexbyID.end())
		{
			newMeshRefs.push_back( dstMeshIt->second);
		} else
		{
			// else we have to add the mesh to the collection and store its newly assigned index at the node
			aiMesh* dstMesh = new aiMesh;
			const Collada::Mesh* srcMesh = srcMeshIt->second;

			// copy positions
			dstMesh->mNumVertices = srcMesh->mPositions.size();
			dstMesh->mVertices = new aiVector3D[dstMesh->mNumVertices];
			std::copy( srcMesh->mPositions.begin(), srcMesh->mPositions.end(), dstMesh->mVertices);

			// normals, if given. HACK: (thom) Due to the fucking Collada spec we never know if we have the same
			// number of normals as there are positions. So we also ignore any vertex attribute if it has a different count
			if( srcMesh->mNormals.size() == dstMesh->mNumVertices)
			{
				dstMesh->mNormals = new aiVector3D[dstMesh->mNumVertices];
				std::copy( srcMesh->mNormals.begin(), srcMesh->mNormals.end(), dstMesh->mNormals);
			}

			// same for texturecoords, as many as we have
			for( size_t a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; a++)
			{
				if( srcMesh->mTexCoords[a].size() == dstMesh->mNumVertices)
				{
					dstMesh->mTextureCoords[a] = new aiVector3D[dstMesh->mNumVertices];
					for( size_t b = 0; b < dstMesh->mNumVertices; ++b)
						dstMesh->mTextureCoords[a][b].Set( srcMesh->mTexCoords[a][b].x, srcMesh->mTexCoords[a][b].y, 0.0f);
					dstMesh->mNumUVComponents[a] = 2;
				}
			}

			// same for vertex colors, as many as we have
			for( size_t a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; a++)
			{
				if( srcMesh->mColors[a].size() == dstMesh->mNumVertices)
				{
					dstMesh->mColors[a] = new aiColor4D[dstMesh->mNumVertices];
					std::copy( srcMesh->mColors[a].begin(), srcMesh->mColors[a].end(), dstMesh->mColors[a]);
				}
			}

			// create faces. Due to the fact that each face uses unique vertices, we can simply count up on each vertex
			size_t vertex = 0;
			dstMesh->mNumFaces = srcMesh->mFaceSize.size();
			dstMesh->mFaces = new aiFace[dstMesh->mNumFaces];
			for( size_t a = 0; a < dstMesh->mNumFaces; ++a)
			{
				size_t s = srcMesh->mFaceSize[a];
				aiFace& face = dstMesh->mFaces[a];
				face.mNumIndices = s;
				face.mIndices = new unsigned int[s];
				for( size_t b = 0; b < s; ++b)
					face.mIndices[b] = vertex++;
			}

			// store the mesh, and store its new index in the node
			newMeshRefs.push_back( mMeshes.size());
			mMeshes.push_back( dstMesh);
		}
	}

	// now place all mesh references we gathered in the target node
	pTarget->mNumMeshes = newMeshRefs.size();
	if( newMeshRefs.size())
	{
		pTarget->mMeshes = new unsigned int[pTarget->mNumMeshes];
		std::copy( newMeshRefs.begin(), newMeshRefs.end(), pTarget->mMeshes);
	}
}

// ------------------------------------------------------------------------------------------------
// Stores all meshes in the given scene
void ColladaLoader::StoreSceneMeshes( aiScene* pScene)
{
	pScene->mNumMeshes = mMeshes.size();
	if( mMeshes.size() > 0)
	{
		pScene->mMeshes = new aiMesh*[mMeshes.size()];
		std::copy( mMeshes.begin(), mMeshes.end(), pScene->mMeshes);
	}
}
