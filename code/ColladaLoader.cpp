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

/** @file Implementation of the Collada loader */

#include "AssimpPCH.h"
#include "../include/aiAnim.h"
#include "ColladaLoader.h"
#include "ColladaParser.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
ColladaLoader::ColladaLoader()
{}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
ColladaLoader::~ColladaLoader()
{}

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

	// XML - too generic, we need to open the file and search for typical keywords
	if( extension == ".xml")	{
		/*  If CanRead() is called in order to check whether we
		 *  support a specific file extension in general pIOHandler
		 *  might be NULL and it's our duty to return true here.
		 */
		if (!pIOHandler)return true;
		const char* tokens[] = {"collada"};
		return SearchFileHeaderForToken(pIOHandler,pFile,tokens,1);
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void ColladaLoader::InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler)
{
	mFileName = pFile;

	// parse the input file
	ColladaParser parser( pFile);

	if( !parser.mRootNode)
		throw new ImportErrorException( "File came out empty. Something is wrong here.");

	// create the materials first, for the meshes to find
	BuildMaterials( parser, pScene);

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

	// add a mesh for each subgroup in each collada mesh
	BOOST_FOREACH( const Collada::MeshInstance& mid, pNode->mMeshes)
	{
		// find the referred mesh
		ColladaParser::MeshLibrary::const_iterator srcMeshIt = pParser.mMeshLibrary.find( mid.mMesh);
		if( srcMeshIt == pParser.mMeshLibrary.end())
		{
			DefaultLogger::get()->warn( boost::str( boost::format( "Unable to find geometry for ID \"%s\". Skipping.") % mid.mMesh));
			continue;
		}
		const Collada::Mesh* srcMesh = srcMeshIt->second;

		// build a mesh for each of its subgroups
		size_t vertexStart = 0, faceStart = 0;
		for( size_t sm = 0; sm < srcMesh->mSubMeshes.size(); ++sm)
		{
			const Collada::SubMesh& submesh = srcMesh->mSubMeshes[sm];
      if( submesh.mNumFaces == 0)
        continue;

			// find material assigned to this submesh
			std::map<std::string, std::string>::const_iterator meshMatIt = mid.mMaterials.find( submesh.mMaterial);
			std::string meshMaterial;
			if( meshMatIt != mid.mMaterials.end())
				meshMaterial = meshMatIt->second;
			else
				DefaultLogger::get()->warn( boost::str( boost::format( "No material specified for subgroup \"%s\" in geometry \"%s\".") % submesh.mMaterial % mid.mMesh));

			// built lookup index of the Mesh-Submesh-Material combination
			ColladaMeshIndex index( mid.mMesh, sm, meshMaterial);

			// if we already have the mesh at the library, just add its index to the node's array
			std::map<ColladaMeshIndex, size_t>::const_iterator dstMeshIt = mMeshIndexByID.find( index);
			if( dstMeshIt != mMeshIndexByID.end())
			{
				newMeshRefs.push_back( dstMeshIt->second);
			} else
			{
				// else we have to add the mesh to the collection and store its newly assigned index at the node
				aiMesh* dstMesh = new aiMesh;

				// count the vertices addressed by its faces
				size_t numVertices = 
					std::accumulate( srcMesh->mFaceSize.begin() + faceStart, srcMesh->mFaceSize.begin() + faceStart + submesh.mNumFaces, 0);

				// copy positions
				dstMesh->mNumVertices = numVertices;
				dstMesh->mVertices = new aiVector3D[numVertices];
				std::copy( srcMesh->mPositions.begin() + vertexStart, srcMesh->mPositions.begin() + vertexStart + numVertices, dstMesh->mVertices);

				// normals, if given. HACK: (thom) Due to the fucking Collada spec we never know if we have the same
				// number of normals as there are positions. So we also ignore any vertex attribute if it has a different count
				if( srcMesh->mNormals.size() == srcMesh->mPositions.size())
				{
					dstMesh->mNormals = new aiVector3D[numVertices];
					std::copy( srcMesh->mNormals.begin() + vertexStart, srcMesh->mNormals.begin() + vertexStart + numVertices, dstMesh->mNormals);
				}

				// same for texturecoords, as many as we have
				for( size_t a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; a++)
				{
					if( srcMesh->mTexCoords[a].size() == srcMesh->mPositions.size())
					{
						dstMesh->mTextureCoords[a] = new aiVector3D[numVertices];
						for( size_t b = 0; b < numVertices; ++b)
							dstMesh->mTextureCoords[a][b].Set( srcMesh->mTexCoords[a][vertexStart+b].x, srcMesh->mTexCoords[a][vertexStart+b].y, 0.0f);
						dstMesh->mNumUVComponents[a] = 2;
					}
				}

				// same for vertex colors, as many as we have
				for( size_t a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; a++)
				{
					if( srcMesh->mColors[a].size() == srcMesh->mPositions.size())
					{
						dstMesh->mColors[a] = new aiColor4D[numVertices];
						std::copy( srcMesh->mColors[a].begin() + vertexStart, srcMesh->mColors[a].begin() + vertexStart + numVertices, dstMesh->mColors[a]);
					}
				}

				// create faces. Due to the fact that each face uses unique vertices, we can simply count up on each vertex
				size_t vertex = 0;
				dstMesh->mNumFaces = submesh.mNumFaces;
				dstMesh->mFaces = new aiFace[dstMesh->mNumFaces];
				for( size_t a = 0; a < dstMesh->mNumFaces; ++a)
				{
					size_t s = srcMesh->mFaceSize[ faceStart + a];
					aiFace& face = dstMesh->mFaces[a];
					face.mNumIndices = s;
					face.mIndices = new unsigned int[s];
					for( size_t b = 0; b < s; ++b)
						face.mIndices[b] = vertex++;
				}

				// store the mesh, and store its new index in the node
				newMeshRefs.push_back( mMeshes.size());
				mMeshIndexByID[index] = mMeshes.size();
				mMeshes.push_back( dstMesh);
				vertexStart += numVertices; faceStart += submesh.mNumFaces;

				// assign the material index
				std::map<std::string, size_t>::const_iterator matIt = mMaterialIndexByName.find( meshMaterial);
				if( matIt != mMaterialIndexByName.end())
					dstMesh->mMaterialIndex = matIt->second;
				else
					dstMesh->mMaterialIndex = 0;
			}
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

// ------------------------------------------------------------------------------------------------
// Constructs materials from the collada material definitions
void ColladaLoader::BuildMaterials( const ColladaParser& pParser, aiScene* pScene)
{
	std::vector<aiMaterial*> newMats;

	for( ColladaParser::MaterialLibrary::const_iterator matIt = pParser.mMaterialLibrary.begin(); matIt != pParser.mMaterialLibrary.end(); ++matIt)
	{
		const Collada::Material& material = matIt->second;
		// a material is only a reference to an effect
		ColladaParser::EffectLibrary::const_iterator effIt = pParser.mEffectLibrary.find( material.mEffect);
		if( effIt == pParser.mEffectLibrary.end())
			continue;
		const Collada::Effect& effect = effIt->second;

		// create material
		Assimp::MaterialHelper* mat = new Assimp::MaterialHelper;
		aiString name( matIt->first);
		mat->AddProperty( &name, AI_MATKEY_NAME);

		int shadeMode;
		switch( effect.mShadeType)
		{
			case Collada::Shade_Constant: shadeMode = aiShadingMode_NoShading; break;
			case Collada::Shade_Lambert: shadeMode = aiShadingMode_Gouraud; break;
			case Collada::Shade_Blinn: shadeMode = aiShadingMode_Blinn; break;
			default: shadeMode = aiShadingMode_Phong; break;
		}
		mat->AddProperty<int>( &shadeMode, 1, AI_MATKEY_SHADING_MODEL);

		mat->AddProperty( &effect.mAmbient, 1, AI_MATKEY_COLOR_AMBIENT);
		mat->AddProperty( &effect.mDiffuse, 1, AI_MATKEY_COLOR_DIFFUSE);
		mat->AddProperty( &effect.mSpecular, 1, AI_MATKEY_COLOR_SPECULAR);
		mat->AddProperty( &effect.mEmissive, 1, AI_MATKEY_COLOR_EMISSIVE);
		mat->AddProperty( &effect.mShininess, 1, AI_MATKEY_SHININESS);
		mat->AddProperty( &effect.mRefractIndex, 1, AI_MATKEY_REFRACTI);
		
		// add textures, if given
		if( !effect.mTexAmbient.empty())
			mat->AddProperty( &FindFilenameForEffectTexture( pParser, effect, effect.mTexAmbient), AI_MATKEY_TEXTURE_AMBIENT( 0));
		if( !effect.mTexDiffuse.empty())
			mat->AddProperty( &FindFilenameForEffectTexture( pParser, effect, effect.mTexDiffuse), AI_MATKEY_TEXTURE_DIFFUSE( 0));
		if( !effect.mTexEmissive.empty())
			mat->AddProperty( &FindFilenameForEffectTexture( pParser, effect, effect.mTexEmissive), AI_MATKEY_TEXTURE_EMISSIVE( 0));
		if( !effect.mTexSpecular.empty())
			mat->AddProperty( &FindFilenameForEffectTexture( pParser, effect, effect.mTexSpecular), AI_MATKEY_TEXTURE_SPECULAR( 0));

		// store the material
		mMaterialIndexByName[matIt->first] = newMats.size();
		newMats.push_back( mat);
	}

	// store a dummy material if none were given
	if( newMats.size() == 0)
	{
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
	}

	// store the materials in the scene
	pScene->mNumMaterials = newMats.size();
	pScene->mMaterials = new aiMaterial*[pScene->mNumMaterials];
	std::copy( newMats.begin(), newMats.end(), pScene->mMaterials);
}

// ------------------------------------------------------------------------------------------------
// Resolves the texture name for the given effect texture entry
const aiString& ColladaLoader::FindFilenameForEffectTexture( const ColladaParser& pParser, const Collada::Effect& pEffect, const std::string& pName)
{
	// recurse through the param references until we end up at an image
	std::string name = pName;
	while( 1)
	{
		// the given string is a param entry. Find it
		Collada::Effect::ParamLibrary::const_iterator it = pEffect.mParams.find( name);
		// if not found, we're at the end of the recursion. The resulting string should be the image ID
		if( it == pEffect.mParams.end())
			break;

		// else recurse on
		name = it->second.mReference;
	}

	// find the image referred by this name in the image library of the scene
	ColladaParser::ImageLibrary::const_iterator imIt = pParser.mImageLibrary.find( name);
	if( imIt == pParser.mImageLibrary.end())
		throw new ImportErrorException( boost::str( boost::format( "Unable to resolve effect texture entry \"%s\", ended up at ID \"%s\".") % pName % name));

	static aiString result;
	result.Set( imIt->second.mFileName );
	ConvertPath(result);
	return result;
}

// ------------------------------------------------------------------------------------------------
// Convert a path read from a collada file to the usual representation
void ColladaLoader::ConvertPath (aiString& ss)
{
	// TODO: collada spec, p 22. Handle URI correctly.
	// For the moment we're just stripping the file:// away to make it work.
	// Windoes doesn't seem to be able to find stuff like
	// 'file://..\LWO\LWO2\MappingModes\earthSpherical.jpg'
	if (0 == ::strncmp(ss.data,"file://",7)) 
	{
		ss.length -= 7;
		::memmove(ss.data,ss.data+7,ss.length);
		ss.data[ss.length] = '\0';
	}
}
