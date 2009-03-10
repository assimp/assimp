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
#ifndef ASSIMP_BUILD_NO_DAE_IMPORTER

#include "../include/aiAnim.h"
#include "ColladaLoader.h"
#include "ColladaParser.h"

#include "fast_atof.h"
#include "ParsingUtils.h"

#include "time.h"

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
bool ColladaLoader::CanRead( const std::string& pFile, IOSystem* pIOHandler, bool checkSig) const
{
	// check file extension 
	std::string extension = GetExtension(pFile);
	
	if( extension == "dae")
		return true;

	// XML - too generic, we need to open the file and search for typical keywords
	if( extension == "xml" || !extension.length() || checkSig)	{
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
// Get file extension list
void ColladaLoader::GetExtensionList( std::string& append)
{
	append.append("*.dae");
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void ColladaLoader::InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler)
{
	mFileName = pFile;

	// clean all member arrays - just for safety, it should work even if we did not
	mMeshIndexByID.clear();
	mMaterialIndexByName.clear();
	mMeshes.clear();
	newMats.clear();
	mLights.clear();
	mCameras.clear();
	mTextures.clear();

	// parse the input file
	ColladaParser parser( pIOHandler, pFile);

	if( !parser.mRootNode)
		throw new ImportErrorException( "Collada: File came out empty. Something is wrong here.");

	// reserve some storage to avoid unnecessary reallocs
	newMats.reserve(parser.mMaterialLibrary.size()*2);
	mMeshes.reserve(parser.mMeshLibrary.size()*2);

	mCameras.reserve(parser.mCameraLibrary.size());
	mLights.reserve(parser.mLightLibrary.size());

	// create the materials first, for the meshes to find
	BuildMaterials( parser, pScene);

	// build the node hierarchy from it
	pScene->mRootNode = BuildHierarchy( parser, parser.mRootNode);

	// ... then fill the materials with the now adjusted settings
	FillMaterials(parser, pScene);

	// Convert to Y_UP, if different orientation
	if( parser.mUpDirection == ColladaParser::UP_X)
		pScene->mRootNode->mTransformation *= aiMatrix4x4( 
			 0, -1,  0,  0, 
			 1,  0,  0,  0,
			 0,  0,  1,  0,
			 0,  0,  0,  1);
	else if( parser.mUpDirection == ColladaParser::UP_Z)
		pScene->mRootNode->mTransformation *= aiMatrix4x4( 
			 1,  0,  0,  0, 
			 0,  0,  1,  0,
			 0, -1,  0,  0,
			 0,  0,  0,  1);

	// store all meshes
	StoreSceneMeshes( pScene);

	// store all materials
	StoreSceneMaterials( pScene);

	// store all lights
	StoreSceneLights( pScene);

	// store all cameras
	StoreSceneCameras( pScene);
}

// ------------------------------------------------------------------------------------------------
// Recursively constructs a scene node for the given parser node and returns it.
aiNode* ColladaLoader::BuildHierarchy( const ColladaParser& pParser, const Collada::Node* pNode)
{
	// create a node for it
	aiNode* node = new aiNode();

	// now setup the name of the node. We take the name if not empty, otherwise the collada ID
	// FIX: Workaround for XSI calling the instanced visual scene 'untitled' by default.
	if (!pNode->mName.empty() && pNode->mName != "untitled")
		node->mName.Set(pNode->mName);
	else if (!pNode->mID.empty())
		node->mName.Set(pNode->mID);
	else
	{
		// No need to worry. Unnamed nodes are no problem at all, except
		// if cameras or lights need to be assigned to them.
		if (!pNode->mLights.empty() || !pNode->mCameras.empty()) {
	
			::strcpy(node->mName.data,"$ColladaAutoName$_");
			node->mName.length = 17 + ASSIMP_itoa10(node->mName.data+18,MAXLEN-18,(uint32_t)clock());
		}
	}
	
	// calculate the transformation matrix for it
	node->mTransformation = pParser.CalculateResultTransform( pNode->mTransforms);

	// now resolve node instances
	std::vector<Collada::Node*> instances;
	ResolveNodeInstances(pParser,pNode,instances);

	// add children. first the *real* ones
	node->mNumChildren = pNode->mChildren.size()+instances.size();
	node->mChildren = new aiNode*[node->mNumChildren];

	unsigned int a = 0;
	for(; a < pNode->mChildren.size(); a++)
	{
		node->mChildren[a] = BuildHierarchy( pParser, pNode->mChildren[a]);
		node->mChildren[a]->mParent = node;
	}

	// ... and finally the resolved node instances
	for(; a < node->mNumChildren; a++)
	{
		node->mChildren[a] = BuildHierarchy( pParser, instances[a-pNode->mChildren.size()]);
		node->mChildren[a]->mParent = node;
	}

	// construct meshes
	BuildMeshesForNode( pParser, pNode, node);

	// construct cameras
	BuildCamerasForNode(pParser, pNode, node);

	// construct lights
	BuildLightsForNode(pParser, pNode, node);
	return node;
}

// ------------------------------------------------------------------------------------------------
// Resolve node instances
void ColladaLoader::ResolveNodeInstances( const ColladaParser& pParser, const Collada::Node* pNode,
	std::vector<Collada::Node*>& resolved)
{
	// reserve enough storage
	resolved.reserve(pNode->mNodeInstances.size());

	// ... and iterate through all nodes to be instanced as children of pNode
	for (std::vector<Collada::NodeInstance>::const_iterator it = pNode->mNodeInstances.begin(),
		 end = pNode->mNodeInstances.end(); it != end; ++it)
	{
		// find the corresponding node in the library
		ColladaParser::NodeLibrary::const_iterator fnd = pParser.mNodeLibrary.find((*it).mNode);
		if (fnd == pParser.mNodeLibrary.end()) 
			DefaultLogger::get()->error("Collada: Unable to resolve reference to instanced node " + (*it).mNode);
		
		else {
			//	attach this node to the list of children
			resolved.push_back((*fnd).second);
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Resolve UV channels
void ColladaLoader::ApplyVertexToEffectSemanticMapping(Collada::Sampler& sampler,
	 const Collada::SemanticMappingTable& table)
{
	std::map<std::string, Collada::InputSemanticMapEntry>::const_iterator it = table.mMap.find(sampler.mUVChannel);
	if (it != table.mMap.end()) {
		if (it->second.mType != Collada::IT_Texcoord)
			DefaultLogger::get()->error("Collada: Unexpected effect input mapping");

		sampler.mUVId = it->second.mSet;
	}
}

// ------------------------------------------------------------------------------------------------
// Builds lights for the given node and references them
void ColladaLoader::BuildLightsForNode( const ColladaParser& pParser, const Collada::Node* pNode, aiNode* pTarget)
{
	BOOST_FOREACH( const Collada::LightInstance& lid, pNode->mLights)
	{
		// find the referred light
		ColladaParser::LightLibrary::const_iterator srcLightIt = pParser.mLightLibrary.find( lid.mLight);
		if( srcLightIt == pParser.mLightLibrary.end())
		{
			DefaultLogger::get()->warn("Collada: Unable to find light for ID \"" + lid.mLight + "\". Skipping.");
			continue;
		}
		const Collada::Light* srcLight = &srcLightIt->second;
		if (srcLight->mType == aiLightSource_AMBIENT) {
			DefaultLogger::get()->error("Collada: Skipping ambient light for the moment");
			continue;
		}
		
		// now fill our ai data structure
		aiLight* out = new aiLight();
		out->mName = pTarget->mName;
		out->mType = (aiLightSourceType)srcLight->mType;

		// collada lights point in -Z by default, rest is specified in node transform
		out->mDirection = aiVector3D(0.f,0.f,-1.f);

		out->mAttenuationConstant = srcLight->mAttConstant;
		out->mAttenuationLinear = srcLight->mAttLinear;
		out->mAttenuationQuadratic = srcLight->mAttQuadratic;

		// collada doesn't differenciate between these color types
		out->mColorDiffuse = out->mColorSpecular = out->mColorAmbient = srcLight->mColor*srcLight->mIntensity;

		// convert falloff angle and falloff exponent in our representation, if given
		if (out->mType == aiLightSource_SPOT) {
			
			out->mAngleInnerCone = AI_DEG_TO_RAD( srcLight->mFalloffAngle );

			// ... some extension magic. FUCKING COLLADA. 
			if (srcLight->mOuterAngle == 10e10f) 
			{
				// ... some deprecation magic. FUCKING FCOLLADA.
				if (srcLight->mPenumbraAngle == 10e10f) 
				{
					// Need to rely on falloff_exponent. I don't know how to interpret it, so I need to guess ....
					// ci - inner cone angle
					// co - outer cone angle
					// fe - falloff exponent
					// ld - spot direction - normalized
					// rd - ray direction - normalized
					//
					// Formula is:
					// 1. (cos(acos (ld dot rd) - ci))^fe == epsilon
					// 2. (ld dot rd) == cos(acos(epsilon^(1/fe)) + ci)
					// 3. co == acos (ld dot rd)
					// 4. co == acos(epsilon^(1/fe)) + ci)

					// epsilon chosen to be 0.1
					out->mAngleOuterCone = AI_DEG_TO_RAD (acos(pow(0.1f,1.f/srcLight->mFalloffExponent))+
						srcLight->mFalloffAngle);
				}
				else {
					out->mAngleOuterCone = out->mAngleInnerCone + AI_DEG_TO_RAD(  srcLight->mPenumbraAngle );
					if (out->mAngleOuterCone < out->mAngleInnerCone)
						std::swap(out->mAngleInnerCone,out->mAngleOuterCone);
				}
			}
			else out->mAngleOuterCone = AI_DEG_TO_RAD(  srcLight->mOuterAngle );
		}

		// add to light list
		mLights.push_back(out);
	}
}

// ------------------------------------------------------------------------------------------------
// Builds cameras for the given node and references them
void ColladaLoader::BuildCamerasForNode( const ColladaParser& pParser, const Collada::Node* pNode, aiNode* pTarget)
{
	BOOST_FOREACH( const Collada::CameraInstance& cid, pNode->mCameras)
	{
		// find the referred light
		ColladaParser::CameraLibrary::const_iterator srcCameraIt = pParser.mCameraLibrary.find( cid.mCamera);
		if( srcCameraIt == pParser.mCameraLibrary.end())
		{
			DefaultLogger::get()->warn("Collada: Unable to find camera for ID \"" + cid.mCamera + "\". Skipping.");
			continue;
		}
		const Collada::Camera* srcCamera = &srcCameraIt->second;

		// orthographic cameras not yet supported in Assimp
		if (srcCamera->mOrtho) {
			DefaultLogger::get()->warn("Collada: Orthographic cameras are not supported.");
		}

		// now fill our ai data structure
		aiCamera* out = new aiCamera();
		out->mName = pTarget->mName;

		// collada cameras point in -Z by default, rest is specified in node transform
		out->mLookAt = aiVector3D(0.f,0.f,-1.f);

		// near/far z is already ok
		out->mClipPlaneFar = srcCamera->mZFar;
		out->mClipPlaneNear = srcCamera->mZNear;

		// ... but for the rest some values are optional 
		// and we need to compute the others in any combination. FUCKING COLLADA.
		 if (srcCamera->mAspect != 10e10f)
			out->mAspect = srcCamera->mAspect;

		if (srcCamera->mHorFov != 10e10f) {
			out->mHorizontalFOV = srcCamera->mHorFov; 

			if (srcCamera->mVerFov != 10e10f && srcCamera->mAspect != 10e10f) {
				out->mAspect = srcCamera->mHorFov/srcCamera->mVerFov;
			}
		}
		else if (srcCamera->mAspect != 10e10f && srcCamera->mVerFov != 10e10f)	{
			out->mHorizontalFOV = srcCamera->mAspect*srcCamera->mVerFov;
		}

		// Collada uses degrees, we use radians
		out->mHorizontalFOV = AI_DEG_TO_RAD(out->mHorizontalFOV);

		// add to camera list
		mCameras.push_back(out);
	}
}

// ------------------------------------------------------------------------------------------------
// Builds meshes for the given node and references them
void ColladaLoader::BuildMeshesForNode( const ColladaParser& pParser, const Collada::Node* pNode, aiNode* pTarget)
{
	// accumulated mesh references by this node
	std::vector<size_t> newMeshRefs;
	newMeshRefs.reserve(pNode->mMeshes.size());

	// add a mesh for each subgroup in each collada mesh
	BOOST_FOREACH( const Collada::MeshInstance& mid, pNode->mMeshes)
	{
		// find the referred mesh
		ColladaParser::MeshLibrary::const_iterator srcMeshIt = pParser.mMeshLibrary.find( mid.mMesh);
		if( srcMeshIt == pParser.mMeshLibrary.end())
		{
			DefaultLogger::get()->warn( boost::str( boost::format( "Collada: Unable to find geometry for ID \"%s\". Skipping.") % mid.mMesh));
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
			std::map<std::string, Collada::SemanticMappingTable >::const_iterator meshMatIt = mid.mMaterials.find( submesh.mMaterial);

			const Collada::SemanticMappingTable* table;
			if( meshMatIt != mid.mMaterials.end())
				table = &meshMatIt->second;
			else {
				table = NULL;
				DefaultLogger::get()->warn( boost::str( boost::format( "Collada: No material specified for subgroup \"%s\" in geometry \"%s\".") % submesh.mMaterial % mid.mMesh));
			}
			const std::string& meshMaterial = table ? table->mMatName : "";

			// OK ... here the *real* fun starts ... we have the vertex-input-to-effect-semantic-table
			// given. The only mapping stuff which we do actually support is the UV channel.
			std::map<std::string, size_t>::const_iterator matIt = mMaterialIndexByName.find( meshMaterial);
			unsigned int matIdx;
			if( matIt != mMaterialIndexByName.end())
				matIdx = matIt->second;
			else
				matIdx = 0;

			if (table && !table->mMap.empty() ) {
				std::pair<Collada::Effect*, aiMaterial*>&  mat = newMats[matIdx];

				// Iterate through all texture channels assigned to the effect and
				// check whether we have mapping information for it.
				ApplyVertexToEffectSemanticMapping(mat.first->mTexDiffuse,    *table);
				ApplyVertexToEffectSemanticMapping(mat.first->mTexAmbient,    *table);
				ApplyVertexToEffectSemanticMapping(mat.first->mTexSpecular,   *table);
				ApplyVertexToEffectSemanticMapping(mat.first->mTexEmissive,   *table);
				ApplyVertexToEffectSemanticMapping(mat.first->mTexTransparent,*table);
				ApplyVertexToEffectSemanticMapping(mat.first->mTexBump,       *table);
			}

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
				const size_t numVertices = std::accumulate( srcMesh->mFaceSize.begin() + faceStart,
					srcMesh->mFaceSize.begin() + faceStart + submesh.mNumFaces, 0);

				// copy positions
				dstMesh->mNumVertices = numVertices;
				dstMesh->mVertices = new aiVector3D[numVertices];
				std::copy( srcMesh->mPositions.begin() + vertexStart, srcMesh->mPositions.begin() + 
					vertexStart + numVertices, dstMesh->mVertices);

				// normals, if given. HACK: (thom) Due to the fucking Collada spec we never 
				// know if we have the same number of normals as there are positions. So we 
				// also ignore any vertex attribute if it has a different count
				if( srcMesh->mNormals.size() == srcMesh->mPositions.size())
				{
					dstMesh->mNormals = new aiVector3D[numVertices];
					std::copy( srcMesh->mNormals.begin() + vertexStart, srcMesh->mNormals.begin() +
						vertexStart + numVertices, dstMesh->mNormals);
				}

				// tangents, if given. 
				if( srcMesh->mTangents.size() == srcMesh->mPositions.size())
				{
					dstMesh->mTangents = new aiVector3D[numVertices];
					std::copy( srcMesh->mTangents.begin() + vertexStart, srcMesh->mTangents.begin() + 
						vertexStart + numVertices, dstMesh->mTangents);
				}

				// bitangents, if given. 
				if( srcMesh->mBitangents.size() == srcMesh->mPositions.size())
				{
					dstMesh->mBitangents = new aiVector3D[numVertices];
					std::copy( srcMesh->mBitangents.begin() + vertexStart, srcMesh->mBitangents.begin() + 
						vertexStart + numVertices, dstMesh->mBitangents);
				}

				// same for texturecoords, as many as we have
				for( size_t a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; a++)
				{
					if( srcMesh->mTexCoords[a].size() == srcMesh->mPositions.size())
					{
						dstMesh->mTextureCoords[a] = new aiVector3D[numVertices];
						for( size_t b = 0; b < numVertices; ++b)
							dstMesh->mTextureCoords[a][b] = srcMesh->mTexCoords[a][vertexStart+b];
						
						dstMesh->mNumUVComponents[a] = srcMesh->mNumUVComponents[a];
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
				dstMesh->mMaterialIndex = matIdx;
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
	mMeshes.clear();
}

// ------------------------------------------------------------------------------------------------
// Stores all cameras in the given scene
void ColladaLoader::StoreSceneCameras( aiScene* pScene)
{
	pScene->mNumCameras = mCameras.size();
	if( mCameras.size() > 0)
	{
		pScene->mCameras = new aiCamera*[mCameras.size()];
		std::copy( mCameras.begin(), mCameras.end(), pScene->mCameras);
	}
	mCameras.clear();
}

// ------------------------------------------------------------------------------------------------
// Stores all lights in the given scene
void ColladaLoader::StoreSceneLights( aiScene* pScene)
{
	pScene->mNumLights = mLights.size();
	if( mLights.size() > 0)
	{
		pScene->mLights = new aiLight*[mLights.size()];
		std::copy( mLights.begin(), mLights.end(), pScene->mLights);
	}
	mLights.clear();
}

// ------------------------------------------------------------------------------------------------
// Stores all textures in the given scene
void ColladaLoader::StoreSceneTextures( aiScene* pScene)
{
	pScene->mNumTextures = mTextures.size();
	if( mTextures.size() > 0)
	{
		pScene->mTextures = new aiTexture*[mTextures.size()];
		std::copy( mTextures.begin(), mTextures.end(), pScene->mTextures);
	}
	mTextures.clear();
}

// ------------------------------------------------------------------------------------------------
// Stores all materials in the given scene
void ColladaLoader::StoreSceneMaterials( aiScene* pScene)
{
	pScene->mNumMaterials = newMats.size();
	
	pScene->mMaterials = new aiMaterial*[newMats.size()];
	for (unsigned int i = 0; i < newMats.size();++i)
		pScene->mMaterials[i] = newMats[i].second;

	newMats.clear();
}

// ------------------------------------------------------------------------------------------------
// Add a texture to a material structure
void ColladaLoader::AddTexture ( Assimp::MaterialHelper& mat, const ColladaParser& pParser,
	const Collada::Effect& effect,
	const Collada::Sampler& sampler,
	aiTextureType type, unsigned int idx)
{
	// first of all, basic file name
	mat.AddProperty( &FindFilenameForEffectTexture( pParser, effect, sampler.mName), 
		_AI_MATKEY_TEXTURE_BASE,type,idx);

	// mapping mode
	int map = aiTextureMapMode_Clamp;
	if (sampler.mWrapU)
		map = aiTextureMapMode_Wrap;
	if (sampler.mWrapU && sampler.mMirrorU)
		map = aiTextureMapMode_Mirror;

	mat.AddProperty( &map, 1, _AI_MATKEY_MAPPINGMODE_U_BASE, type, idx);

	map = aiTextureMapMode_Clamp;
	if (sampler.mWrapV)
		map = aiTextureMapMode_Wrap;
	if (sampler.mWrapV && sampler.mMirrorV)
		map = aiTextureMapMode_Mirror;

	mat.AddProperty( &map, 1, _AI_MATKEY_MAPPINGMODE_V_BASE, type, idx);

	// UV transformation
	mat.AddProperty(&sampler.mTransform, 1,
		_AI_MATKEY_UVTRANSFORM_BASE, type, idx);

	// Blend mode
	mat.AddProperty((int*)&sampler.mOp , 1,
		_AI_MATKEY_TEXBLEND_BASE, type, idx);

	// Blend factor
	mat.AddProperty((float*)&sampler.mWeighting , 1,
		_AI_MATKEY_TEXBLEND_BASE, type, idx);

	// UV source index ... if we didn't resolve the mapping it is actually just 
	// a guess but it works in most cases. We search for the frst occurence of a
	// number in the channel name. We assume it is the zero-based index into the
	// UV channel array of all corresponding meshes.
	if (sampler.mUVId != 0xffffffff)
		map = sampler.mUVId;
	else {
		map = 0xffffffff;
		for (std::string::const_iterator it = sampler.mUVChannel.begin();
			it != sampler.mUVChannel.end(); ++it)
		{
			if (IsNumeric(*it)) {
				map = strtol10(&(*it));
				break;
			}
		}
		if (0xffffffff == map) {
			DefaultLogger::get()->warn("Collada: unable to determine UV channel for texture");
			map = 0;
		}
	}
	mat.AddProperty(&map,1,_AI_MATKEY_UVWSRC_BASE,type,idx);
}

// ------------------------------------------------------------------------------------------------
// Fills materials from the collada material definitions
void ColladaLoader::FillMaterials( const ColladaParser& pParser, aiScene* pScene)
{
	for (std::vector<std::pair<Collada::Effect*, aiMaterial*> >::iterator it = newMats.begin(),
		end = newMats.end(); it != end; ++it)
	{
		MaterialHelper&  mat = (MaterialHelper&)*it->second; 
		Collada::Effect& effect = *it->first;

		// resolve shading mode
		int shadeMode;
		if (effect.mFaceted) /* fixme */
			shadeMode = aiShadingMode_Flat;
		else {
			switch( effect.mShadeType)
			{
			case Collada::Shade_Constant: 
				shadeMode = aiShadingMode_NoShading; 
				break;
			case Collada::Shade_Lambert:
				shadeMode = aiShadingMode_Gouraud; 
				break;
			case Collada::Shade_Blinn: 
				shadeMode = aiShadingMode_Blinn;
				break;
			case Collada::Shade_Phong: 
				shadeMode = aiShadingMode_Phong; 
				break;

			default:
				DefaultLogger::get()->warn("Collada: Unrecognized shading mode, using gouraud shading");
				shadeMode = aiShadingMode_Gouraud; 
				break;
			}
		}
		mat.AddProperty<int>( &shadeMode, 1, AI_MATKEY_SHADING_MODEL);

		// double-sided?
		shadeMode = effect.mDoubleSided;
		mat.AddProperty<int>( &shadeMode, 1, AI_MATKEY_TWOSIDED);

		// wireframe?
		shadeMode = effect.mWireframe;
		mat.AddProperty<int>( &shadeMode, 1, AI_MATKEY_ENABLE_WIREFRAME);

		// add material colors
		mat.AddProperty( &effect.mAmbient, 1,AI_MATKEY_COLOR_AMBIENT);
		mat.AddProperty( &effect.mDiffuse, 1, AI_MATKEY_COLOR_DIFFUSE);
		mat.AddProperty( &effect.mSpecular, 1,AI_MATKEY_COLOR_SPECULAR);
		mat.AddProperty( &effect.mEmissive, 1,	AI_MATKEY_COLOR_EMISSIVE);
		mat.AddProperty( &effect.mTransparent, 1, AI_MATKEY_COLOR_TRANSPARENT);
		mat.AddProperty( &effect.mReflective, 1, AI_MATKEY_COLOR_REFLECTIVE);

		// scalar properties
		mat.AddProperty( &effect.mShininess, 1, AI_MATKEY_SHININESS);
		mat.AddProperty( &effect.mRefractIndex, 1, AI_MATKEY_REFRACTI);

		// add textures, if given
		if( !effect.mTexAmbient.mName.empty()) 
			 /* It is merely a lightmap */
			AddTexture( mat, pParser, effect, effect.mTexAmbient,aiTextureType_LIGHTMAP);

		if( !effect.mTexEmissive.mName.empty())
			AddTexture( mat, pParser, effect, effect.mTexEmissive,aiTextureType_EMISSIVE);

		if( !effect.mTexSpecular.mName.empty())
			AddTexture( mat, pParser, effect, effect.mTexSpecular,aiTextureType_SPECULAR);

		if( !effect.mTexDiffuse.mName.empty())
			AddTexture( mat, pParser, effect, effect.mTexDiffuse,aiTextureType_DIFFUSE);

		if( !effect.mTexBump.mName.empty())
			AddTexture( mat, pParser, effect, effect.mTexBump,aiTextureType_HEIGHT);

		if( !effect.mTexTransparent.mName.empty())
			AddTexture( mat, pParser, effect, effect.mTexBump,aiTextureType_OPACITY);

		if( !effect.mTexReflective.mName.empty())
			AddTexture( mat, pParser, effect, effect.mTexReflective,aiTextureType_REFLECTION);
	}
}

// ------------------------------------------------------------------------------------------------
// Constructs materials from the collada material definitions
void ColladaLoader::BuildMaterials( const ColladaParser& pParser, aiScene* pScene)
{
	newMats.reserve(pParser.mMaterialLibrary.size());

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
		mat->AddProperty(&name,AI_MATKEY_NAME);

		// MEGA SUPER MONSTER HACK by Alex ... It's all my fault, yes.
		// We store the reference to the effect in the material and
		// return ... we'll add the actual material properties later
		// after we processed all meshes. During mesh processing,
		// we evaluate vertex input mappings. Afterwards we should be
		// able to correctly setup source UV channels for textures.

		// ... moved to ColladaLoader::FillMaterials()
		// *duck*

		// store the material
		mMaterialIndexByName[matIt->first] = newMats.size();
		newMats.push_back( std::pair<Collada::Effect*, aiMaterial*>(const_cast<Collada::Effect*>(&effect),mat) );
	}

	// store a dummy material if none were given
	if( newMats.size() == 0)
	{
		Assimp::MaterialHelper* mat = new Assimp::MaterialHelper;
		aiString name( AI_DEFAULT_MATERIAL_NAME );
		mat->AddProperty( &name, AI_MATKEY_NAME);

		const int shadeMode = aiShadingMode_Phong;
		mat->AddProperty<int>( &shadeMode, 1, AI_MATKEY_SHADING_MODEL);
		aiColor4D colAmbient( 0.2f, 0.2f, 0.2f, 1.0f), colDiffuse( 0.8f, 0.8f, 0.8f, 1.0f), colSpecular( 0.5f, 0.5f, 0.5f, 0.5f);
		mat->AddProperty( &colAmbient, 1, AI_MATKEY_COLOR_AMBIENT);
		mat->AddProperty( &colDiffuse, 1, AI_MATKEY_COLOR_DIFFUSE);
		mat->AddProperty( &colSpecular, 1, AI_MATKEY_COLOR_SPECULAR);
		const float specExp = 5.0f;
		mat->AddProperty( &specExp, 1, AI_MATKEY_SHININESS);
	}
}

// ------------------------------------------------------------------------------------------------
// Resolves the texture name for the given effect texture entry
const aiString& ColladaLoader::FindFilenameForEffectTexture( const ColladaParser& pParser,
	const Collada::Effect& pEffect, const std::string& pName)
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
	if( imIt == pParser.mImageLibrary.end()) {
		throw new ImportErrorException( boost::str( boost::format( 
			"Collada: Unable to resolve effect texture entry \"%s\", ended up at ID \"%s\".") % pName % name));
	}
	static aiString result;

	// if this is an embedded texture image setup an aiTexture for it
	if (imIt->second.mFileName.empty()) {
		if (imIt->second.mImageData.empty()) {
			throw new ImportErrorException("Collada: Invalid texture, no data or file reference given");
		}
		aiTexture* tex = new aiTexture();

		// setup format hint
		if (imIt->second.mEmbeddedFormat.length() > 3) {
			DefaultLogger::get()->warn("Collada: texture format hint is too long, truncating to 3 characters");
		}
		::strncpy(tex->achFormatHint,imIt->second.mEmbeddedFormat.c_str(),3);

		// and copy texture data
		tex->mHeight = 0;
		tex->mWidth = imIt->second.mImageData.size();
		tex->pcData = (aiTexel*)new char[tex->mWidth];
		::memcpy(tex->pcData,&imIt->second.mImageData[0],tex->mWidth);

		// setup texture reference string
		result.data[0] = '*';
		result.length = 1 + ASSIMP_itoa10(result.data+1,MAXLEN-1,mTextures.size());

		// and add this texture to the list
		mTextures.push_back(tex);
	}
	else {
		result.Set( imIt->second.mFileName );
		ConvertPath(result);
	}
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

#endif // !! ASSIMP_BUILD_NO_DAE_IMPORTER
