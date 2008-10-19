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

/** @file Implementation of the MD5 importer class */

#include "AssimpPCH.h"

// internal headers
#include "MaterialSystem.h"
#include "RemoveComments.h"
#include "MD5Loader.h"
#include "StringComparison.h"
#include "fast_atof.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
MD5Importer::MD5Importer()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
MD5Importer::~MD5Importer()
{
	// nothing to do here
}
// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool MD5Importer::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
{
	// simple check of file extension is enough for the moment
	std::string::size_type pos = pFile.find_last_of('.');
	// no file extension - can't read
	if( pos == std::string::npos)
		return false;
	std::string extension = pFile.substr( pos);

	if (extension.length() < 4)return false;
	if (extension[0] != '.')return false;

	if (extension[1] != 'm' && extension[1] != 'M')return false;
	if (extension[2] != 'd' && extension[2] != 'D')return false;
	if (extension[3] != '5')return false;
	return true;
}
// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void MD5Importer::InternReadFile( 
	const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler)
{
	// remove the file extension
	std::string::size_type pos = pFile.find_last_of('.');
	this->mFile = pFile.substr(0,pos+1);
	this->pIOHandler = pIOHandler;
	this->pScene = pScene;

	bHadMD5Mesh = bHadMD5Anim = false;

	// load the animation keyframes
	this->LoadMD5AnimFile();

	// load the mesh vertices and bones
	this->LoadMD5MeshFile();

	// make sure we return no incomplete data
	if (!bHadMD5Mesh && !bHadMD5Anim)
		throw new ImportErrorException("Failed to read valid data from this MD5");
	
	if (!bHadMD5Mesh)pScene->mFlags |= AI_SCENE_FLAGS_INCOMPLETE;
}
// ------------------------------------------------------------------------------------------------
void MD5Importer::LoadFileIntoMemory (IOStream* file)
{
	ai_assert(NULL != file);

	this->fileSize = (unsigned int)file->FileSize();

	// allocate storage and copy the contents of the file to a memory buffer
	this->pScene = pScene;
	this->mBuffer = new char[this->fileSize+1];
	file->Read( (void*)mBuffer, 1, this->fileSize);
	this->iLineNumber = 1;

	// append a terminal 0
	this->mBuffer[this->fileSize] = '\0';

	// now remove all line comments from the file
	CommentRemover::RemoveLineComments("//",this->mBuffer,' ');
}
// ------------------------------------------------------------------------------------------------
void MD5Importer::UnloadFileFromMemory ()
{
	// delete the file buffer
	delete[] this->mBuffer;
	this->mBuffer = NULL;
	this->fileSize = 0;
}
// ------------------------------------------------------------------------------------------------
void MakeDataUnique (MD5::MeshDesc& meshSrc)
{
	std::vector<bool> abHad(meshSrc.mVertices.size(),false);

	// allocate enough storage to keep the output structures
	const unsigned int iNewNum = (unsigned int)meshSrc.mFaces.size()*3;
	unsigned int iNewIndex = (unsigned int)meshSrc.mVertices.size();
	meshSrc.mVertices.resize(iNewNum);

	// try to guess how much storage we'll need for new weights
	const float fWeightsPerVert = meshSrc.mWeights.size() / (float)iNewIndex;
	const unsigned int guess = (unsigned int)(fWeightsPerVert*iNewNum); 
	meshSrc.mWeights.reserve(guess + (guess >> 3)); // + 12.5% as buffer

	for (FaceList::const_iterator iter = meshSrc.mFaces.begin(),iterEnd = meshSrc.mFaces.end();
		iter != iterEnd;++iter)
	{
		const aiFace& face = *iter;
		for (unsigned int i = 0; i < 3;++i)
		{
			if (abHad[face.mIndices[i]])
			{
				// generate a new vertex
				meshSrc.mVertices[iNewIndex] = meshSrc.mVertices[face.mIndices[i]];
				face.mIndices[i] = iNewIndex++;

				// FIX: removed this ...
#if 0
				// the algorithm in MD5Importer::LoadMD5MeshFile() doesn't work if
				// a weight is referenced by more than one vertex. This shouldn't 
				// occur in MD5 files, but we must take care that we generate new
				// weights now, too.

				vertNew.mFirstWeight = (unsigned int)meshSrc.mWeights.size();
				meshSrc.mWeights.resize(vertNew.mFirstWeight+vertNew.mNumWeights);
				for (unsigned int q = 0; q < vertNew.mNumWeights;++q)
				{
					meshSrc.mWeights[vertNew.mFirstWeight+q] = meshSrc.mWeights[vertOld.mFirstWeight+q];
				}
#endif
			}
			else abHad[face.mIndices[i]] = true;
		}
	}
}
// ------------------------------------------------------------------------------------------------
void AttachChilds(int iParentID,aiNode* piParent,BoneList& bones)
{
	ai_assert(NULL != piParent && !piParent->mNumChildren);
	for (int i = 0; i < (int)bones.size();++i)
	{
		// (avoid infinite recursion)
		if (iParentID != i && bones[i].mParentIndex == iParentID)
		{
			// have it ...
			++piParent->mNumChildren;
		}
	}
	if (piParent->mNumChildren)
	{
		piParent->mChildren = new aiNode*[piParent->mNumChildren];
		for (int i = 0; i < (int)bones.size();++i)
		{
			// (avoid infinite recursion)
			if (iParentID != i && bones[i].mParentIndex == iParentID)
			{
				aiNode* pc;
				*piParent->mChildren++ = pc = new aiNode();
				pc->mName = aiString(bones[i].mName);
				pc->mParent = piParent;

				// get the transformation matrix from rotation and translational components
				aiQuaternion quat = aiQuaternion ( bones[i].mRotationQuat );
				//quat.w *= -1.0f; // DX to OGL
				pc->mTransformation = aiMatrix4x4 ( quat.GetMatrix());
				aiMatrix4x4 mTranslate;
				mTranslate.a4 = bones[i].mPositionXYZ.x;
				mTranslate.b4 = bones[i].mPositionXYZ.y;
				mTranslate.c4 = bones[i].mPositionXYZ.z;
				pc->mTransformation = pc->mTransformation*mTranslate;

				// store it for later use
				bones[i].mTransform = bones[i].mInvTransform = pc->mTransformation;
				bones[i].mInvTransform.Inverse();

				// the transformations for each bone are absolute,
				// so we need to multiply them with the inverse
				// of the absolut matrix of the parent
				if (-1 != iParentID)
				{
					pc->mTransformation = bones[iParentID].mInvTransform*pc->mTransformation;
				}

				// add children to this node, too
				AttachChilds( i, pc, bones);
			}
		}
		// undo our nice shift
		piParent->mChildren -= piParent->mNumChildren;
	}
}
// ------------------------------------------------------------------------------------------------
void MD5Importer::LoadMD5MeshFile ()
{
	std::string pFile = this->mFile + "MD5MESH";
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile, "rb"));

	// Check whether we can read from the file
	if( file.get() == NULL)
	{
		DefaultLogger::get()->warn("Failed to read MD5 mesh file: " + pFile);
		return;
	}
	bHadMD5Mesh = true;

	// now load the file into memory
	this->LoadFileIntoMemory(file.get());

	// now construct a parser and parse the file
	MD5::MD5Parser parser(mBuffer,fileSize);

	// load the mesh information from it
	MD5::MD5MeshParser meshParser(parser.mSections);

	// create the bone hierarchy - first the root node 
	// and dummy nodes for all meshes
	pScene->mRootNode = new aiNode();
	pScene->mRootNode->mNumChildren = 2;
	pScene->mRootNode->mChildren = new aiNode*[2];

	// now create the hierarchy of animated bones
	aiNode* pcNode = pScene->mRootNode->mChildren[1] = new aiNode();
	pcNode->mName.Set("MD5Anim");
	pcNode->mParent = pScene->mRootNode;
	AttachChilds(-1,pcNode,meshParser.mJoints);

	pcNode = pScene->mRootNode->mChildren[0] = new aiNode();
	pcNode->mName.Set("MD5Mesh");
	pcNode->mParent = pScene->mRootNode;

	std::vector<MD5::MeshDesc>::const_iterator end = meshParser.mMeshes.end();

	// FIX: MD5 files exported from Blender can have empty meshes
	for (std::vector<MD5::MeshDesc>::const_iterator 
		 it  = meshParser.mMeshes.begin(),
		 end = meshParser.mMeshes.end(); it != end;++it)
	{
		if (!(*it).mFaces.empty() && !(*it).mVertices.empty())
			++pScene->mNumMaterials;
	}

	// generate all meshes
	pScene->mNumMeshes = pScene->mNumMaterials;
	pScene->mMeshes = new aiMesh*[pScene->mNumMeshes];
	pScene->mMaterials = new aiMaterial*[pScene->mNumMeshes];

	//  storage for node mesh indices
	pcNode->mNumMeshes = pScene->mNumMeshes;
	pcNode->mMeshes = new unsigned int[pcNode->mNumMeshes];
	for (unsigned int m = 0; m < pcNode->mNumMeshes;++m)
		pcNode->mMeshes[m] = m;

	unsigned int n = 0;
	for (std::vector<MD5::MeshDesc>::iterator 
		 it  = meshParser.mMeshes.begin(),
		 end = meshParser.mMeshes.end(); it != end;++it)
	{
		MD5::MeshDesc& meshSrc = *it;
		if (meshSrc.mFaces.empty() || meshSrc.mVertices.empty())
			continue;

		aiMesh* mesh = pScene->mMeshes[n] = new aiMesh();
		mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

		// generate unique vertices in our internal verbose format
		MakeDataUnique(meshSrc);

		mesh->mNumVertices = (unsigned int) meshSrc.mVertices.size();
		mesh->mVertices = new aiVector3D[mesh->mNumVertices];
		mesh->mTextureCoords[0] = new aiVector3D[mesh->mNumVertices];
		mesh->mNumUVComponents[0] = 2;

		// copy texture coordinates
		aiVector3D* pv = mesh->mTextureCoords[0];
		for (MD5::VertexList::const_iterator
			iter =  meshSrc.mVertices.begin();
			iter != meshSrc.mVertices.end();++iter,++pv)
		{
			pv->x = (*iter).mUV.x;
			pv->y = 1.0f-(*iter).mUV.y; // D3D to OpenGL
			pv->z = 0.0f;
		}

		// sort all bone weights - per bone
		unsigned int* piCount = new unsigned int[meshParser.mJoints.size()];
		::memset(piCount,0,sizeof(unsigned int)*meshParser.mJoints.size());

		for (MD5::VertexList::const_iterator
			iter =  meshSrc.mVertices.begin();
			iter != meshSrc.mVertices.end();++iter,++pv)
		{
			for (unsigned int jub = (*iter).mFirstWeight, w = jub; w < jub + (*iter).mNumWeights;++w)
			{
				MD5::WeightDesc& desc = meshSrc.mWeights[w];
				++piCount[desc.mBone]; 
			}
		}

		// check how many we will need
		for (unsigned int p = 0; p < meshParser.mJoints.size();++p)
			if (piCount[p])mesh->mNumBones++;

		if (mesh->mNumBones) // just for safety
		{
			mesh->mBones = new aiBone*[mesh->mNumBones];
			for (unsigned int q = 0,h = 0; q < meshParser.mJoints.size();++q) 
			{
				if (!piCount[q])continue;
				aiBone* p = mesh->mBones[h] = new aiBone();
				p->mNumWeights = piCount[q];
				p->mWeights = new aiVertexWeight[p->mNumWeights];
				p->mName = aiString(meshParser.mJoints[q].mName);

				// store the index for later use
				meshParser.mJoints[q].mMap = h++;
			}
	
			//unsigned int g = 0;
			pv = mesh->mVertices;
			for (MD5::VertexList::const_iterator
				iter =  meshSrc.mVertices.begin();
				iter != meshSrc.mVertices.end();++iter,++pv)
			{
				// compute the final vertex position from all single weights
				*pv = aiVector3D();

				// there are models which have weights which don't sum to 1 ...
				// granite.md5mesh for example
				float fSum = 0.0f;
				for (unsigned int jub = (*iter).mFirstWeight, w = jub; w < jub + (*iter).mNumWeights;++w)
					fSum += meshSrc.mWeights[w].mWeight;
				if (!fSum)throw new ImportErrorException("The sum of all vertex bone weights is 0");

				// process bone weights
				for (unsigned int jub = (*iter).mFirstWeight, w = jub; w < jub + (*iter).mNumWeights;++w)
				{
					MD5::WeightDesc& desc = meshSrc.mWeights[w];
					float fNewWeight = desc.mWeight / fSum; 
					
					// transform the local position into worldspace
					MD5::BoneDesc& boneSrc = meshParser.mJoints[desc.mBone];
					aiVector3D v = desc.vOffsetPosition;
					aiQuaternion quat = aiQuaternion( boneSrc.mRotationQuat );
					//quat.w *= -1.0f;
					v = quat.GetMatrix() * v;
					v += boneSrc.mPositionXYZ;

					// use the original weight to compute the vertex position
					// (some MD5s seem to depend on the invalid weight values ...)
					pv->operator +=(v * desc.mWeight);
			
					aiBone* bone = mesh->mBones[boneSrc.mMap];
					*bone->mWeights++ = aiVertexWeight((unsigned int)(pv-mesh->mVertices),fNewWeight);
				}
				// convert from DOOM coordinate system to OGL
				std::swap((float&)pv->z,(float&)pv->y);
			}

			// undo our nice offset tricks ...
			for (unsigned int p = 0; p < mesh->mNumBones;++p)
				mesh->mBones[p]->mWeights -= mesh->mBones[p]->mNumWeights;
		}

		delete[] piCount;

		// now setup all faces - we can directly copy the list
		// (however, take care that the aiFace destructor doesn't delete the mIndices array)
		mesh->mNumFaces = (unsigned int)meshSrc.mFaces.size();
		mesh->mFaces = new aiFace[mesh->mNumFaces];
		for (unsigned int c = 0; c < mesh->mNumFaces;++c)
		{
			mesh->mFaces[c].mNumIndices = 3;
			mesh->mFaces[c].mIndices = meshSrc.mFaces[c].mIndices;
			meshSrc.mFaces[c].mIndices = NULL;
		}

		// generate a material for the mesh
		MaterialHelper* mat = new MaterialHelper();
		pScene->mMaterials[n] = mat;
		mat->AddProperty(&meshSrc.mShader,AI_MATKEY_TEXTURE_DIFFUSE(0));
		mesh->mMaterialIndex = n++;
	}

	// delete the file again
	this->UnloadFileFromMemory();
}
// ------------------------------------------------------------------------------------------------
void MD5Importer::LoadMD5AnimFile ()
{
	std::string pFile = this->mFile + "MD5ANIM";
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile, "rb"));

	// Check whether we can read from the file
	if( file.get() == NULL)
	{
		DefaultLogger::get()->warn("Failed to read MD5 anim file: " + pFile);
		return;
	}
	bHadMD5Anim = true;

	// now load the file into memory
	this->LoadFileIntoMemory(file.get());

	// now construct a parser and parse the file
	MD5::MD5Parser parser(mBuffer,fileSize);

	// load the animation information from it
	MD5::MD5AnimParser animParser(parser.mSections);

	// generate and fill the output animation
	if (!animParser.mAnimatedBones.empty())
	{
		pScene->mNumAnimations = 1;
		pScene->mAnimations = new aiAnimation*[1];
		aiAnimation* anim = pScene->mAnimations[0] = new aiAnimation();
		anim->mNumChannels = (unsigned int)animParser.mAnimatedBones.size();
		anim->mChannels = new aiNodeAnim*[anim->mNumChannels];
		for (unsigned int i = 0; i < anim->mNumChannels;++i)
		{
			aiNodeAnim* node = anim->mChannels[i] = new aiNodeAnim();
			node->mNodeName = aiString( animParser.mAnimatedBones[i].mName );

			// allocate storage for the keyframes
			node->mNumPositionKeys = node->mNumRotationKeys = (unsigned int)animParser.mFrames.size();
			node->mPositionKeys = new aiVectorKey[node->mNumPositionKeys];
			node->mRotationKeys = new aiQuatKey[node->mNumPositionKeys];
		}

		// 1 tick == 1 frame
		anim->mTicksPerSecond = animParser.fFrameRate;

		for (FrameList::const_iterator iter = animParser.mFrames.begin(), 
			iterEnd = animParser.mFrames.end();iter != iterEnd;++iter)
		{
			double dTime = (double)(*iter).iIndex;
			if (!(*iter).mValues.empty())
			{
				// now process all values in there ... read all joints
				aiNodeAnim** pcAnimNode = anim->mChannels;
				MD5::BaseFrameDesc* pcBaseFrame = &animParser.mBaseFrames[0];
				for (AnimBoneList::const_iterator 
					iter2		= animParser.mAnimatedBones.begin(), 
					iterEnd2	= animParser.mAnimatedBones.end();
					iter2		!= iterEnd2;++iter2,++pcAnimNode,++pcBaseFrame)
				{
					if((*iter2).iFirstKeyIndex >= (*iter).mValues.size())
					{
						// TODO: add proper array checks for all cases here ...
						DefaultLogger::get()->error("Keyframe index is out of range. ");
						continue;
					}
					const float* fpCur = &(*iter).mValues[(*iter2).iFirstKeyIndex];

					aiNodeAnim* pcCurAnimBone = *pcAnimNode;
					aiVectorKey* vKey = pcCurAnimBone->mPositionKeys++;

					// translation on the x axis
					if ((*iter2).iFlags & AI_MD5_ANIMATION_FLAG_TRANSLATE_X)
						vKey->mValue.x = *fpCur++;
					else vKey->mValue.x = pcBaseFrame->vPositionXYZ.x;

					// translation on the y axis
					if ((*iter2).iFlags & AI_MD5_ANIMATION_FLAG_TRANSLATE_Y)
						vKey->mValue.y = *fpCur++;
					else vKey->mValue.y = pcBaseFrame->vPositionXYZ.y;

					// translation on the z axis
					if ((*iter2).iFlags & AI_MD5_ANIMATION_FLAG_TRANSLATE_Z)
						vKey->mValue.z = *fpCur++;
					else vKey->mValue.z = pcBaseFrame->vPositionXYZ.z;


					// rotation quaternion, x component
					aiQuatKey* qKey = pcCurAnimBone->mRotationKeys++;
					aiVector3D vTemp;
					if ((*iter2).iFlags & AI_MD5_ANIMATION_FLAG_ROTQUAT_X)
						vTemp.x = *fpCur++;
					else vTemp.x = pcBaseFrame->vRotationQuat.x;

					// rotation quaternion, y component
					if ((*iter2).iFlags & AI_MD5_ANIMATION_FLAG_ROTQUAT_Y)
						vTemp.y = *fpCur++;
					else vTemp.y = pcBaseFrame->vRotationQuat.y;

					// rotation quaternion, z component
					if ((*iter2).iFlags & AI_MD5_ANIMATION_FLAG_ROTQUAT_Z)
						vTemp.z = *fpCur++;
					else vTemp.z = pcBaseFrame->vRotationQuat.z;

					// compute the w component of the quaternion - invert it (DX to OGL)
					qKey->mValue = aiQuaternion(vTemp);
					//qKey->mValue.w *= -1.0f;

					qKey->mTime = dTime;
					vKey->mTime = dTime;
				}
			}
			// compute the duration of the animation
			anim->mDuration = std::max(dTime,anim->mDuration);
		}

		// undo our offset computations
		for (unsigned int i = 0; i < anim->mNumChannels;++i)
		{
			aiNodeAnim* node = anim->mChannels[i];
			node->mPositionKeys -= node->mNumPositionKeys;
			node->mRotationKeys -= node->mNumPositionKeys;
		}
	}

	// delete the file again
	this->UnloadFileFromMemory();
}
