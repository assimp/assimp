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

/** @file Implementation of the ASE importer class */


#include "AssimpPCH.h"

// internal headers
#include "ASELoader.h"
#include "MaterialSystem.h"
#include "StringComparison.h"
#include "TextureTransform.h"
#include "SkeletonMeshBuilder.h"

// utilities
#include "fast_atof.h"
#include "qnan.h"

using namespace Assimp;
using namespace Assimp::ASE;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
ASEImporter::ASEImporter()
{
}
// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
ASEImporter::~ASEImporter()
{
}
// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool ASEImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
{
	// simple check of file extension is enough for the moment
	std::string::size_type pos = pFile.find_last_of('.');
	// no file extension - can't read
	if( pos == std::string::npos)
		return false;
	std::string extension = pFile.substr( pos);

	// Either ASE, ASC or ASK
	return  !(extension.length() < 4 || extension[0] != '.' ||
			  extension[1] != 'a' && extension[1] != 'A' ||
			  extension[2] != 's' && extension[2] != 'S' ||
			  extension[3] != 'e' && extension[3] != 'E' &&
			  extension[3] != 'k' && extension[3] != 'K' &&
			  extension[3] != 'c' && extension[3] != 'C');
}

// ------------------------------------------------------------------------------------------------
// Setup configuration options
void ASEImporter::SetupProperties(const Importer* pImp)
{
	configRecomputeNormals = (pImp->GetPropertyInteger(
		AI_CONFIG_IMPORT_ASE_RECONSTRUCT_NORMALS,0) ? true : false);
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void ASEImporter::InternReadFile( const std::string& pFile, 
	aiScene* pScene, IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile, "rb"));

	// Check whether we can read from the file
	if( file.get() == NULL)
		throw new ImportErrorException( "Failed to open ASE file " + pFile + ".");

	size_t fileSize = file->FileSize();

	// allocate storage and copy the contents of the file to a memory buffer
	// (terminate it with zero)
	std::vector<char> mBuffer2(fileSize+1);
	file->Read( &mBuffer2[0], 1, fileSize);
	mBuffer2[fileSize] = '\0';

	this->mBuffer = &mBuffer2[0];
	this->pcScene = pScene;

	// construct an ASE parser and parse the file
	// TODO: clean this up, mParser should be a reference, not a pointer ...
	ASE::Parser parser(this->mBuffer);
	mParser = &parser;
	mParser->Parse();

	// Check whether we loaded at least one mesh. If we did - generate
	// materials and copy meshes. 
	if ( !mParser->m_vMeshes.empty())
	{
		// if absolutely no material has been loaded from the file
		// we need to generate a default material
		GenerateDefaultMaterial();

		// process all meshes
		bool tookNormals = false;
		std::vector<aiMesh*> avOutMeshes;
		avOutMeshes.reserve(mParser->m_vMeshes.size()*2);
		for (std::vector<ASE::Mesh>::iterator
			i =  mParser->m_vMeshes.begin();
			i != mParser->m_vMeshes.end();++i)
		{
			if ((*i).bSkip)continue;

			// now we need to create proper meshes from the import we 
			// need to split them by materials, build valid vertex/
			// face lists ...
			BuildUniqueRepresentation(*i);

			// need to generate proper vertex normals if necessary
			if(GenerateNormals(*i))
				tookNormals = true;

			// convert all meshes to aiMesh objects
			ConvertMeshes(*i,avOutMeshes);
		}
		if (tookNormals)
		{
			DefaultLogger::get()->debug("ASE: Taking normals from the file. Use "
				"the AI_CONFIG_IMPORT_ASE_RECONSTRUCT_NORMALS option if you "
				"experience problems");
		}

		// now build the output mesh list. Remove dummies
		pScene->mNumMeshes = (unsigned int)avOutMeshes.size();
		aiMesh** pp = pScene->mMeshes = new aiMesh*[pScene->mNumMeshes];
		for (std::vector<aiMesh*>::const_iterator
			i =  avOutMeshes.begin();
			i != avOutMeshes.end();++i)
		{
			if (!(*i)->mNumFaces)continue;
			*pp++ = *i;
		}
		pScene->mNumMeshes = (unsigned int)(pp - pScene->mMeshes);

		// build final material indices (remove submaterials and setup
		// the final list)
		BuildMaterialIndices();
	}

	// Copy all scene graph nodes - lights, cameras, dummies and meshes
	// into one large array
	nodes.reserve(mParser->m_vMeshes.size() +mParser->m_vLights.size()
		+ mParser->m_vCameras.size() + mParser->m_vDummies.size());

	for (std::vector<ASE::Light>::iterator it = mParser->m_vLights.begin(), 
		 end = mParser->m_vLights.end();it != end; ++it)nodes.push_back(&(*it));
	for (std::vector<ASE::Camera>::iterator it = mParser->m_vCameras.begin(), 
		 end = mParser->m_vCameras.end();it != end; ++it)nodes.push_back(&(*it));
	for (std::vector<ASE::Mesh>::iterator it = mParser->m_vMeshes.begin(),
		end = mParser->m_vMeshes.end();it != end; ++it)nodes.push_back(&(*it));
		for (std::vector<ASE::Dummy>::iterator it = mParser->m_vDummies.begin(),
		end = mParser->m_vDummies.end();it != end; ++it)nodes.push_back(&(*it));

	// process target cameras and target lights (
	// generate animation channels for them and adjust the node graph)
	// ProcessTargets();

	// build the final node graph
	BuildNodes();

	// build output animations
	BuildAnimations();

	// build output cameras
	BuildCameras();

	// build output lights
	BuildLights();

	// TODO: STRANGE RESULTS ATM
	// If we have no meshes use the SkeletonMeshBuilder helper class
	// to build a mesh for the animation skeleton
	if (!pScene->mNumMeshes)
	{
		pScene->mFlags |= AI_SCENE_FLAGS_INCOMPLETE;
		SkeletonMeshBuilder skeleton(pScene);
	}
}
// ------------------------------------------------------------------------------------------------
void ASEImporter::GenerateDefaultMaterial()
{
	ai_assert(NULL != mParser);

	bool bHas = false;
	for (std::vector<ASE::Mesh>::iterator
		i =  mParser->m_vMeshes.begin();
		i != mParser->m_vMeshes.end();++i)
	{
		if ((*i).bSkip)continue;
		if (ASE::Face::DEFAULT_MATINDEX == (*i).iMaterialIndex)
		{
			(*i).iMaterialIndex = (unsigned int)mParser->m_vMaterials.size();
			bHas = true;
		}
	}
	if (bHas || mParser->m_vMaterials.empty())
	{
		// add a simple material without submaterials to the parser's list
		mParser->m_vMaterials.push_back ( ASE::Material() );
		ASE::Material& mat = mParser->m_vMaterials.back();

		mat.mDiffuse  = aiColor3D(0.6f,0.6f,0.6f);
		mat.mSpecular = aiColor3D(1.0f,1.0f,1.0f);
		mat.mAmbient  = aiColor3D(0.05f,0.05f,0.05f);
		mat.mShading  = Discreet3DS::Gouraud;
		mat.mName     = AI_DEFAULT_MATERIAL_NAME;
	}
}

// ------------------------------------------------------------------------------------------------
void ASEImporter::BuildAnimations()
{
	// check whether we have at least one mesh which has animations
	std::vector<ASE::BaseNode*>::iterator i =  nodes.begin();
	unsigned int iNum = 0;
	for (;i != nodes.end();++i)
	{
		// TODO: Implement Bezier & TCB support
		if ((*i)->mAnim.mPositionType != ASE::Animation::TRACK)
		{
			DefaultLogger::get()->warn("ASE: Position controller uses Bezier/TCB keys. "
				"This is not supported.");
		}
		if ((*i)->mAnim.mRotationType != ASE::Animation::TRACK)
		{
			DefaultLogger::get()->warn("ASE: Rotation controller uses Bezier/TCB keys. "
				"This is not supported.");
		}
		if ((*i)->mAnim.mScalingType != ASE::Animation::TRACK)
		{
			DefaultLogger::get()->warn("ASE: Position controller uses Bezier/TCB keys. "
				"This is not supported.");
		}

		// We compare against 1 here - firstly one key is not
		// really an animation and secondly MAX writes dummies
		// that represent the node transformation.
		if ((*i)->mAnim.akeyPositions.size() > 1 || 
			(*i)->mAnim.akeyRotations.size() > 1 ||
			(*i)->mAnim.akeyScaling.size()   > 1)
		{
			++iNum;
		}
	}
	if (iNum)
	{
		// Generate a new animation channel and setup everything for it
		pcScene->mNumAnimations = 1;
		pcScene->mAnimations    = new aiAnimation*[1];
		aiAnimation* pcAnim     = pcScene->mAnimations[0] = new aiAnimation();
		pcAnim->mNumChannels    = iNum;
		pcAnim->mChannels       = new aiNodeAnim*[iNum];
		pcAnim->mTicksPerSecond = mParser->iFrameSpeed * mParser->iTicksPerFrame;

		iNum = 0;
		
		// Now iterate through all meshes and collect all data we can find
		for (i =  nodes.begin();i != nodes.end();++i)
		{
			if ((*i)->mAnim.akeyPositions.size() > 1 || (*i)->mAnim.akeyRotations.size() > 1)
			{
				// Begin a new node animation channel for this node
				aiNodeAnim* pcNodeAnim = pcAnim->mChannels[iNum++] = new aiNodeAnim();
				pcNodeAnim->mNodeName.Set((*i)->mName);

				// copy position keys
				if ((*i)->mAnim.akeyPositions.size() > 1 )
				{
					// Allocate the key array and fill it
					pcNodeAnim->mNumPositionKeys = (unsigned int) (*i)->mAnim.akeyPositions.size();
					pcNodeAnim->mPositionKeys    = new aiVectorKey[pcNodeAnim->mNumPositionKeys];

					::memcpy(pcNodeAnim->mPositionKeys,&(*i)->mAnim.akeyPositions[0],
						pcNodeAnim->mNumPositionKeys * sizeof(aiVectorKey));

					// get the longest node anim channel 
					for (unsigned int qq = 0; qq < pcNodeAnim->mNumPositionKeys;++qq)
					{
						pcAnim->mDuration = std::max(pcAnim->mDuration,
							pcNodeAnim->mPositionKeys[qq].mTime);
					}
				}
				// copy rotation keys
				if ((*i)->mAnim.akeyRotations.size() > 1 )
				{
					// Allocate the key array and fill it
					pcNodeAnim->mNumRotationKeys = (unsigned int) (*i)->mAnim.akeyRotations.size();
					pcNodeAnim->mRotationKeys    = new aiQuatKey[pcNodeAnim->mNumRotationKeys];

					::memcpy(pcNodeAnim->mRotationKeys,&(*i)->mAnim.akeyRotations[0],
						pcNodeAnim->mNumRotationKeys * sizeof(aiQuatKey));

					// get the longest node anim channel
					for (unsigned int qq = 0; qq < pcNodeAnim->mNumRotationKeys;++qq)
					{
						pcAnim->mDuration = std::max(pcAnim->mDuration,
							pcNodeAnim->mRotationKeys[qq].mTime);
					}
				}
				// copy scaling keys
				if ((*i)->mAnim.akeyScaling.size() > 1 )
				{
					// Allocate the key array and fill it
					pcNodeAnim->mNumScalingKeys = (unsigned int) (*i)->mAnim.akeyScaling.size();
					pcNodeAnim->mScalingKeys    = new aiVectorKey[pcNodeAnim->mNumScalingKeys];

					::memcpy(pcNodeAnim->mScalingKeys,&(*i)->mAnim.akeyScaling[0],
						pcNodeAnim->mNumScalingKeys * sizeof(aiVectorKey));

					// get the longest node anim channel 
					for (unsigned int qq = 0; qq < pcNodeAnim->mNumScalingKeys;++qq)
					{
						pcAnim->mDuration = std::max(pcAnim->mDuration,
							pcNodeAnim->mScalingKeys[qq].mTime);
					}
				}
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
void ASEImporter::BuildCameras()
{
	if (!mParser->m_vCameras.empty())
	{
		pcScene->mNumCameras = (unsigned int)mParser->m_vCameras.size();
		pcScene->mCameras = new aiCamera*[pcScene->mNumCameras];

		for (unsigned int i = 0; i < pcScene->mNumCameras;++i)
		{
			aiCamera* out   = pcScene->mCameras[i] = new aiCamera();
			ASE::Camera& in = mParser->m_vCameras[i];

			// copy members
			out->mClipPlaneFar  = in.mFar;
			out->mClipPlaneNear = (in.mNear ? in.mNear : 0.1f); 
			out->mHorizontalFOV = in.mFOV;

			// TODO: Implement proper camera target

			out->mName.Set(in.mName);
		}
	}
}

// ------------------------------------------------------------------------------------------------
void ASEImporter::BuildLights()
{
	if (!mParser->m_vLights.empty())
	{
		pcScene->mNumLights = (unsigned int)mParser->m_vLights.size();
		pcScene->mLights    = new aiLight*[pcScene->mNumLights];

		for (unsigned int i = 0; i < pcScene->mNumLights;++i)
		{
			aiLight* out   = pcScene->mLights[i] = new aiLight();
			ASE::Light& in = mParser->m_vLights[i];

			// The direction is encoded in the transformation
			// matrix of the node. In 3DS MAX the light source
			// points in negative Z direction if the node 
			// transformation is the identity. 
			out->mDirection = aiVector3D(0.f,0.f,-1.f);

			out->mName.Set(in.mName);
			switch (in.mLightType)
			{
			case ASE::Light::TARGET:
				out->mType = aiLightSource_SPOT;
				out->mAngleInnerCone = AI_DEG_TO_RAD(in.mAngle);
				out->mAngleOuterCone = (in.mFalloff ? AI_DEG_TO_RAD(in.mFalloff) 
					: out->mAngleInnerCone);
				break;

			case ASE::Light::DIRECTIONAL:

				out->mType = aiLightSource_DIRECTIONAL;
				break;

			default:
			//case ASE::Light::OMNI:
				out->mType = aiLightSource_POINT;
				break;
			};
			out->mColorDiffuse = out->mColorSpecular = in.mColor * in.mIntensity;
		}
	}
}

// ------------------------------------------------------------------------------------------------
void ASEImporter::AddNodes(std::vector<BaseNode*>& nodes,
	aiNode* pcParent,const char* szName)
{
	aiMatrix4x4 m;
	this->AddNodes(nodes,pcParent,szName,m);
}

// ------------------------------------------------------------------------------------------------
void ASEImporter::AddMeshes(const ASE::BaseNode* snode,aiNode* node)
{
	for (unsigned int i = 0; i < pcScene->mNumMeshes;++i)
	{
		// Get the name of the mesh (the mesh instance has been temporarily
		// stored in the third vertex color)
		const aiMesh* pcMesh  = pcScene->mMeshes[i];
		const ASE::Mesh* mesh = (const ASE::Mesh*)pcMesh->mColors[2];

		if (mesh == snode)++node->mNumMeshes;
	}

	if(node->mNumMeshes)
	{
		node->mMeshes = new unsigned int[node->mNumMeshes];
		for (unsigned int i = 0, p = 0; i < pcScene->mNumMeshes;++i)
		{
			const aiMesh* pcMesh  = pcScene->mMeshes[i];
			const ASE::Mesh* mesh = (const ASE::Mesh*)pcMesh->mColors[2];
			if (mesh == snode)
			{
				node->mMeshes[p++] = i;

				// Transform all vertices of the mesh back into their local space -> 
				// at the moment they are pretransformed
				aiMatrix4x4 m  = mesh->mTransform;
				m.Inverse();

				aiVector3D* pvCurPtr = pcMesh->mVertices;
				const aiVector3D* pvEndPtr = pvCurPtr + pcMesh->mNumVertices;
				while (pvCurPtr != pvEndPtr)
				{
					*pvCurPtr = m * (*pvCurPtr);
					pvCurPtr++;
				}

				// Do the same for the normal vectors if we have them
				// Here we need to use the (Inverse)Transpose of a 3x3
				// matrix without the translational component.
				if (pcMesh->mNormals)
				{
					aiMatrix3x3 m3 = aiMatrix3x3( mesh->mTransform );
					m3.Transpose();

					pvCurPtr = pcMesh->mNormals;
					pvEndPtr = pvCurPtr + pcMesh->mNumVertices;
					while (pvCurPtr != pvEndPtr)
					{
						*pvCurPtr = m3 * (*pvCurPtr);
						pvCurPtr++;
					}
				}
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
void ASEImporter::AddNodes (std::vector<BaseNode*>& nodes,
	aiNode* pcParent, const char* szName,
	const aiMatrix4x4& mat)
{
	const size_t len = szName ? ::strlen(szName) : 0;
	ai_assert(4 <= AI_MAX_NUMBER_OF_COLOR_SETS);

	// Receives child nodes for the pcParent node
	std::vector<aiNode*> apcNodes;

	// Now iterate through all nodes in the scene and search for one
	// which has *us* as parent.
	for (std::vector<BaseNode*>::const_iterator it = nodes.begin(), end = nodes.end();
		 it != end; ++it)
	{
		const BaseNode* snode = *it;
		if (szName)
		{
			if (len != snode->mParent.length() || ::strcmp(szName,snode->mParent.c_str()))
				continue;
		}
		else if (snode->mParent.length())
			continue;

		(*it)->mProcessed = true;

		// Allocate a new node and add it to the output data structure
		apcNodes.push_back(new aiNode());
		aiNode* node = apcNodes.back();

		node->mName.Set((snode->mName.length() ? snode->mName.c_str() : "Unnamed_Node"));
		node->mParent = pcParent;

		// Setup the transformation matrix of the node
		aiMatrix4x4 mParentAdjust  = mat;
		mParentAdjust.Inverse();
		node->mTransformation = mParentAdjust*snode->mTransform;

		// If the type of this node is "Mesh" we need to search
		// the list of output meshes in the data structure for
		// all those that belonged to this node once. This is
		// slightly inconvinient here and a better solution should
		// be used when this code is refactored next.
		if (snode->mType == BaseNode::Mesh)
		{
			AddMeshes(snode,node);
		}

		// add sub nodes
		// aiMatrix4x4 mNewAbs =  mat * node->mTransformation;

		// prevent stack overflow
		if (node->mName != node->mParent->mName)
		{
			AddNodes(nodes,node,node->mName.data,snode->mTransform);
		}
	}

	// allocate enough space for the child nodes
	pcParent->mNumChildren = (unsigned int)apcNodes.size();
	pcParent->mChildren = new aiNode*[apcNodes.size()];

	// now build all nodes for our nice new children
	for (unsigned int p = 0; p < apcNodes.size();++p)
	{
		pcParent->mChildren[p] = apcNodes[p];
	}
	return;
}

// ------------------------------------------------------------------------------------------------
void ASEImporter::BuildNodes()
{
	ai_assert(NULL != pcScene);

	// allocate the one and only root node
	pcScene->mRootNode = new aiNode();
	pcScene->mRootNode->mNumMeshes = 0;
	pcScene->mRootNode->mMeshes = 0;
	pcScene->mRootNode->mName.Set("<root>");

	// Setup the coordinate system transformation
	pcScene->mRootNode->mTransformation.c3 *= -1.f;
	pcScene->mRootNode->mNumChildren = 1;
	pcScene->mRootNode->mChildren = new aiNode*[1];
	pcScene->mRootNode->mChildren[0] = new aiNode();

	// Change the transformation matrix of all nodes
	for (std::vector<BaseNode*>::iterator it = nodes.begin(), end = nodes.end();
		 it != end; ++it)
	{
		aiMatrix4x4& m = (*it)->mTransform;
		m.Transpose(); // row-order vs column-order
	}

	// add all nodes
	AddNodes(nodes,pcScene->mRootNode->mChildren[0],NULL);

	// now iterate through al nodes and find those that have not yet
	// been added to the nodegraph (= their parent could not be recognized)
	std::vector<const BaseNode*> aiList;
	for (std::vector<BaseNode*>::iterator it = nodes.begin(), end = nodes.end();
		 it != end; ++it)
	{
		if ((*it)->mProcessed)continue;

		// check whether our parent is known
		bool bKnowParent = false;
		
		// research the list, beginning from now and try to find out whether
		// there is a node that references *us* as a parent
		for (std::vector<BaseNode*>::const_iterator it2 = nodes.begin();
			 it2 != end; ++it2)
		{
			if (it2 == it)continue;

			if ((*it2)->mName == (*it)->mParent)
			{
				bKnowParent = true;
				break;
			}
		}
		if (!bKnowParent)
		{
			aiList.push_back(*it);
		}
	}

	// Are there ane orphaned nodes?
	if (!aiList.empty())
	{
		std::vector<aiNode*> apcNodes;
		apcNodes.reserve(aiList.size() + pcScene->mRootNode->mNumChildren);

		for (unsigned int i = 0; i < pcScene->mRootNode->mNumChildren;++i)
			apcNodes.push_back(pcScene->mRootNode->mChildren[i]);

		delete[] pcScene->mRootNode->mChildren;
		for (std::vector<const BaseNode*>::/*const_*/iterator
			i =  aiList.begin();
			i != aiList.end();++i)
		{
			const ASE::BaseNode* src = *i;

			// the parent is not known, so we can assume that we must add 
			// this node to the root node of the whole scene
			aiNode* pcNode = new aiNode();
			pcNode->mParent = pcScene->mRootNode;
			pcNode->mName.Set(src->mName);
			AddMeshes(src,pcNode);
			AddNodes(nodes,pcNode,pcNode->mName.data);
			apcNodes.push_back(pcNode);
		}

		// Regenerate our output array
		pcScene->mRootNode->mChildren = new aiNode*[apcNodes.size()];
		for (unsigned int i = 0; i < apcNodes.size();++i)
			pcScene->mRootNode->mChildren[i] = apcNodes[i];

		pcScene->mRootNode->mNumChildren = (unsigned int)apcNodes.size();
	}

	// Reset the third color set to NULL - we used this field to
	// store a temporary pointer
	for (unsigned int i = 0; i < pcScene->mNumMeshes;++i)
		pcScene->mMeshes[i]->mColors[2] = NULL;

	// if there is only one subnode, set it as root node
	// FIX: The sub node may not have animations assigned
	if (1 == pcScene->mRootNode->mNumChildren && !pcScene->mNumAnimations)
	{
		aiNode* cc = pcScene->mRootNode->mChildren[0];
		aiNode* pc = pcScene->mRootNode;

		pcScene->mRootNode = cc;
		pcScene->mRootNode->mParent = NULL;
		cc->mTransformation = pc->mTransformation * cc->mTransformation;

		// make sure the destructor won't delete us ...
		delete[] pc->mChildren;
		pc->mChildren = NULL;
		pc->mNumChildren = 0;
		delete pc;
	}
	// The root node should not have at least one child or the file is invalid
	else if (!pcScene->mRootNode->mNumChildren)
	{
		throw new ImportErrorException("No nodes loaded. The ASE/ASK file is either empty or corrupt");
	}
	return;
}

// ------------------------------------------------------------------------------------------------
void ASEImporter::BuildUniqueRepresentation(ASE::Mesh& mesh)
{
	// allocate output storage
	std::vector<aiVector3D> mPositions;
	std::vector<aiVector3D> amTexCoords[AI_MAX_NUMBER_OF_TEXTURECOORDS];
	std::vector<aiColor4D>  mVertexColors;
	std::vector<aiVector3D> mNormals;
	std::vector<BoneVertex> mBoneVertices;

	unsigned int iSize = (unsigned int)mesh.mFaces.size() * 3;
	mPositions.resize(iSize);

	// optional texture coordinates
	for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS;++i)
	{
		if (!mesh.amTexCoords[i].empty())
		{
			amTexCoords[i].resize(iSize);
		}
	}
	// optional vertex colors
	if (!mesh.mVertexColors.empty())
	{
		mVertexColors.resize(iSize);
	}

	// optional vertex normals (vertex normals can simply be copied)
	if (!mesh.mNormals.empty())
	{
		mNormals.resize(iSize);
	}
	// bone vertices. There is no need to change the bone list
	if (!mesh.mBoneVertices.empty())
	{
		mBoneVertices.resize(iSize);
	}

	// iterate through all faces in the mesh
	unsigned int iCurrent = 0, fi = 0;
	for (std::vector<ASE::Face>::iterator
		i =  mesh.mFaces.begin();
		i != mesh.mFaces.end();++i,++fi)
	{
		for (unsigned int n = 0; n < 3;++n,++iCurrent)
		{
			mPositions[iCurrent] = mesh.mPositions[(*i).mIndices[n]];
			//std::swap((float&)mPositions[iCurrent].z,(float&)mPositions[iCurrent].y); // DX-to-OGL
			//mPositions[iCurrent].y *= -1.f;

			// add texture coordinates
			for (unsigned int c = 0; c < AI_MAX_NUMBER_OF_TEXTURECOORDS;++c)
			{
				if (!mesh.amTexCoords[c].empty())
				{
					amTexCoords[c][iCurrent] = mesh.amTexCoords[c][(*i).amUVIndices[c][n]];
					amTexCoords[c][iCurrent].y = 1.0f - amTexCoords[c][iCurrent].y; // DX-to-OGL
				}
			}
			// add vertex colors
			if (!mesh.mVertexColors.empty())
			{
				mVertexColors[iCurrent] = mesh.mVertexColors[(*i).mColorIndices[n]];
			}
			// add normal vectors
			if (!mesh.mNormals.empty())
			{
				mNormals[iCurrent] = mesh.mNormals[fi*3+n];
				mNormals[iCurrent].Normalize();

				//std::swap((float&)mNormals[iCurrent].z,(float&)mNormals[iCurrent].y); // DX-to-OGL
				//mNormals[iCurrent].y *= -1.0f;
			}

			// handle bone vertices
			if ((*i).mIndices[n] < mesh.mBoneVertices.size())
			{
				// (sometimes this will cause bone verts to be duplicated
				//  however, I' quite sure Schrompf' JoinVerticesStep
				//  will fix that again ...)
				mBoneVertices[iCurrent] =  mesh.mBoneVertices[(*i).mIndices[n]];
			}
		}
		// we need to flip the order of the indices
		(*i).mIndices[0] = iCurrent-1;
		(*i).mIndices[1] = iCurrent-2;
		(*i).mIndices[2] = iCurrent-3;
	}

	// replace the old arrays
	mesh.mNormals = mNormals;
	mesh.mPositions = mPositions;
	mesh.mVertexColors = mVertexColors;

	for (unsigned int c = 0; c < AI_MAX_NUMBER_OF_TEXTURECOORDS;++c)
		mesh.amTexCoords[c] = amTexCoords[c];
	return;
}

// ------------------------------------------------------------------------------------------------
void ASEImporter::ConvertMaterial(ASE::Material& mat)
{
	// allocate the output material
	mat.pcInstance = new MaterialHelper();

	// At first add the base ambient color of the
	// scene to	the material
	mat.mAmbient.r += this->mParser->m_clrAmbient.r;
	mat.mAmbient.g += this->mParser->m_clrAmbient.g;
	mat.mAmbient.b += this->mParser->m_clrAmbient.b;

	aiString name;
	name.Set( mat.mName);
	mat.pcInstance->AddProperty( &name, AI_MATKEY_NAME);

	// material colors
	mat.pcInstance->AddProperty( &mat.mAmbient, 1, AI_MATKEY_COLOR_AMBIENT);
	mat.pcInstance->AddProperty( &mat.mDiffuse, 1, AI_MATKEY_COLOR_DIFFUSE);
	mat.pcInstance->AddProperty( &mat.mSpecular, 1, AI_MATKEY_COLOR_SPECULAR);
	mat.pcInstance->AddProperty( &mat.mEmissive, 1, AI_MATKEY_COLOR_EMISSIVE);

	// shininess
	if (0.0f != mat.mSpecularExponent && 0.0f != mat.mShininessStrength)
	{
		mat.pcInstance->AddProperty( &mat.mSpecularExponent, 1, AI_MATKEY_SHININESS);
		mat.pcInstance->AddProperty( &mat.mShininessStrength, 1, AI_MATKEY_SHININESS_STRENGTH);
	}
	// if there is no shininess, we can disable phong lighting
	else if (D3DS::Discreet3DS::Metal == mat.mShading ||
		D3DS::Discreet3DS::Phong == mat.mShading ||
		D3DS::Discreet3DS::Blinn == mat.mShading)
	{
		mat.mShading = D3DS::Discreet3DS::Gouraud;
	}

	// opacity
	mat.pcInstance->AddProperty<float>( &mat.mTransparency,1,AI_MATKEY_OPACITY);


	// shading mode
	aiShadingMode eShading = aiShadingMode_NoShading;
	switch (mat.mShading)
	{
		case D3DS::Discreet3DS::Flat:
			eShading = aiShadingMode_Flat; break;
		case D3DS::Discreet3DS::Phong :
			eShading = aiShadingMode_Phong; break;
		case D3DS::Discreet3DS::Blinn :
			eShading = aiShadingMode_Blinn; break;

		// I don't know what "Wire" shading should be,
		// assume it is simple lambertian diffuse (L dot N) shading
		case D3DS::Discreet3DS::Wire:
		case D3DS::Discreet3DS::Gouraud:
			eShading = aiShadingMode_Gouraud; break;
		case D3DS::Discreet3DS::Metal :
			eShading = aiShadingMode_CookTorrance; break;
	}
	mat.pcInstance->AddProperty<int>( (int*)&eShading,1,AI_MATKEY_SHADING_MODEL);

	if (D3DS::Discreet3DS::Wire == mat.mShading)
	{
		// set the wireframe flag
		unsigned int iWire = 1;
		mat.pcInstance->AddProperty<int>( (int*)&iWire,1,AI_MATKEY_ENABLE_WIREFRAME);
	}

	// texture, if there is one
	if( mat.sTexDiffuse.mMapName.length() > 0)
	{
		aiString tex;
		tex.Set( mat.sTexDiffuse.mMapName);
		mat.pcInstance->AddProperty( &tex, AI_MATKEY_TEXTURE_DIFFUSE(0));

		if (is_not_qnan(mat.sTexDiffuse.mTextureBlend))
			mat.pcInstance->AddProperty<float>( &mat.sTexDiffuse.mTextureBlend, 1, 
			AI_MATKEY_TEXBLEND_DIFFUSE(0));
	}
	if( mat.sTexSpecular.mMapName.length() > 0)
	{
		aiString tex;
		tex.Set( mat.sTexSpecular.mMapName);
		mat.pcInstance->AddProperty( &tex, AI_MATKEY_TEXTURE_SPECULAR(0));

		if (is_not_qnan(mat.sTexSpecular.mTextureBlend))
			mat.pcInstance->AddProperty<float>( &mat.sTexSpecular.mTextureBlend, 1,
			AI_MATKEY_TEXBLEND_SPECULAR(0));
	}
	if( mat.sTexOpacity.mMapName.length() > 0)
	{
		aiString tex;
		tex.Set( mat.sTexOpacity.mMapName);
		mat.pcInstance->AddProperty( &tex, AI_MATKEY_TEXTURE_OPACITY(0));

		if (is_not_qnan(mat.sTexOpacity.mTextureBlend))
			mat.pcInstance->AddProperty<float>( &mat.sTexOpacity.mTextureBlend, 1,
			AI_MATKEY_TEXBLEND_OPACITY(0));
	}
	if( mat.sTexEmissive.mMapName.length() > 0)
	{
		aiString tex;
		tex.Set( mat.sTexEmissive.mMapName);
		mat.pcInstance->AddProperty( &tex, AI_MATKEY_TEXTURE_EMISSIVE(0));

		if (is_not_qnan(mat.sTexEmissive.mTextureBlend))
			mat.pcInstance->AddProperty<float>( &mat.sTexEmissive.mTextureBlend, 1, 
			AI_MATKEY_TEXBLEND_EMISSIVE(0));
	}
	if( mat.sTexAmbient.mMapName.length() > 0)
	{
		aiString tex;
		tex.Set( mat.sTexAmbient.mMapName);
		mat.pcInstance->AddProperty( &tex, AI_MATKEY_TEXTURE_AMBIENT(0));

		if (is_not_qnan(mat.sTexAmbient.mTextureBlend))
			mat.pcInstance->AddProperty<float>( &mat.sTexAmbient.mTextureBlend, 1, 
			AI_MATKEY_TEXBLEND_AMBIENT(0));
	}
	if( mat.sTexBump.mMapName.length() > 0)
	{
		aiString tex;
		tex.Set( mat.sTexBump.mMapName);
		mat.pcInstance->AddProperty( &tex, AI_MATKEY_TEXTURE_HEIGHT(0));

		if (is_not_qnan(mat.sTexBump.mTextureBlend))
			mat.pcInstance->AddProperty<float>( &mat.sTexBump.mTextureBlend, 1,
			AI_MATKEY_TEXBLEND_HEIGHT(0));
	}
	if( mat.sTexShininess.mMapName.length() > 0)
	{
		aiString tex;
		tex.Set( mat.sTexShininess.mMapName);
		mat.pcInstance->AddProperty( &tex, AI_MATKEY_TEXTURE_SHININESS(0));

		if (is_not_qnan(mat.sTexShininess.mTextureBlend))
			mat.pcInstance->AddProperty<float>( &mat.sTexBump.mTextureBlend, 1, 
			AI_MATKEY_TEXBLEND_SHININESS(0));
	}

	// store the name of the material itself, too
	if( mat.mName.length() > 0)
	{
		aiString tex;
		tex.Set( mat.mName);
		mat.pcInstance->AddProperty( &tex, AI_MATKEY_NAME);
	}
	return;
}

// ------------------------------------------------------------------------------------------------
void ASEImporter::ConvertMeshes(ASE::Mesh& mesh, std::vector<aiMesh*>& avOutMeshes)
{
	// validate the material index of the mesh
	if (mesh.iMaterialIndex >= mParser->m_vMaterials.size())
	{
		mesh.iMaterialIndex = (unsigned int)mParser->m_vMaterials.size()-1;
		DefaultLogger::get()->warn("Material index is out of range");
	}

	// if the material the mesh is assigned to is consisting of submeshes
	// we'll need to split it ... Quak.
	if (!mParser->m_vMaterials[mesh.iMaterialIndex].avSubMaterials.empty())
	{
		std::vector<ASE::Material> vSubMaterials = mParser->
			m_vMaterials[mesh.iMaterialIndex].avSubMaterials;

		std::vector<unsigned int>* aiSplit = new std::vector<unsigned int>[
			vSubMaterials.size()];

		// build a list of all faces per submaterial
		for (unsigned int i = 0; i < mesh.mFaces.size();++i)
		{
			// check range
			if (mesh.mFaces[i].iMaterial >= vSubMaterials.size())
				{
					DefaultLogger::get()->warn("Submaterial index is out of range");

					// use the last material instead
					aiSplit[vSubMaterials.size()-1].push_back(i);
				}
			else aiSplit[mesh.mFaces[i].iMaterial].push_back(i);
		}

		// now generate submeshes
		for (unsigned int p = 0; p < vSubMaterials.size();++p)
		{
			if (!aiSplit[p].empty())
			{
				aiMesh* p_pcOut = new aiMesh();
				p_pcOut->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

				// let the sub material index
				p_pcOut->mMaterialIndex = p;

				// we will need this material
				mParser->m_vMaterials[mesh.iMaterialIndex].avSubMaterials[p].bNeed = true;

				// store the real index here ... color channel 3
				p_pcOut->mColors[3] = (aiColor4D*)(uintptr_t)mesh.iMaterialIndex;

				// store a pointer to the mesh in color channel 2
				p_pcOut->mColors[2] = (aiColor4D*) &mesh;
				avOutMeshes.push_back(p_pcOut);

				// convert vertices
				p_pcOut->mNumVertices = (unsigned int)aiSplit[p].size()*3;
				p_pcOut->mNumFaces = (unsigned int)aiSplit[p].size();

				// receive output vertex weights
				std::vector<std::pair<unsigned int, float> >* avOutputBones;
				if (!mesh.mBones.empty())
				{
					avOutputBones = new std::vector<std::pair<unsigned int, float> >[mesh.mBones.size()];
				}
				
				// allocate enough storage for faces
				p_pcOut->mFaces = new aiFace[p_pcOut->mNumFaces];

				unsigned int iBase = 0,iIndex;
				if (p_pcOut->mNumVertices)
				{
					p_pcOut->mVertices = new aiVector3D[p_pcOut->mNumVertices];
					p_pcOut->mNormals  = new aiVector3D[p_pcOut->mNumVertices];
					for (unsigned int q = 0; q < aiSplit[p].size();++q)
					{
					iIndex = aiSplit[p][q];

						p_pcOut->mFaces[q].mIndices = new unsigned int[3];
						p_pcOut->mFaces[q].mNumIndices = 3;

						for (unsigned int t = 0; t < 3;++t)
						{
							const uint32_t iIndex2 = mesh.mFaces[iIndex].mIndices[t];

							p_pcOut->mVertices[iBase] = mesh.mPositions [iIndex2];
							p_pcOut->mNormals [iBase] = mesh.mNormals   [iIndex2];

							// convert bones, if existing
							if (!mesh.mBones.empty())
							{
								// check whether there is a vertex weight that is using
								// this vertex index ...
								if (iIndex2 < mesh.mBoneVertices.size())
								{
									for (std::vector<std::pair<int,float> >::const_iterator
										blubb =  mesh.mBoneVertices[iIndex2].mBoneWeights.begin();
										blubb != mesh.mBoneVertices[iIndex2].mBoneWeights.end();++blubb)
									{
										// NOTE: illegal cases have already been filtered out
										avOutputBones[(*blubb).first].push_back(std::pair<unsigned int, float>(
											iBase,(*blubb).second));
									}
								}
							}
							++iBase;
						}

						// Flip the face order 
						p_pcOut->mFaces[q].mIndices[0] = iBase-3;
						p_pcOut->mFaces[q].mIndices[1] = iBase-2;
						p_pcOut->mFaces[q].mIndices[2] = iBase-1;
					}
				}
				// convert texture coordinates (up to AI_MAX_NUMBER_OF_TEXTURECOORDS sets supported)
				for (unsigned int c = 0; c < AI_MAX_NUMBER_OF_TEXTURECOORDS;++c)
				{
					if (!mesh.amTexCoords[c].empty())
					{
						p_pcOut->mTextureCoords[c] = new aiVector3D[p_pcOut->mNumVertices];
						iBase = 0;
						for (unsigned int q = 0; q < aiSplit[p].size();++q)
						{
							iIndex = aiSplit[p][q];
							for (unsigned int t = 0; t < 3;++t)
							{
								p_pcOut->mTextureCoords[c][iBase++] = mesh.amTexCoords[c][mesh.mFaces[iIndex].mIndices[t]];
							}
						}
						// setup the number of valid vertex components
						p_pcOut->mNumUVComponents[c] = mesh.mNumUVComponents[c];
					}
				}

				// convert vertex colors (only one set supported)
				if (!mesh.mVertexColors.empty())
				{
					p_pcOut->mColors[0] = new aiColor4D[p_pcOut->mNumVertices];
					iBase = 0;
					for (unsigned int q = 0; q < aiSplit[p].size();++q)
					{
						iIndex = aiSplit[p][q];
						for (unsigned int t = 0; t < 3;++t)
						{
							p_pcOut->mColors[0][iBase++] = mesh.mVertexColors[mesh.mFaces[iIndex].mIndices[t]];
						}
					}
				}
				if (!mesh.mBones.empty())
				{
					p_pcOut->mNumBones = 0;
					for (unsigned int mrspock = 0; mrspock < mesh.mBones.size();++mrspock)
						if (!avOutputBones[mrspock].empty())p_pcOut->mNumBones++;

					p_pcOut->mBones = new aiBone* [ p_pcOut->mNumBones ];
					aiBone** pcBone = p_pcOut->mBones;
					for (unsigned int mrspock = 0; mrspock < mesh.mBones.size();++mrspock)
					{
						if (!avOutputBones[mrspock].empty())
						{
							// we will need this bone. add it to the output mesh and
							// add all per-vertex weights
							aiBone* pc = *pcBone = new aiBone();
							pc->mName.Set(mesh.mBones[mrspock].mName);

							pc->mNumWeights = (unsigned int)avOutputBones[mrspock].size();
							pc->mWeights = new aiVertexWeight[pc->mNumWeights];

							for (unsigned int captainkirk = 0; captainkirk < pc->mNumWeights;++captainkirk)
							{
								const std::pair<unsigned int,float>& ref = avOutputBones[mrspock][captainkirk];
								pc->mWeights[captainkirk].mVertexId = ref.first;
								pc->mWeights[captainkirk].mWeight = ref.second;
							}
							++pcBone;
						}
					}
					// delete allocated storage
					delete[] avOutputBones;
				}
			}
		}
		// delete storage
		delete[] aiSplit;
	}
	else
	{
		// Otherwise we can simply copy the data to one output mesh
		// This codepath needs less memory and uses fast memcpy()s
		// to do the actual copying. So I think it is worth the 
		// effort here.

		aiMesh* p_pcOut = new aiMesh();
		p_pcOut->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

		// set an empty sub material index
		p_pcOut->mMaterialIndex = ASE::Face::DEFAULT_MATINDEX;
		mParser->m_vMaterials[mesh.iMaterialIndex].bNeed = true;

		// store the real index here ... in color channel 3
		p_pcOut->mColors[3] = (aiColor4D*)(uintptr_t)mesh.iMaterialIndex;

		// store a pointer to the mesh in color channel 2
		p_pcOut->mColors[2] = (aiColor4D*) &mesh;
		avOutMeshes.push_back(p_pcOut);

		// if the mesh hasn't faces or vertices, there are two cases
		// possible: 1. the model is invalid. 2. This is a dummy
		// helper object which we are going to remove later ...
		if (mesh.mFaces.empty() || mesh.mPositions.empty())
		{
			return;
		}

		// convert vertices
		p_pcOut->mNumVertices = (unsigned int)mesh.mPositions.size();
		p_pcOut->mNumFaces = (unsigned int)mesh.mFaces.size();

		// allocate enough storage for faces
		p_pcOut->mFaces = new aiFace[p_pcOut->mNumFaces];

		// copy vertices
		p_pcOut->mVertices = new aiVector3D[mesh.mPositions.size()];
		memcpy(p_pcOut->mVertices,&mesh.mPositions[0],
			mesh.mPositions.size() * sizeof(aiVector3D));

		// copy normals
		p_pcOut->mNormals = new aiVector3D[mesh.mNormals.size()];
		memcpy(p_pcOut->mNormals,&mesh.mNormals[0],
			mesh.mNormals.size() * sizeof(aiVector3D));

		// copy texture coordinates
		for (unsigned int c = 0; c < AI_MAX_NUMBER_OF_TEXTURECOORDS;++c)
		{
			if (!mesh.amTexCoords[c].empty())
			{
				p_pcOut->mTextureCoords[c] = new aiVector3D[mesh.amTexCoords[c].size()];
				memcpy(p_pcOut->mTextureCoords[c],&mesh.amTexCoords[c][0],
					mesh.amTexCoords[c].size() * sizeof(aiVector3D));

				// setup the number of valid vertex components
				p_pcOut->mNumUVComponents[c] = mesh.mNumUVComponents[c];
			}
		}

		// copy vertex colors
		if (!mesh.mVertexColors.empty())
		{
			p_pcOut->mColors[0] = new aiColor4D[mesh.mVertexColors.size()];
			memcpy(p_pcOut->mColors[0],&mesh.mVertexColors[0],
				mesh.mVertexColors.size() * sizeof(aiColor4D));
		}

		// copy faces
		for (unsigned int iFace = 0; iFace < p_pcOut->mNumFaces;++iFace)
		{
			p_pcOut->mFaces[iFace].mNumIndices = 3;
			p_pcOut->mFaces[iFace].mIndices = new unsigned int[3];

			// copy indices (flip the face order, too)
			p_pcOut->mFaces[iFace].mIndices[0] = mesh.mFaces[iFace].mIndices[2];
			p_pcOut->mFaces[iFace].mIndices[1] = mesh.mFaces[iFace].mIndices[1];
			p_pcOut->mFaces[iFace].mIndices[2] = mesh.mFaces[iFace].mIndices[0];
		}

		// copy vertex bones
		if (!mesh.mBones.empty() && !mesh.mBoneVertices.empty())
		{
			std::vector<aiVertexWeight>* avBonesOut = new
				std::vector<aiVertexWeight>[mesh.mBones.size()];

			// find all vertex weights for this bone
			unsigned int quak = 0;
			for (std::vector<BoneVertex>::const_iterator
				harrypotter =  mesh.mBoneVertices.begin();
				harrypotter != mesh.mBoneVertices.end();++harrypotter,++quak)
			{
				for (std::vector<std::pair<int,float> >::const_iterator
					ronaldweasley  = (*harrypotter).mBoneWeights.begin();
					ronaldweasley != (*harrypotter).mBoneWeights.end();++ronaldweasley)
				{
					aiVertexWeight weight;
					weight.mVertexId = quak;
					weight.mWeight = (*ronaldweasley).second;
					avBonesOut[(*ronaldweasley).first].push_back(weight);
				}
			}

			// now build a final bone list
			p_pcOut->mNumBones = 0;
			for (unsigned int jfkennedy = 0; jfkennedy < mesh.mBones.size();++jfkennedy)
				if (!avBonesOut[jfkennedy].empty())p_pcOut->mNumBones++;

			p_pcOut->mBones = new aiBone*[p_pcOut->mNumBones];
			aiBone** pcBone = p_pcOut->mBones;
			for (unsigned int jfkennedy = 0; jfkennedy < mesh.mBones.size();++jfkennedy)
			{
				if (!avBonesOut[jfkennedy].empty())
				{
					aiBone* pc = *pcBone = new aiBone();
					pc->mName.Set(mesh.mBones[jfkennedy].mName);
					pc->mNumWeights = (unsigned int)avBonesOut[jfkennedy].size();
					pc->mWeights = new aiVertexWeight[pc->mNumWeights];
					::memcpy(pc->mWeights,&avBonesOut[jfkennedy][0],
						sizeof(aiVertexWeight) * pc->mNumWeights);
					++pcBone;
				}
			}

			// delete allocated storage
			delete[] avBonesOut;
		}
	}
}

// ------------------------------------------------------------------------------------------------
void ASEImporter::BuildMaterialIndices()
{
	ai_assert(NULL != pcScene);

	// iterate through all materials and check whether we need them
	for (unsigned int iMat = 0; iMat < mParser->m_vMaterials.size();++iMat)
	{
		if (mParser->m_vMaterials[iMat].bNeed)
		{
			// convert it to the aiMaterial layout
			ASE::Material& mat = mParser->m_vMaterials[iMat];
			ConvertMaterial(mat);
			TextureTransform::ApplyScaleNOffset(mat);
			++pcScene->mNumMaterials;
		}
		for (unsigned int iSubMat = 0; iSubMat < mParser->m_vMaterials[
			iMat].avSubMaterials.size();++iSubMat)
		{
			if (mParser->m_vMaterials[iMat].avSubMaterials[iSubMat].bNeed)
			{
				// convert it to the aiMaterial layout
				ASE::Material& mat = mParser->m_vMaterials[iMat].avSubMaterials[iSubMat];
				ConvertMaterial(mat);
				TextureTransform::ApplyScaleNOffset(mat);
				++pcScene->mNumMaterials;
			}
		}
	}

	// allocate the output material array
	pcScene->mMaterials = new aiMaterial*[pcScene->mNumMaterials];
	D3DS::Material** pcIntMaterials = new D3DS::Material*[pcScene->mNumMaterials];

	unsigned int iNum = 0;
	for (unsigned int iMat = 0; iMat < mParser->m_vMaterials.size();++iMat)
	{
		if (mParser->m_vMaterials[iMat].bNeed)
		{
			ai_assert(NULL != mParser->m_vMaterials[iMat].pcInstance);
			pcScene->mMaterials[iNum] = mParser->m_vMaterials[iMat].pcInstance;

			// store the internal material, too
			pcIntMaterials[iNum] = &mParser->m_vMaterials[iMat];

			// iterate through all meshes and search for one which is using
			// this top-level material index
			for (unsigned int iMesh = 0; iMesh < pcScene->mNumMeshes;++iMesh)
			{
				if (ASE::Face::DEFAULT_MATINDEX == pcScene->mMeshes[iMesh]->mMaterialIndex &&
					iMat == (uintptr_t)pcScene->mMeshes[iMesh]->mColors[3])
				{
					pcScene->mMeshes[iMesh]->mMaterialIndex = iNum;
					pcScene->mMeshes[iMesh]->mColors[3] = NULL;
				}
			}
			iNum++;
		}
		for (unsigned int iSubMat = 0; iSubMat < mParser->m_vMaterials[iMat].avSubMaterials.size();++iSubMat)
		{
			if (mParser->m_vMaterials[iMat].avSubMaterials[iSubMat].bNeed)
			{
				ai_assert(NULL != mParser->m_vMaterials[iMat].avSubMaterials[iSubMat].pcInstance);
				pcScene->mMaterials[iNum] = mParser->m_vMaterials[iMat].
					avSubMaterials[iSubMat].pcInstance;

				// store the internal material, too
				pcIntMaterials[iNum] = &mParser->m_vMaterials[iMat].avSubMaterials[iSubMat];

				// iterate through all meshes and search for one which is using
				// this sub-level material index
				for (unsigned int iMesh = 0; iMesh < pcScene->mNumMeshes;++iMesh)
				{
					if (iSubMat == pcScene->mMeshes[iMesh]->mMaterialIndex &&
						iMat == (uintptr_t)pcScene->mMeshes[iMesh]->mColors[3])
					{
						pcScene->mMeshes[iMesh]->mMaterialIndex = iNum;
						pcScene->mMeshes[iMesh]->mColors[3]     = NULL;
					}
				}
				iNum++;
			}
		}
	}
	// prepare for the next step
	for (unsigned int hans = 0; hans < mParser->m_vMaterials.size();++hans)
		TextureTransform::ApplyScaleNOffset(mParser->m_vMaterials[hans]);

	// now we need to iterate through all meshes,
	// generating correct texture coordinates and material uv indices
	for (unsigned int curie = 0; curie < pcScene->mNumMeshes;++curie)
	{
		aiMesh* pcMesh = pcScene->mMeshes[curie];

		// apply texture coordinate transformations
		TextureTransform::BakeScaleNOffset(pcMesh,pcIntMaterials[pcMesh->mMaterialIndex]);
	}
	for (unsigned int hans = 0; hans < pcScene->mNumMaterials;++hans)
	{
		// setup the correct UV indices for each material
		TextureTransform::SetupMatUVSrc(pcScene->mMaterials[hans],
			pcIntMaterials[hans]);
	}
	delete[] pcIntMaterials;

	// finished!
	return;
}
// ------------------------------------------------------------------------------------------------
// Generate normal vectors basing on smoothing groups
bool ASEImporter::GenerateNormals(ASE::Mesh& mesh)
{
	if (!mesh.mNormals.empty() && !configRecomputeNormals)
	{
		// check whether there are only uninitialized normals. If there are
		// some, skip all normals from the file and compute them on our own
		for (std::vector<aiVector3D>::const_iterator
			qq =  mesh.mNormals.begin();
			qq != mesh.mNormals.end();++qq)
		{
			if ((*qq).x || (*qq).y || (*qq).z)
			{
				return true;
			}
		}
	}
	// The array will be reused
	ComputeNormalsWithSmoothingsGroups<ASE::Face>(mesh);
	return false;
}
