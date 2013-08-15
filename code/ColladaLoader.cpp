/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team

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

/** @file Implementation of the Collada loader */

#include "AssimpPCH.h"
#ifndef ASSIMP_BUILD_NO_COLLADA_IMPORTER

#include "../include/assimp/anim.h"
#include "ColladaLoader.h"
#include "ColladaParser.h"

#include "fast_atof.h"
#include "ParsingUtils.h"
#include "SkeletonMeshBuilder.h"

#include "time.h"

using namespace Assimp;

static const aiImporterDesc desc = {
	"Collada Importer",
	"",
	"",
	"http://collada.org",
	aiImporterFlags_SupportTextFlavour,
	1,
	3,
	1,
	5,
	"dae" 
};


// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
ColladaLoader::ColladaLoader()
: noSkeletonMesh(), ignoreUpDirection(false)
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
void ColladaLoader::SetupProperties(const Importer* pImp)
{
	noSkeletonMesh = pImp->GetPropertyInteger(AI_CONFIG_IMPORT_NO_SKELETON_MESHES,0) != 0;
	ignoreUpDirection = pImp->GetPropertyInteger(AI_CONFIG_IMPORT_COLLADA_IGNORE_UP_DIRECTION,0) != 0;
}


// ------------------------------------------------------------------------------------------------
// Get file extension list
const aiImporterDesc* ColladaLoader::GetInfo () const
{
	return &desc;
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
		throw DeadlyImportError( "Collada: File came out empty. Something is wrong here.");

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

        // Apply unitsize scale calculation
        pScene->mRootNode->mTransformation *= aiMatrix4x4(parser.mUnitSize, 0,  0,  0, 
                                                          0,  parser.mUnitSize,  0,  0,
                                                          0,  0,  parser.mUnitSize,  0,
                                                          0,  0,  0,  1);
        if( !ignoreUpDirection ) {
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
        }
	// store all meshes
	StoreSceneMeshes( pScene);

	// store all materials
	StoreSceneMaterials( pScene);

	// store all lights
	StoreSceneLights( pScene);

	// store all cameras
	StoreSceneCameras( pScene);

	// store all animations
	StoreAnimations( pScene, parser);


	// If no meshes have been loaded, it's probably just an animated skeleton.
	if (!pScene->mNumMeshes) {
	
		if (!noSkeletonMesh) {
			SkeletonMeshBuilder hero(pScene);
		}
		pScene->mFlags |= AI_SCENE_FLAGS_INCOMPLETE;
	}
}

// ------------------------------------------------------------------------------------------------
// Recursively constructs a scene node for the given parser node and returns it.
aiNode* ColladaLoader::BuildHierarchy( const ColladaParser& pParser, const Collada::Node* pNode)
{
	// create a node for it
	aiNode* node = new aiNode();

	// find a name for the new node. It's more complicated than you might think
	node->mName.Set( FindNameForNode( pNode));

	// calculate the transformation matrix for it
	node->mTransformation = pParser.CalculateResultTransform( pNode->mTransforms);

	// now resolve node instances
	std::vector<const Collada::Node*> instances;
	ResolveNodeInstances(pParser,pNode,instances);

	// add children. first the *real* ones
	node->mNumChildren = pNode->mChildren.size()+instances.size();
	node->mChildren = new aiNode*[node->mNumChildren];

	for( size_t a = 0; a < pNode->mChildren.size(); a++)
	{
		node->mChildren[a] = BuildHierarchy( pParser, pNode->mChildren[a]);
		node->mChildren[a]->mParent = node;
	}

	// ... and finally the resolved node instances
	for( size_t a = 0; a < instances.size(); a++)
	{
		node->mChildren[pNode->mChildren.size() + a] = BuildHierarchy( pParser, instances[a]);
		node->mChildren[pNode->mChildren.size() + a]->mParent = node;
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
	std::vector<const Collada::Node*>& resolved)
{
	// reserve enough storage
	resolved.reserve(pNode->mNodeInstances.size());

	// ... and iterate through all nodes to be instanced as children of pNode
	for (std::vector<Collada::NodeInstance>::const_iterator it = pNode->mNodeInstances.begin(),
		 end = pNode->mNodeInstances.end(); it != end; ++it)
	{
		// find the corresponding node in the library
		const ColladaParser::NodeLibrary::const_iterator itt = pParser.mNodeLibrary.find((*it).mNode);
		const Collada::Node* nd = itt == pParser.mNodeLibrary.end() ? NULL : (*itt).second;

		// FIX for http://sourceforge.net/tracker/?func=detail&aid=3054873&group_id=226462&atid=1067632
		// need to check for both name and ID to catch all. To avoid breaking valid files,
		// the workaround is only enabled when the first attempt to resolve the node has failed.
		if (!nd) {
			nd = FindNode(pParser.mRootNode,(*it).mNode);
		}
		if (!nd) 
			DefaultLogger::get()->error("Collada: Unable to resolve reference to instanced node " + (*it).mNode);
		
		else {
			//	attach this node to the list of children
			resolved.push_back(nd);
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

			// ... some extension magic. 
			if (srcLight->mOuterAngle >= ASSIMP_COLLADA_LIGHT_ANGLE_NOT_SET*(1-1e-6f))
			{
				// ... some deprecation magic. 
				if (srcLight->mPenumbraAngle >= ASSIMP_COLLADA_LIGHT_ANGLE_NOT_SET*(1-1e-6f))
				{
					// Need to rely on falloff_exponent. I don't know how to interpret it, so I need to guess ....
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
		// and we need to compute the others in any combination. 
		 if (srcCamera->mAspect != 10e10f)
			out->mAspect = srcCamera->mAspect;

		if (srcCamera->mHorFov != 10e10f) {
			out->mHorizontalFOV = srcCamera->mHorFov; 

			if (srcCamera->mVerFov != 10e10f && srcCamera->mAspect == 10e10f) {
				out->mAspect = tan(AI_DEG_TO_RAD(srcCamera->mHorFov)) /
                    tan(AI_DEG_TO_RAD(srcCamera->mVerFov));
			}
		}
		else if (srcCamera->mAspect != 10e10f && srcCamera->mVerFov != 10e10f)	{
			out->mHorizontalFOV = 2.0f * AI_RAD_TO_DEG(atan(srcCamera->mAspect *
                tan(AI_DEG_TO_RAD(srcCamera->mVerFov) * 0.5f)));
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
		const Collada::Mesh* srcMesh = NULL;
		const Collada::Controller* srcController = NULL;

		// find the referred mesh
		ColladaParser::MeshLibrary::const_iterator srcMeshIt = pParser.mMeshLibrary.find( mid.mMeshOrController);
		if( srcMeshIt == pParser.mMeshLibrary.end())
		{
			// if not found in the mesh-library, it might also be a controller referring to a mesh
			ColladaParser::ControllerLibrary::const_iterator srcContrIt = pParser.mControllerLibrary.find( mid.mMeshOrController);
			if( srcContrIt != pParser.mControllerLibrary.end())
			{
				srcController = &srcContrIt->second;
				srcMeshIt = pParser.mMeshLibrary.find( srcController->mMeshId);
				if( srcMeshIt != pParser.mMeshLibrary.end())
					srcMesh = srcMeshIt->second;
			}

			if( !srcMesh)
			{
				DefaultLogger::get()->warn( boost::str( boost::format( "Collada: Unable to find geometry for ID \"%s\". Skipping.") % mid.mMeshOrController));
				continue;
			}
		} else
		{
			// ID found in the mesh library -> direct reference to an unskinned mesh
			srcMesh = srcMeshIt->second;
		}

		// build a mesh for each of its subgroups
		size_t vertexStart = 0, faceStart = 0;
		for( size_t sm = 0; sm < srcMesh->mSubMeshes.size(); ++sm)
		{
			const Collada::SubMesh& submesh = srcMesh->mSubMeshes[sm];
			if( submesh.mNumFaces == 0)
				continue;

			// find material assigned to this submesh
			std::string meshMaterial;
			std::map<std::string, Collada::SemanticMappingTable >::const_iterator meshMatIt = mid.mMaterials.find( submesh.mMaterial);

			const Collada::SemanticMappingTable* table = NULL;
			if( meshMatIt != mid.mMaterials.end())
			{
				table = &meshMatIt->second;
				meshMaterial = table->mMatName;
			}
			else 
			{
				DefaultLogger::get()->warn( boost::str( boost::format( "Collada: No material specified for subgroup <%s> in geometry <%s>.") % submesh.mMaterial % mid.mMeshOrController));
				if( !mid.mMaterials.empty() )
					meshMaterial = mid.mMaterials.begin()->second.mMatName;
			}

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
			ColladaMeshIndex index( mid.mMeshOrController, sm, meshMaterial);

			// if we already have the mesh at the library, just add its index to the node's array
			std::map<ColladaMeshIndex, size_t>::const_iterator dstMeshIt = mMeshIndexByID.find( index);
			if( dstMeshIt != mMeshIndexByID.end())	{
				newMeshRefs.push_back( dstMeshIt->second);
			} 
			else
			{
				// else we have to add the mesh to the collection and store its newly assigned index at the node
				aiMesh* dstMesh = CreateMesh( pParser, srcMesh, submesh, srcController, vertexStart, faceStart);

				// store the mesh, and store its new index in the node
				newMeshRefs.push_back( mMeshes.size());
				mMeshIndexByID[index] = mMeshes.size();
				mMeshes.push_back( dstMesh);
				vertexStart += dstMesh->mNumVertices; faceStart += submesh.mNumFaces;

				// assign the material index
				dstMesh->mMaterialIndex = matIdx;
                if(dstMesh->mName.length == 0)
                {
                    dstMesh->mName = mid.mMeshOrController;
                }
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
// Creates a mesh for the given ColladaMesh face subset and returns the newly created mesh
aiMesh* ColladaLoader::CreateMesh( const ColladaParser& pParser, const Collada::Mesh* pSrcMesh, const Collada::SubMesh& pSubMesh, 
	const Collada::Controller* pSrcController, size_t pStartVertex, size_t pStartFace)
{
	aiMesh* dstMesh = new aiMesh;
    
    dstMesh->mName = pSrcMesh->mName;

	// count the vertices addressed by its faces
	const size_t numVertices = std::accumulate( pSrcMesh->mFaceSize.begin() + pStartFace,
		pSrcMesh->mFaceSize.begin() + pStartFace + pSubMesh.mNumFaces, 0);

	// copy positions
	dstMesh->mNumVertices = numVertices;
	dstMesh->mVertices = new aiVector3D[numVertices];
	std::copy( pSrcMesh->mPositions.begin() + pStartVertex, pSrcMesh->mPositions.begin() + 
		pStartVertex + numVertices, dstMesh->mVertices);

	// normals, if given. HACK: (thom) Due to the glorious Collada spec we never 
	// know if we have the same number of normals as there are positions. So we 
	// also ignore any vertex attribute if it has a different count
	if( pSrcMesh->mNormals.size() >= pStartVertex + numVertices)
	{
		dstMesh->mNormals = new aiVector3D[numVertices];
		std::copy( pSrcMesh->mNormals.begin() + pStartVertex, pSrcMesh->mNormals.begin() +
			pStartVertex + numVertices, dstMesh->mNormals);
	}

	// tangents, if given. 
	if( pSrcMesh->mTangents.size() >= pStartVertex + numVertices)
	{
		dstMesh->mTangents = new aiVector3D[numVertices];
		std::copy( pSrcMesh->mTangents.begin() + pStartVertex, pSrcMesh->mTangents.begin() + 
			pStartVertex + numVertices, dstMesh->mTangents);
	}

	// bitangents, if given. 
	if( pSrcMesh->mBitangents.size() >= pStartVertex + numVertices)
	{
		dstMesh->mBitangents = new aiVector3D[numVertices];
		std::copy( pSrcMesh->mBitangents.begin() + pStartVertex, pSrcMesh->mBitangents.begin() + 
			pStartVertex + numVertices, dstMesh->mBitangents);
	}

	// same for texturecoords, as many as we have
	// empty slots are not allowed, need to pack and adjust UV indexes accordingly
	for( size_t a = 0, real = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; a++)
	{
		if( pSrcMesh->mTexCoords[a].size() >= pStartVertex + numVertices)
		{
			dstMesh->mTextureCoords[real] = new aiVector3D[numVertices];
			for( size_t b = 0; b < numVertices; ++b)
				dstMesh->mTextureCoords[real][b] = pSrcMesh->mTexCoords[a][pStartVertex+b];
			
			dstMesh->mNumUVComponents[real] = pSrcMesh->mNumUVComponents[a];
			++real;
		}
	}

	// same for vertex colors, as many as we have. again the same packing to avoid empty slots
	for( size_t a = 0, real = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; a++)
	{
		if( pSrcMesh->mColors[a].size() >= pStartVertex + numVertices)
		{
			dstMesh->mColors[real] = new aiColor4D[numVertices];
			std::copy( pSrcMesh->mColors[a].begin() + pStartVertex, pSrcMesh->mColors[a].begin() + pStartVertex + numVertices,dstMesh->mColors[real]);
			++real;
		}
	}

	// create faces. Due to the fact that each face uses unique vertices, we can simply count up on each vertex
	size_t vertex = 0;
	dstMesh->mNumFaces = pSubMesh.mNumFaces;
	dstMesh->mFaces = new aiFace[dstMesh->mNumFaces];
	for( size_t a = 0; a < dstMesh->mNumFaces; ++a)
	{
		size_t s = pSrcMesh->mFaceSize[ pStartFace + a];
		aiFace& face = dstMesh->mFaces[a];
		face.mNumIndices = s;
		face.mIndices = new unsigned int[s];
		for( size_t b = 0; b < s; ++b)
			face.mIndices[b] = vertex++;
	}

	// create bones if given
	if( pSrcController)
	{
		// refuse if the vertex count does not match
//		if( pSrcController->mWeightCounts.size() != dstMesh->mNumVertices)
//			throw DeadlyImportError( "Joint Controller vertex count does not match mesh vertex count");

		// resolve references - joint names
		const Collada::Accessor& jointNamesAcc = pParser.ResolveLibraryReference( pParser.mAccessorLibrary, pSrcController->mJointNameSource);
		const Collada::Data& jointNames = pParser.ResolveLibraryReference( pParser.mDataLibrary, jointNamesAcc.mSource);
		// joint offset matrices
		const Collada::Accessor& jointMatrixAcc = pParser.ResolveLibraryReference( pParser.mAccessorLibrary, pSrcController->mJointOffsetMatrixSource);
		const Collada::Data& jointMatrices = pParser.ResolveLibraryReference( pParser.mDataLibrary, jointMatrixAcc.mSource);
		// joint vertex_weight name list - should refer to the same list as the joint names above. If not, report and reconsider
		const Collada::Accessor& weightNamesAcc = pParser.ResolveLibraryReference( pParser.mAccessorLibrary, pSrcController->mWeightInputJoints.mAccessor);
		if( &weightNamesAcc != &jointNamesAcc)
			throw DeadlyImportError( "Temporary implementational lazyness. If you read this, please report to the author.");
		// vertex weights
		const Collada::Accessor& weightsAcc = pParser.ResolveLibraryReference( pParser.mAccessorLibrary, pSrcController->mWeightInputWeights.mAccessor);
		const Collada::Data& weights = pParser.ResolveLibraryReference( pParser.mDataLibrary, weightsAcc.mSource);

		if( !jointNames.mIsStringArray || jointMatrices.mIsStringArray || weights.mIsStringArray)
			throw DeadlyImportError( "Data type mismatch while resolving mesh joints");
		// sanity check: we rely on the vertex weights always coming as pairs of BoneIndex-WeightIndex
		if( pSrcController->mWeightInputJoints.mOffset != 0 || pSrcController->mWeightInputWeights.mOffset != 1)
			throw DeadlyImportError( "Unsupported vertex_weight addressing scheme. ");

		// create containers to collect the weights for each bone
		size_t numBones = jointNames.mStrings.size();
		std::vector<std::vector<aiVertexWeight> > dstBones( numBones);

		// build a temporary array of pointers to the start of each vertex's weights
		typedef std::vector< std::pair<size_t, size_t> > IndexPairVector;
		std::vector<IndexPairVector::const_iterator> weightStartPerVertex;
		weightStartPerVertex.resize(pSrcController->mWeightCounts.size(),pSrcController->mWeights.end());

		IndexPairVector::const_iterator pit = pSrcController->mWeights.begin();
		for( size_t a = 0; a < pSrcController->mWeightCounts.size(); ++a)
		{
			weightStartPerVertex[a] = pit;
			pit += pSrcController->mWeightCounts[a];
		}

		// now for each vertex put the corresponding vertex weights into each bone's weight collection
		for( size_t a = pStartVertex; a < pStartVertex + numVertices; ++a)
		{
			// which position index was responsible for this vertex? that's also the index by which
			// the controller assigns the vertex weights
			size_t orgIndex = pSrcMesh->mFacePosIndices[a];
			// find the vertex weights for this vertex
			IndexPairVector::const_iterator iit = weightStartPerVertex[orgIndex];
			size_t pairCount = pSrcController->mWeightCounts[orgIndex];

			for( size_t b = 0; b < pairCount; ++b, ++iit)
			{
				size_t jointIndex = iit->first;
				size_t vertexIndex = iit->second;

				float weight = ReadFloat( weightsAcc, weights, vertexIndex, 0);

				// one day I gonna kill that XSI Collada exporter
				if( weight > 0.0f)
				{
					aiVertexWeight w;
					w.mVertexId = a - pStartVertex;
					w.mWeight = weight;
					dstBones[jointIndex].push_back( w);
				}
			}
		}

		// count the number of bones which influence vertices of the current submesh
		size_t numRemainingBones = 0;
		for( std::vector<std::vector<aiVertexWeight> >::const_iterator it = dstBones.begin(); it != dstBones.end(); ++it)
			if( it->size() > 0)
				numRemainingBones++;

		// create bone array and copy bone weights one by one
		dstMesh->mNumBones = numRemainingBones;
		dstMesh->mBones = new aiBone*[numRemainingBones];
		size_t boneCount = 0;
		for( size_t a = 0; a < numBones; ++a)
		{
			// omit bones without weights
			if( dstBones[a].size() == 0)
				continue;

			// create bone with its weights
			aiBone* bone = new aiBone;
			bone->mName = ReadString( jointNamesAcc, jointNames, a);
			bone->mOffsetMatrix.a1 = ReadFloat( jointMatrixAcc, jointMatrices, a, 0);
			bone->mOffsetMatrix.a2 = ReadFloat( jointMatrixAcc, jointMatrices, a, 1);
			bone->mOffsetMatrix.a3 = ReadFloat( jointMatrixAcc, jointMatrices, a, 2);
			bone->mOffsetMatrix.a4 = ReadFloat( jointMatrixAcc, jointMatrices, a, 3);
			bone->mOffsetMatrix.b1 = ReadFloat( jointMatrixAcc, jointMatrices, a, 4);
			bone->mOffsetMatrix.b2 = ReadFloat( jointMatrixAcc, jointMatrices, a, 5);
			bone->mOffsetMatrix.b3 = ReadFloat( jointMatrixAcc, jointMatrices, a, 6);
			bone->mOffsetMatrix.b4 = ReadFloat( jointMatrixAcc, jointMatrices, a, 7);
			bone->mOffsetMatrix.c1 = ReadFloat( jointMatrixAcc, jointMatrices, a, 8);
			bone->mOffsetMatrix.c2 = ReadFloat( jointMatrixAcc, jointMatrices, a, 9);
			bone->mOffsetMatrix.c3 = ReadFloat( jointMatrixAcc, jointMatrices, a, 10);
			bone->mOffsetMatrix.c4 = ReadFloat( jointMatrixAcc, jointMatrices, a, 11);
			bone->mNumWeights = dstBones[a].size();
			bone->mWeights = new aiVertexWeight[bone->mNumWeights];
			std::copy( dstBones[a].begin(), dstBones[a].end(), bone->mWeights);

			// apply bind shape matrix to offset matrix
			aiMatrix4x4 bindShapeMatrix;
			bindShapeMatrix.a1 = pSrcController->mBindShapeMatrix[0];
			bindShapeMatrix.a2 = pSrcController->mBindShapeMatrix[1];
			bindShapeMatrix.a3 = pSrcController->mBindShapeMatrix[2];
			bindShapeMatrix.a4 = pSrcController->mBindShapeMatrix[3];
			bindShapeMatrix.b1 = pSrcController->mBindShapeMatrix[4];
			bindShapeMatrix.b2 = pSrcController->mBindShapeMatrix[5];
			bindShapeMatrix.b3 = pSrcController->mBindShapeMatrix[6];
			bindShapeMatrix.b4 = pSrcController->mBindShapeMatrix[7];
			bindShapeMatrix.c1 = pSrcController->mBindShapeMatrix[8];
			bindShapeMatrix.c2 = pSrcController->mBindShapeMatrix[9];
			bindShapeMatrix.c3 = pSrcController->mBindShapeMatrix[10];
			bindShapeMatrix.c4 = pSrcController->mBindShapeMatrix[11];
			bindShapeMatrix.d1 = pSrcController->mBindShapeMatrix[12];
			bindShapeMatrix.d2 = pSrcController->mBindShapeMatrix[13];
			bindShapeMatrix.d3 = pSrcController->mBindShapeMatrix[14];
			bindShapeMatrix.d4 = pSrcController->mBindShapeMatrix[15];
			bone->mOffsetMatrix *= bindShapeMatrix;

			// HACK: (thom) Some exporters address the bone nodes by SID, others address them by ID or even name.
			// Therefore I added a little name replacement here: I search for the bone's node by either name, ID or SID,
			// and replace the bone's name by the node's name so that the user can use the standard
			// find-by-name method to associate nodes with bones.
			const Collada::Node* bnode = FindNode( pParser.mRootNode, bone->mName.data);
			if( !bnode)
				bnode = FindNodeBySID( pParser.mRootNode, bone->mName.data);

			// assign the name that we would have assigned for the source node
			if( bnode)
				bone->mName.Set( FindNameForNode( bnode));
			else
				DefaultLogger::get()->warn( boost::str( boost::format( "ColladaLoader::CreateMesh(): could not find corresponding node for joint \"%s\".") % bone->mName.data));

			// and insert bone
			dstMesh->mBones[boneCount++] = bone;
		}
	}

	return dstMesh;
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
		mMeshes.clear();
	}
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
		mCameras.clear();
	}
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
		mLights.clear();
	}
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
		mTextures.clear();
	}
}

// ------------------------------------------------------------------------------------------------
// Stores all materials in the given scene
void ColladaLoader::StoreSceneMaterials( aiScene* pScene)
{
	pScene->mNumMaterials = newMats.size();

	if (newMats.size() > 0) {
		pScene->mMaterials = new aiMaterial*[newMats.size()];
		for (unsigned int i = 0; i < newMats.size();++i)
			pScene->mMaterials[i] = newMats[i].second;

		newMats.clear();
	}
}

// ------------------------------------------------------------------------------------------------
// Stores all animations 
void ColladaLoader::StoreAnimations( aiScene* pScene, const ColladaParser& pParser)
{
	// recursivly collect all animations from the collada scene
	StoreAnimations( pScene, pParser, &pParser.mAnims, "");

	// catch special case: many animations with the same length, each affecting only a single node.
	// we need to unite all those single-node-anims to a proper combined animation
	for( size_t a = 0; a < mAnims.size(); ++a)
	{
		aiAnimation* templateAnim = mAnims[a];
		if( templateAnim->mNumChannels == 1)
		{
			// search for other single-channel-anims with the same duration
			std::vector<size_t> collectedAnimIndices;
			for( size_t b = a+1; b < mAnims.size(); ++b)
			{
				aiAnimation* other = mAnims[b];
				if( other->mNumChannels == 1 && other->mDuration == templateAnim->mDuration && other->mTicksPerSecond == templateAnim->mTicksPerSecond )
					collectedAnimIndices.push_back( b);
			}

			// if there are other animations which fit the template anim, combine all channels into a single anim
			if( !collectedAnimIndices.empty() )
			{
				aiAnimation* combinedAnim = new aiAnimation();
				combinedAnim->mName = aiString( std::string( "combinedAnim_") + char( '0' + a));
				combinedAnim->mDuration = templateAnim->mDuration;
				combinedAnim->mTicksPerSecond = templateAnim->mTicksPerSecond;
				combinedAnim->mNumChannels = collectedAnimIndices.size() + 1;
				combinedAnim->mChannels = new aiNodeAnim*[combinedAnim->mNumChannels];
				// add the template anim as first channel by moving its aiNodeAnim to the combined animation
				combinedAnim->mChannels[0] = templateAnim->mChannels[0];
				templateAnim->mChannels[0] = NULL;
				delete templateAnim;
				// combined animation replaces template animation in the anim array
				mAnims[a] = combinedAnim;

				// move the memory of all other anims to the combined anim and erase them from the source anims
				for( size_t b = 0; b < collectedAnimIndices.size(); ++b)
				{
					aiAnimation* srcAnimation = mAnims[collectedAnimIndices[b]];
					combinedAnim->mChannels[1 + b] = srcAnimation->mChannels[0];
					srcAnimation->mChannels[0] = NULL;
					delete srcAnimation;
				}

				// in a second go, delete all the single-channel-anims that we've stripped from their channels
				// back to front to preserve indices - you know, removing an element from a vector moves all elements behind the removed one
				while( !collectedAnimIndices.empty() )
				{
					mAnims.erase( mAnims.begin() + collectedAnimIndices.back());
					collectedAnimIndices.pop_back();
				}
			}
		}
	}

	// now store all anims in the scene
	if( !mAnims.empty())
	{
		pScene->mNumAnimations = mAnims.size();
		pScene->mAnimations = new aiAnimation*[mAnims.size()];
		std::copy( mAnims.begin(), mAnims.end(), pScene->mAnimations);
	}
}

// ------------------------------------------------------------------------------------------------
// Constructs the animations for the given source anim 
void ColladaLoader::StoreAnimations( aiScene* pScene, const ColladaParser& pParser, const Collada::Animation* pSrcAnim, const std::string pPrefix)
{
	std::string animName = pPrefix.empty() ? pSrcAnim->mName : pPrefix + "_" + pSrcAnim->mName;

	// create nested animations, if given
	for( std::vector<Collada::Animation*>::const_iterator it = pSrcAnim->mSubAnims.begin(); it != pSrcAnim->mSubAnims.end(); ++it)
		StoreAnimations( pScene, pParser, *it, animName);

	// create animation channels, if any
	if( !pSrcAnim->mChannels.empty())
		CreateAnimation( pScene, pParser, pSrcAnim, animName);
}

// ------------------------------------------------------------------------------------------------
// Constructs the animation for the given source anim
void ColladaLoader::CreateAnimation( aiScene* pScene, const ColladaParser& pParser, const Collada::Animation* pSrcAnim, const std::string& pName)
{
	// collect a list of animatable nodes
	std::vector<const aiNode*> nodes;
	CollectNodes( pScene->mRootNode, nodes);

	std::vector<aiNodeAnim*> anims;
	for( std::vector<const aiNode*>::const_iterator nit = nodes.begin(); nit != nodes.end(); ++nit)
	{
		// find all the collada anim channels which refer to the current node
		std::vector<Collada::ChannelEntry> entries;
		std::string nodeName = (*nit)->mName.data;

		// find the collada node corresponding to the aiNode
		const Collada::Node* srcNode = FindNode( pParser.mRootNode, nodeName);
//		ai_assert( srcNode != NULL);
		if( !srcNode)
			continue;

		// now check all channels if they affect the current node
		for( std::vector<Collada::AnimationChannel>::const_iterator cit = pSrcAnim->mChannels.begin();
			cit != pSrcAnim->mChannels.end(); ++cit)
		{
			const Collada::AnimationChannel& srcChannel = *cit;
			Collada::ChannelEntry entry;

			// we expect the animation target to be of type "nodeName/transformID.subElement". Ignore all others
			// find the slash that separates the node name - there should be only one
			std::string::size_type slashPos = srcChannel.mTarget.find( '/');
			if( slashPos == std::string::npos)
				continue;
			if( srcChannel.mTarget.find( '/', slashPos+1) != std::string::npos)
				continue;
			std::string targetID = srcChannel.mTarget.substr( 0, slashPos);
			if( targetID != srcNode->mID)
				continue;

			// find the dot that separates the transformID - there should be only one or zero
			std::string::size_type dotPos = srcChannel.mTarget.find( '.');
			if( dotPos != std::string::npos)
			{
				if( srcChannel.mTarget.find( '.', dotPos+1) != std::string::npos)
					continue;

				entry.mTransformId = srcChannel.mTarget.substr( slashPos+1, dotPos - slashPos - 1);

				std::string subElement = srcChannel.mTarget.substr( dotPos+1);
				if( subElement == "ANGLE")
					entry.mSubElement = 3; // last number in an Axis-Angle-Transform is the angle
				else if( subElement == "X")
					entry.mSubElement = 0;
				else if( subElement == "Y")
					entry.mSubElement = 1;
				else if( subElement == "Z")
					entry.mSubElement = 2;
				else 
					DefaultLogger::get()->warn( boost::str( boost::format( "Unknown anim subelement <%s>. Ignoring") % subElement));
			} else
			{
				// no subelement following, transformId is remaining string
				entry.mTransformId = srcChannel.mTarget.substr( slashPos+1);
			}

			// determine which transform step is affected by this channel
			entry.mTransformIndex = SIZE_MAX;
			for( size_t a = 0; a < srcNode->mTransforms.size(); ++a)
				if( srcNode->mTransforms[a].mID == entry.mTransformId)
					entry.mTransformIndex = a;

			if( entry.mTransformIndex == SIZE_MAX) {
				continue;
			}

			entry.mChannel = &(*cit);
			entries.push_back( entry);
		}

		// if there's no channel affecting the current node, we skip it
		if( entries.empty())
			continue;

		// resolve the data pointers for all anim channels. Find the minimum time while we're at it
		float startTime = 1e20f, endTime = -1e20f;
		for( std::vector<Collada::ChannelEntry>::iterator it = entries.begin(); it != entries.end(); ++it)
		{
			Collada::ChannelEntry& e = *it;
			e.mTimeAccessor = &pParser.ResolveLibraryReference( pParser.mAccessorLibrary, e.mChannel->mSourceTimes);
			e.mTimeData = &pParser.ResolveLibraryReference( pParser.mDataLibrary, e.mTimeAccessor->mSource);
			e.mValueAccessor = &pParser.ResolveLibraryReference( pParser.mAccessorLibrary, e.mChannel->mSourceValues);
			e.mValueData = &pParser.ResolveLibraryReference( pParser.mDataLibrary, e.mValueAccessor->mSource);

			// time count and value count must match
			if( e.mTimeAccessor->mCount != e.mValueAccessor->mCount)
				throw DeadlyImportError( boost::str( boost::format( "Time count / value count mismatch in animation channel \"%s\".") % e.mChannel->mTarget));

      if( e.mTimeAccessor->mCount > 0 )
      {
			  // find bounding times
			  startTime = std::min( startTime, ReadFloat( *e.mTimeAccessor, *e.mTimeData, 0, 0));
  			endTime = std::max( endTime, ReadFloat( *e.mTimeAccessor, *e.mTimeData, e.mTimeAccessor->mCount-1, 0));
      }
		}

    std::vector<aiMatrix4x4> resultTrafos;
    if( !entries.empty() && entries.front().mTimeAccessor->mCount > 0 )
    {
		  // create a local transformation chain of the node's transforms
		  std::vector<Collada::Transform> transforms = srcNode->mTransforms;

		  // now for every unique point in time, find or interpolate the key values for that time
		  // and apply them to the transform chain. Then the node's present transformation can be calculated.
		  float time = startTime;
		  while( 1)
		  {
			  for( std::vector<Collada::ChannelEntry>::iterator it = entries.begin(); it != entries.end(); ++it)
			  {
				  Collada::ChannelEntry& e = *it;

				  // find the keyframe behind the current point in time
				  size_t pos = 0;
				  float postTime = 0.f;
				  while( 1)
				  {
					  if( pos >= e.mTimeAccessor->mCount)
						  break;
					  postTime = ReadFloat( *e.mTimeAccessor, *e.mTimeData, pos, 0);
					  if( postTime >= time)
						  break;
					  ++pos;
				  }

				  pos = std::min( pos, e.mTimeAccessor->mCount-1);

				  // read values from there
				  float temp[16];
				  for( size_t c = 0; c < e.mValueAccessor->mSize; ++c)
					  temp[c] = ReadFloat( *e.mValueAccessor, *e.mValueData, pos, c);

				  // if not exactly at the key time, interpolate with previous value set
				  if( postTime > time && pos > 0)
				  {
					  float preTime = ReadFloat( *e.mTimeAccessor, *e.mTimeData, pos-1, 0);
					  float factor = (time - postTime) / (preTime - postTime);

					  for( size_t c = 0; c < e.mValueAccessor->mSize; ++c)
					  {
						  float v = ReadFloat( *e.mValueAccessor, *e.mValueData, pos-1, c);
						  temp[c] += (v - temp[c]) * factor;
					  }
				  }

				  // Apply values to current transformation
				  std::copy( temp, temp + e.mValueAccessor->mSize, transforms[e.mTransformIndex].f + e.mSubElement);
			  }

			  // Calculate resulting transformation
			  aiMatrix4x4 mat = pParser.CalculateResultTransform( transforms);

			  // out of lazyness: we store the time in matrix.d4
			  mat.d4 = time;
			  resultTrafos.push_back( mat);

			  // find next point in time to evaluate. That's the closest frame larger than the current in any channel
			  float nextTime = 1e20f;
			  for( std::vector<Collada::ChannelEntry>::iterator it = entries.begin(); it != entries.end(); ++it)
			  {
				  Collada::ChannelEntry& e = *it;

				  // find the next time value larger than the current
				  size_t pos = 0;
				  while( pos < e.mTimeAccessor->mCount)
				  {
					  float t = ReadFloat( *e.mTimeAccessor, *e.mTimeData, pos, 0);
					  if( t > time)
					  {
						  nextTime = std::min( nextTime, t);
						  break;
					  }
					  ++pos;
				  }
			  }

			  // no more keys on any channel after the current time -> we're done
			  if( nextTime > 1e19)
				  break;

			  // else construct next keyframe at this following time point
			  time = nextTime;
		  }
    }

		// there should be some keyframes, but we aren't that fixated on valid input data
//		ai_assert( resultTrafos.size() > 0);

		// build an animation channel for the given node out of these trafo keys
    if( !resultTrafos.empty() )
    {
		  aiNodeAnim* dstAnim = new aiNodeAnim;
		  dstAnim->mNodeName = nodeName;
		  dstAnim->mNumPositionKeys = resultTrafos.size();
		  dstAnim->mNumRotationKeys= resultTrafos.size();
		  dstAnim->mNumScalingKeys = resultTrafos.size();
		  dstAnim->mPositionKeys = new aiVectorKey[resultTrafos.size()];
		  dstAnim->mRotationKeys = new aiQuatKey[resultTrafos.size()];
		  dstAnim->mScalingKeys = new aiVectorKey[resultTrafos.size()];

		  for( size_t a = 0; a < resultTrafos.size(); ++a)
		  {
			  aiMatrix4x4 mat = resultTrafos[a];
			  double time = double( mat.d4); // remember? time is stored in mat.d4
        mat.d4 = 1.0f;

			  dstAnim->mPositionKeys[a].mTime = time;
			  dstAnim->mRotationKeys[a].mTime = time;
			  dstAnim->mScalingKeys[a].mTime = time;
			  mat.Decompose( dstAnim->mScalingKeys[a].mValue, dstAnim->mRotationKeys[a].mValue, dstAnim->mPositionKeys[a].mValue);
		  }

		  anims.push_back( dstAnim);
    } else
    {
      DefaultLogger::get()->warn( "Collada loader: found empty animation channel, ignored. Please check your exporter.");
    }
	}

	if( !anims.empty())
	{
		aiAnimation* anim = new aiAnimation;
		anim->mName.Set( pName);
		anim->mNumChannels = anims.size();
		anim->mChannels = new aiNodeAnim*[anims.size()];
		std::copy( anims.begin(), anims.end(), anim->mChannels);
		anim->mDuration = 0.0f;
		for( size_t a = 0; a < anims.size(); ++a)
		{
			anim->mDuration = std::max( anim->mDuration, anims[a]->mPositionKeys[anims[a]->mNumPositionKeys-1].mTime);
			anim->mDuration = std::max( anim->mDuration, anims[a]->mRotationKeys[anims[a]->mNumRotationKeys-1].mTime);
			anim->mDuration = std::max( anim->mDuration, anims[a]->mScalingKeys[anims[a]->mNumScalingKeys-1].mTime);
		}
		anim->mTicksPerSecond = 1;
		mAnims.push_back( anim);
	}
}

// ------------------------------------------------------------------------------------------------
// Add a texture to a material structure
void ColladaLoader::AddTexture ( aiMaterial& mat, const ColladaParser& pParser,
	const Collada::Effect& effect,
	const Collada::Sampler& sampler,
	aiTextureType type, unsigned int idx)
{
	// first of all, basic file name
	const aiString name = FindFilenameForEffectTexture( pParser, effect, sampler.mName );
	mat.AddProperty( &name, _AI_MATKEY_TEXTURE_BASE, type, idx );

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

	// UV source index ... if we didn't resolve the mapping, it is actually just 
	// a guess but it works in most cases. We search for the frst occurence of a
	// number in the channel name. We assume it is the zero-based index into the
	// UV channel array of all corresponding meshes. It could also be one-based
	// for some exporters, but we won't care of it unless someone complains about.
	if (sampler.mUVId != UINT_MAX)
		map = sampler.mUVId;
	else {
		map = -1;
		for (std::string::const_iterator it = sampler.mUVChannel.begin();it != sampler.mUVChannel.end(); ++it){
			if (IsNumeric(*it)) {
				map = strtoul10(&(*it));
				break;
			}
		}
		if (-1 == map) {
			DefaultLogger::get()->warn("Collada: unable to determine UV channel for texture");
			map = 0;
		}
	}
	mat.AddProperty(&map,1,_AI_MATKEY_UVWSRC_BASE,type,idx);
}

// ------------------------------------------------------------------------------------------------
// Fills materials from the collada material definitions
void ColladaLoader::FillMaterials( const ColladaParser& pParser, aiScene* /*pScene*/)
{
	for (std::vector<std::pair<Collada::Effect*, aiMaterial*> >::iterator it = newMats.begin(),
		end = newMats.end(); it != end; ++it)
	{
		aiMaterial&  mat = (aiMaterial&)*it->second; 
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
		mat.AddProperty( &effect.mReflectivity, 1, AI_MATKEY_REFLECTIVITY);
		mat.AddProperty( &effect.mRefractIndex, 1, AI_MATKEY_REFRACTI);

		// transparency, a very hard one. seemingly not all files are following the
		// specification here .. but we can trick.
		if (effect.mTransparency >= 0.f && effect.mTransparency < 1.f) {
			effect.mTransparency = 1.f- effect.mTransparency;
			mat.AddProperty( &effect.mTransparency, 1, AI_MATKEY_OPACITY );
			mat.AddProperty( &effect.mTransparent, 1, AI_MATKEY_COLOR_TRANSPARENT );
		}

		// add textures, if given
		if( !effect.mTexAmbient.mName.empty()) 
			 /* It is merely a lightmap */
			AddTexture( mat, pParser, effect, effect.mTexAmbient, aiTextureType_LIGHTMAP);

		if( !effect.mTexEmissive.mName.empty())
			AddTexture( mat, pParser, effect, effect.mTexEmissive, aiTextureType_EMISSIVE);

		if( !effect.mTexSpecular.mName.empty())
			AddTexture( mat, pParser, effect, effect.mTexSpecular, aiTextureType_SPECULAR);

		if( !effect.mTexDiffuse.mName.empty())
			AddTexture( mat, pParser, effect, effect.mTexDiffuse, aiTextureType_DIFFUSE);

		if( !effect.mTexBump.mName.empty())
			AddTexture( mat, pParser, effect, effect.mTexBump, aiTextureType_NORMALS);

		if( !effect.mTexTransparent.mName.empty())
			AddTexture( mat, pParser, effect, effect.mTexTransparent, aiTextureType_OPACITY);

		if( !effect.mTexReflective.mName.empty())
			AddTexture( mat, pParser, effect, effect.mTexReflective, aiTextureType_REFLECTION);
	}
}

// ------------------------------------------------------------------------------------------------
// Constructs materials from the collada material definitions
void ColladaLoader::BuildMaterials( ColladaParser& pParser, aiScene* /*pScene*/)
{
	newMats.reserve(pParser.mMaterialLibrary.size());

	for( ColladaParser::MaterialLibrary::const_iterator matIt = pParser.mMaterialLibrary.begin(); matIt != pParser.mMaterialLibrary.end(); ++matIt)
	{
		const Collada::Material& material = matIt->second;
		// a material is only a reference to an effect
		ColladaParser::EffectLibrary::iterator effIt = pParser.mEffectLibrary.find( material.mEffect);
		if( effIt == pParser.mEffectLibrary.end())
			continue;
		Collada::Effect& effect = effIt->second;

		// create material
		aiMaterial* mat = new aiMaterial;
		aiString name( matIt->first);
		mat->AddProperty(&name,AI_MATKEY_NAME);

		// store the material
		mMaterialIndexByName[matIt->first] = newMats.size();
		newMats.push_back( std::pair<Collada::Effect*, aiMaterial*>( &effect,mat) );
	}
	// ScenePreprocessor generates a default material automatically if none is there.
	// All further code here in this loader works well without a valid material so
	// we can safely let it to ScenePreprocessor.
#if 0
	if( newMats.size() == 0)
	{
		aiMaterial* mat = new aiMaterial;
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
#endif
}

// ------------------------------------------------------------------------------------------------
// Resolves the texture name for the given effect texture entry
aiString ColladaLoader::FindFilenameForEffectTexture( const ColladaParser& pParser,
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
	if( imIt == pParser.mImageLibrary.end()) 
	{
		throw DeadlyImportError( boost::str( boost::format( 
			"Collada: Unable to resolve effect texture entry \"%s\", ended up at ID \"%s\".") % pName % name));
	}

	aiString result;

	// if this is an embedded texture image setup an aiTexture for it
	if (imIt->second.mFileName.empty()) 
	{
		if (imIt->second.mImageData.empty())  {
			throw DeadlyImportError("Collada: Invalid texture, no data or file reference given");
		}

		aiTexture* tex = new aiTexture();

		// setup format hint
		if (imIt->second.mEmbeddedFormat.length() > 3) {
			DefaultLogger::get()->warn("Collada: texture format hint is too long, truncating to 3 characters");
		}
		strncpy(tex->achFormatHint,imIt->second.mEmbeddedFormat.c_str(),3);

		// and copy texture data
		tex->mHeight = 0;
		tex->mWidth = imIt->second.mImageData.size();
		tex->pcData = (aiTexel*)new char[tex->mWidth];
		memcpy(tex->pcData,&imIt->second.mImageData[0],tex->mWidth);

		// setup texture reference string
		result.data[0] = '*';
		result.length = 1 + ASSIMP_itoa10(result.data+1,MAXLEN-1,mTextures.size());

		// and add this texture to the list
		mTextures.push_back(tex);
	}
	else 
	{
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
	if (0 == strncmp(ss.data,"file://",7)) 
	{
		ss.length -= 7;
		memmove(ss.data,ss.data+7,ss.length);
		ss.data[ss.length] = '\0';
	}

  // Maxon Cinema Collada Export writes "file:///C:\andsoon" with three slashes... 
  // I need to filter it without destroying linux paths starting with "/somewhere"
  if( ss.data[0] == '/' && isalpha( ss.data[1]) && ss.data[2] == ':' )
  {
    ss.length--;
    memmove( ss.data, ss.data+1, ss.length);
    ss.data[ss.length] = 0;
  }

  // find and convert all %xy special chars
  char* out = ss.data;
  for( const char* it = ss.data; it != ss.data + ss.length; /**/ )
  {
    if( *it == '%' && (it + 3) < ss.data + ss.length )
    {
      // separate the number to avoid dragging in chars from behind into the parsing
      char mychar[3] = { it[1], it[2], 0 };
      size_t nbr = strtoul16( mychar);
      it += 3;
      *out++ = (char)(nbr & 0xFF);
    } else
    {
      *out++ = *it++;
    }
  }

  // adjust length and terminator of the shortened string
  *out = 0;
  ss.length = (ptrdiff_t) (out - ss.data);
}

// ------------------------------------------------------------------------------------------------
// Reads a float value from an accessor and its data array.
float ColladaLoader::ReadFloat( const Collada::Accessor& pAccessor, const Collada::Data& pData, size_t pIndex, size_t pOffset) const
{
	// FIXME: (thom) Test for data type here in every access? For the moment, I leave this to the caller
	size_t pos = pAccessor.mStride * pIndex + pAccessor.mOffset + pOffset;
	ai_assert( pos < pData.mValues.size());
	return pData.mValues[pos];
}

// ------------------------------------------------------------------------------------------------
// Reads a string value from an accessor and its data array.
const std::string& ColladaLoader::ReadString( const Collada::Accessor& pAccessor, const Collada::Data& pData, size_t pIndex) const
{
	size_t pos = pAccessor.mStride * pIndex + pAccessor.mOffset;
	ai_assert( pos < pData.mStrings.size());
	return pData.mStrings[pos];
}

// ------------------------------------------------------------------------------------------------
// Collects all nodes into the given array
void ColladaLoader::CollectNodes( const aiNode* pNode, std::vector<const aiNode*>& poNodes) const
{
	poNodes.push_back( pNode);

	for( size_t a = 0; a < pNode->mNumChildren; ++a)
		CollectNodes( pNode->mChildren[a], poNodes);
}

// ------------------------------------------------------------------------------------------------
// Finds a node in the collada scene by the given name
const Collada::Node* ColladaLoader::FindNode( const Collada::Node* pNode, const std::string& pName) const
{
	if( pNode->mName == pName || pNode->mID == pName)
		return pNode;

	for( size_t a = 0; a < pNode->mChildren.size(); ++a)
	{
		const Collada::Node* node = FindNode( pNode->mChildren[a], pName);
		if( node)
			return node;
	}

	return NULL;
}

// ------------------------------------------------------------------------------------------------
// Finds a node in the collada scene by the given SID
const Collada::Node* ColladaLoader::FindNodeBySID( const Collada::Node* pNode, const std::string& pSID) const
{
  if( pNode->mSID == pSID)
    return pNode;

  for( size_t a = 0; a < pNode->mChildren.size(); ++a)
  {
    const Collada::Node* node = FindNodeBySID( pNode->mChildren[a], pSID);
    if( node)
      return node;
  }

  return NULL;
}

// ------------------------------------------------------------------------------------------------
// Finds a proper name for a node derived from the collada-node's properties
std::string ColladaLoader::FindNameForNode( const Collada::Node* pNode) const
{
	// now setup the name of the node. We take the name if not empty, otherwise the collada ID
	// FIX: Workaround for XSI calling the instanced visual scene 'untitled' by default.
	if (!pNode->mName.empty() && pNode->mName != "untitled")
		return pNode->mName;
	else if (!pNode->mID.empty())
		return pNode->mID;
	else if (!pNode->mSID.empty())
    return pNode->mSID;
  else
	{
		// No need to worry. Unnamed nodes are no problem at all, except
		// if cameras or lights need to be assigned to them.
    return boost::str( boost::format( "$ColladaAutoName$_%d") % clock());
	}
}

#endif // !! ASSIMP_BUILD_NO_DAE_IMPORTER
