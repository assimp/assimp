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

/** @file Implementation of the SMD importer class */

// internal headers
#include "MaterialSystem.h"
#include "SMDLoader.h"
#include "StringComparison.h"
#include "fast_atof.h"

// public headers
#include "../include/DefaultLogger.h"
#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"
#include "../include/aiAssert.h"
#include "../include/assimp.hpp"

// boost headers
#include <boost/scoped_ptr.hpp>

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
SMDImporter::SMDImporter()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
SMDImporter::~SMDImporter()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool SMDImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
{
	// simple check of file extension is enough for the moment
	std::string::size_type pos = pFile.find_last_of('.');
	// no file extension - can't read
	if( pos == std::string::npos)
		return false;
	std::string extension = pFile.substr( pos);

	if (extension.length() < 4)return false;
	if (extension[0] != '.')return false;

	// VTA is not really supported as it contains vertex animations.
	// However, at least the first keyframe can be loaded
	if ((extension[1] != 's' && extension[1] != 'S') ||
	    (extension[2] != 'm' && extension[2] != 'M') ||
	    (extension[3] != 'd' && extension[3] != 'D'))
	{
		if ((extension[1] != 'v' && extension[1] != 'V') ||
			(extension[2] != 't' && extension[2] != 'T') ||
			(extension[3] != 'a' && extension[3] != 'A'))
		{
			return false;
		}
	}
	return true;
}
// ------------------------------------------------------------------------------------------------
// Setup configuration properties
void SMDImporter::SetupProperties(const Importer* pImp)
{
	// The AI_CONFIG_IMPORT_SMD_KEYFRAME option overrides the
	// AI_CONFIG_IMPORT_GLOBAL_KEYFRAME option.
	if(0xffffffff == (this->configFrameID = pImp->GetPropertyInteger(
		AI_CONFIG_IMPORT_SMD_KEYFRAME,0xffffffff)))
	{
		this->configFrameID = pImp->GetPropertyInteger(AI_CONFIG_IMPORT_GLOBAL_KEYFRAME,0);
	}
}
// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void SMDImporter::InternReadFile( 
	const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile, "rt"));

	// Check whether we can read from the file
	if( file.get() == NULL)
	{
		throw new ImportErrorException( "Failed to open SMD/VTA file " + pFile + ".");
	}

	this->iFileSize = (unsigned int)file->FileSize();

	// allocate storage and copy the contents of the file to a memory buffer
	this->pScene = pScene;
	this->mBuffer = new char[this->iFileSize+1];
	file->Read( (void*)mBuffer, 1, this->iFileSize);
	this->iSmallestFrame = (1 << 31);
	this->bHasUVs = true;
	this->iLineNumber = 1;

	// append a terminal 0
	this->mBuffer[this->iFileSize] = '\0';

	// reserve enough space for ... hm ... 10 textures
	this->aszTextures.reserve(10);

	// reserve enough space for ... hm ... 1000 triangles
	this->asTriangles.reserve(1000);

	// reserve enough space for ... hm ... 20 bones
	this->asBones.reserve(20);

	try
	{
		// parse the file ...
		this->ParseFile();

		// if there are no triangles it seems to be an animation SMD,
		// containing only the animation skeleton.
		if (this->asTriangles.empty())
		{
			if (this->asBones.empty())
			{
				throw new ImportErrorException("No triangles and no bones have "
					"been found in the file. This file seems to be invalid.");
			}
			// set the flag in the scene structure which indicates
			// that there is nothing than an animation skeleton
			pScene->mFlags |= AI_SCENE_FLAGS_ANIM_SKELETON_ONLY;
		}
		
		if (!this->asBones.empty())
		{
			// check whether all bones have been initialized
			for (std::vector<SMD::Bone>::const_iterator
				i =  this->asBones.begin();
				i != this->asBones.end();++i)
			{
				if (!(*i).mName.length())
				{
					DefaultLogger::get()->warn("Not all bones have been initialized");
					break;
				}
			}

			// now fix invalid time values and make sure the animation starts at frame 0
			this->FixTimeValues();

			// compute absolute bone transformation matrices
			this->ComputeAbsoluteBoneTransformations();
		}
		if (!(pScene->mFlags & AI_SCENE_FLAGS_ANIM_SKELETON_ONLY))
		{
			// create output meshes
			this->CreateOutputMeshes();

			// build an output material list
			this->CreateOutputMaterials();
		}

		// build the output animation
		this->CreateOutputAnimations();

		// build output nodes (bones are added as empty dummy nodes)
		this->CreateOutputNodes();
	}
	catch (ImportErrorException* ex)
	{
		delete[] this->mBuffer;
		AI_DEBUG_INVALIDATE_PTR(this->mBuffer);
		throw ex;
	}

	// delete the file buffer
	delete[] this->mBuffer;
	AI_DEBUG_INVALIDATE_PTR(this->mBuffer);
}
// ------------------------------------------------------------------------------------------------
// Write an error message with line number to the log file
void SMDImporter::LogErrorNoThrow(const char* msg)
{
	char szTemp[1024];

#if _MSC_VER >= 1400
	sprintf_s(szTemp,"Line %i: %s",this->iLineNumber,msg);
#else
	ai_assert(strlen(msg) < 1000);
	sprintf(szTemp,"Line %i: %s",this->iLineNumber,msg);
#endif

	DefaultLogger::get()->error(szTemp);
}
// ------------------------------------------------------------------------------------------------
// Write a warning with line number to the log file
void SMDImporter::LogWarning(const char* msg)
{
	char szTemp[1024];

#if _MSC_VER >= 1400
	sprintf_s(szTemp,"Line %i: %s",this->iLineNumber,msg);
#else
	ai_assert(strlen(msg) < 1000);
	sprintf(szTemp,"Line %i: %s",this->iLineNumber,msg);
#endif
	DefaultLogger::get()->warn(szTemp);
}
// ------------------------------------------------------------------------------------------------
// Fix invalid time values in the file
void SMDImporter::FixTimeValues()
{
	double dDelta = (double)this->iSmallestFrame;
	double dMax = 0.0f;
	for (std::vector<SMD::Bone>::iterator
		iBone =  this->asBones.begin();
		iBone != this->asBones.end();++iBone)
	{
		for (std::vector<SMD::Bone::Animation::MatrixKey>::iterator
			iKey =  (*iBone).sAnim.asKeys.begin();
			iKey != (*iBone).sAnim.asKeys.end();++iKey)
		{
			(*iKey).dTime -= dDelta;
			dMax = std::max(dMax, (*iKey).dTime);
		}
	}
	this->dLengthOfAnim = dMax;
}
// ------------------------------------------------------------------------------------------------
// create output meshes
void SMDImporter::CreateOutputMeshes()
{
	// we need to sort all faces by their material index
	// in opposition to other loaders we can be sure that each
	// material is at least used once.
	this->pScene->mNumMeshes = (unsigned int) this->aszTextures.size();
	this->pScene->mMeshes = new aiMesh*[this->pScene->mNumMeshes];

	typedef std::vector<unsigned int> FaceList;
	FaceList* aaiFaces = new FaceList[this->pScene->mNumMeshes];

	// approximate the space that will be required
	unsigned int iNum = (unsigned int)this->asTriangles.size() / this->pScene->mNumMeshes;
	iNum += iNum >> 1;
	for (unsigned int i = 0; i < this->pScene->mNumMeshes;++i)
	{
		aaiFaces[i].reserve(iNum);
	}

	// collect all faces
	iNum = 0;
	for (std::vector<SMD::Face>::const_iterator
		iFace =  this->asTriangles.begin();
		iFace != this->asTriangles.end();++iFace,++iNum)
	{
		if (0xffffffff == (*iFace).iTexture)aaiFaces[(*iFace).iTexture].push_back( 0 );
		else if ((*iFace).iTexture >= this->aszTextures.size())
		{
			DefaultLogger::get()->error("[SMD/VTA] Material index overflow in face");
			aaiFaces[(*iFace).iTexture].push_back((unsigned int)this->aszTextures.size()-1);
		}
		else aaiFaces[(*iFace).iTexture].push_back(iNum);
	} 

	// now create the output meshes
	for (unsigned int i = 0; i < this->pScene->mNumMeshes;++i)
	{
		aiMesh*& pcMesh = this->pScene->mMeshes[i] = new aiMesh();
		ai_assert(!aaiFaces[i].empty()); // should not be empty ...

		pcMesh->mNumVertices = (unsigned int)aaiFaces[i].size()*3;
		pcMesh->mNumFaces = (unsigned int)aaiFaces[i].size();
		pcMesh->mMaterialIndex = i;

		// storage for bones
		typedef std::pair<unsigned int,float> TempWeightListEntry;
		typedef std::vector< TempWeightListEntry > TempBoneWeightList;

		TempBoneWeightList* aaiBones = new TempBoneWeightList[this->asBones.size()]();

		// try to reserve enough memory without wasting too much
		for (unsigned int iBone = 0; iBone < this->asBones.size();++iBone)
		{
			aaiBones[iBone].reserve(pcMesh->mNumVertices/this->asBones.size());
		}

		// allocate storage
		pcMesh->mFaces = new aiFace[pcMesh->mNumFaces];
		aiVector3D* pcNormals = pcMesh->mNormals = new aiVector3D[pcMesh->mNumVertices];
		aiVector3D* pcVerts = pcMesh->mVertices = new aiVector3D[pcMesh->mNumVertices];

		aiVector3D* pcUVs = NULL;
		if (this->bHasUVs)
		{
			pcUVs = pcMesh->mTextureCoords[0] = new aiVector3D[pcMesh->mNumVertices];
			pcMesh->mNumUVComponents[0] = 2;
		}

		iNum = 0;
		for (unsigned int iFace = 0; iFace < pcMesh->mNumFaces;++iFace)
		{
			pcMesh->mFaces[iFace].mIndices = new unsigned int[3];
			pcMesh->mFaces[iFace].mNumIndices = 3;

			// fill the vertices (hardcode the loop for performance)
			unsigned int iSrcFace = aaiFaces[i][iFace];
			SMD::Face& face = this->asTriangles[iSrcFace];

			*pcVerts++ = face.avVertices[0].pos;
			*pcVerts++ = face.avVertices[1].pos;
			*pcVerts++ = face.avVertices[2].pos; 

			// fill the normals
			*pcNormals++ = face.avVertices[0].nor;
			*pcNormals++ = face.avVertices[1].nor;
			*pcNormals++ = face.avVertices[2].nor;

			// fill the texture coordinates
			if (pcUVs)
			{
				*pcUVs++ = face.avVertices[0].uv;
				*pcUVs++ = face.avVertices[1].uv;
				*pcUVs++ = face.avVertices[2].uv;
			}
			
			for (unsigned int iVert = 0; iVert < 3;++iVert)
			{
				float fSum = 0.0f;
				for (unsigned int iBone = 0;iBone < face.avVertices[iVert].aiBoneLinks.size();++iBone)
				{
					TempWeightListEntry& pairval = face.avVertices[iVert].aiBoneLinks[iBone];
					if (pairval.first >= this->asBones.size())
					{
						DefaultLogger::get()->error("[SMD/VTA] Bone index overflow. "
							"The bone index will be ignored, the weight will be assigned "
							"to the vertex' parent node");
						continue;
					}
					aaiBones[pairval.first].push_back(TempWeightListEntry(iNum,pairval.second));
					fSum += pairval.second;
				}
				// if the sum of all vertex weights is not 1.0 we must assign 
				// the rest to the vertex' parent node. Well, at least the doc says 
				// we should ...
				if (fSum <= 1.0f)
				{
					if (face.avVertices[iVert].iParentNode >= this->asBones.size())
					{
						DefaultLogger::get()->error("[SMD/VTA] Bone index overflow. "
							"The index of the vertex parent bone is invalid. "
							"The remaining weights will be normalized to 1.0");

						if (fSum)
						{
							fSum = 1 / fSum;
							for (unsigned int iBone = 0;iBone < face.avVertices[iVert].aiBoneLinks.size();++iBone)
							{
								TempWeightListEntry& pairval = face.avVertices[iVert].aiBoneLinks[iBone];
								if (pairval.first >= this->asBones.size())continue;
								aaiBones[pairval.first].back().second *= fSum;
							}
						}
					}
					else
					{
						aaiBones[face.avVertices[iVert].iParentNode].push_back(
							TempWeightListEntry(iNum,1.0f-fSum));
					}
				}

				pcMesh->mFaces[iFace].mIndices[iVert] = iNum++;
			}
		}

		// now build all bones of the mesh
		iNum = 0;
		for (unsigned int iBone = 0; iBone < this->asBones.size();++iBone)
		{
			if (!aaiBones[iBone].empty())++iNum;
		}
		pcMesh->mNumBones = iNum;
		pcMesh->mBones = new aiBone*[pcMesh->mNumBones];
		iNum = 0;
		for (unsigned int iBone = 0; iBone < this->asBones.size();++iBone)
		{
			if (aaiBones[iBone].empty())continue;
			aiBone*& bone = pcMesh->mBones[iNum] = new aiBone();

			bone->mNumWeights = (unsigned int)aaiBones[iBone].size();
			bone->mWeights = new aiVertexWeight[bone->mNumWeights];
			bone->mOffsetMatrix = this->asBones[iBone].mOffsetMatrix;
			bone->mName.Set( this->asBones[iBone].mName );

			this->asBones[iBone].bIsUsed = true;

			for (unsigned int iWeight = 0; iWeight < bone->mNumWeights;++iWeight)
			{
				bone->mWeights[iWeight].mVertexId = aaiBones[iBone][iWeight].first;
				bone->mWeights[iWeight].mWeight = aaiBones[iBone][iWeight].second;
			}
			++iNum;
		}

		delete[] aaiBones;
	}
	delete[] aaiFaces;
}
// ------------------------------------------------------------------------------------------------
// add bone child nodes
void SMDImporter::AddBoneChildren(aiNode* pcNode, uint32_t iParent)
{
	ai_assert(NULL != pcNode && 0 == pcNode->mNumChildren && NULL == pcNode->mChildren);

	// first count ...
	for (unsigned int i = 0; i < this->asBones.size();++i)
	{
		SMD::Bone& bone = this->asBones[i];
		if (bone.iParent == iParent)++pcNode->mNumChildren;
	}

	// now allocate the output array
	pcNode->mChildren = new aiNode*[pcNode->mNumChildren];

	// and fill all subnodes
	unsigned int qq = 0;
	for (unsigned int i = 0; i < this->asBones.size();++i)
	{
		SMD::Bone& bone = this->asBones[i];
		if (bone.iParent != iParent)continue;

		aiNode* pc = pcNode->mChildren[qq++] = new aiNode();
		pc->mName.Set(bone.mName);

		// store the local transformation matrix of the bind pose
		pc->mTransformation = bone.sAnim.asKeys[bone.sAnim.iFirstTimeKey].matrix;
		pc->mParent = pcNode;

		// add children to this node, too
		AddBoneChildren(pc,i);
	}
}
// ------------------------------------------------------------------------------------------------
// create output nodes
void SMDImporter::CreateOutputNodes()
{
	this->pScene->mRootNode = new aiNode();
	if (!(this->pScene->mFlags & AI_SCENE_FLAGS_ANIM_SKELETON_ONLY))
	{
		// create one root node that renders all meshes
		this->pScene->mRootNode->mNumMeshes = this->pScene->mNumMeshes;
		this->pScene->mRootNode->mMeshes = new unsigned int[this->pScene->mNumMeshes];
		for (unsigned int i = 0; i < this->pScene->mNumMeshes;++i)
			this->pScene->mRootNode->mMeshes[i] = i;
	}

	// now add all bones as dummy sub nodes to the graph
	this->AddBoneChildren(this->pScene->mRootNode,(uint32_t)-1);

	// if we have only one bone we can even remove the root node
	if (this->pScene->mFlags & AI_SCENE_FLAGS_ANIM_SKELETON_ONLY && 
		1 == this->pScene->mRootNode->mNumChildren)
	{
		aiNode* pcOldRoot = this->pScene->mRootNode;
		this->pScene->mRootNode = pcOldRoot->mChildren[0];
		pcOldRoot->mChildren[0] = NULL;
		delete pcOldRoot;

		this->pScene->mRootNode->mParent = NULL;
	}
	else
	{
		::strcpy(this->pScene->mRootNode->mName.data, "<SMD_root>");
		this->pScene->mRootNode->mName.length = 10;
	}
}
// ------------------------------------------------------------------------------------------------
// create output animations
void SMDImporter::CreateOutputAnimations()
{
	unsigned int iNumBones = 0;
	for (std::vector<SMD::Bone>::const_iterator
		i =  this->asBones.begin();
		i != this->asBones.end();++i)
	{
		if ((*i).bIsUsed)++iNumBones;
	}
	if (!iNumBones)
	{
		// just make sure this case doesn't occur ... (it could occur
		// if the file was invalid)
		return;
	}

	this->pScene->mNumAnimations = 1;
	this->pScene->mAnimations = new aiAnimation*[1];
	aiAnimation*& anim = this->pScene->mAnimations[0] = new aiAnimation();

	anim->mDuration = this->dLengthOfAnim;
	anim->mNumBones = iNumBones;
	anim->mTicksPerSecond = 25.0; // FIXME: is this correct?

	aiBoneAnim** pp = anim->mBones = new aiBoneAnim*[anim->mNumBones];
	
	// now build valid keys
	unsigned int a = 0;
	for (std::vector<SMD::Bone>::const_iterator
		i =  this->asBones.begin();
		i != this->asBones.end();++i)
	{
		if (!(*i).bIsUsed)continue;

		aiBoneAnim* p = pp[a] = new aiBoneAnim();

		// copy the name of the bone
		p->mBoneName.length = (*i).mName.length();
		::memcpy(p->mBoneName.data,(*i).mName.c_str(),p->mBoneName.length);
		p->mBoneName.data[p->mBoneName.length] = '\0';

		p->mNumRotationKeys = (unsigned int) (*i).sAnim.asKeys.size();
		if (p->mNumRotationKeys)
		{
			p->mNumPositionKeys = p->mNumRotationKeys;
			aiVectorKey* pVecKeys = p->mPositionKeys = new aiVectorKey[p->mNumRotationKeys];
			aiQuatKey* pRotKeys = p->mRotationKeys = new aiQuatKey[p->mNumRotationKeys];

			for (std::vector<SMD::Bone::Animation::MatrixKey>::const_iterator
				qq =  (*i).sAnim.asKeys.begin();
				qq != (*i).sAnim.asKeys.end(); ++qq)
			{
				pRotKeys->mTime = pVecKeys->mTime = (*qq).dTime;

				// compute the rotation quaternion from the euler angles
				pRotKeys->mValue = aiQuaternion( (*qq).vRot.x, (*qq).vRot.y, (*qq).vRot.z );
				pVecKeys->mValue = (*qq).vPos;

				++pVecKeys; ++pRotKeys;
			}
		}
		++a;

		// there are no scaling keys ...
	}
}
// ------------------------------------------------------------------------------------------------
void SMDImporter::ComputeAbsoluteBoneTransformations()
{
	// for each bone: determine the key with the lowest time value
	// theoretically the SMD format should have all keyframes
	// in order. However, I've seen a file where this wasn't true.
	for (unsigned int i = 0; i < this->asBones.size();++i)
	{
		SMD::Bone& bone = this->asBones[i];

		uint32_t iIndex = 0;
		double dMin = 10e10;
		for (unsigned int i = 0; i < bone.sAnim.asKeys.size();++i)
		{
			double d = std::min(bone.sAnim.asKeys[i].dTime,dMin);
			if (d < dMin)	
			{
				dMin = d;
				iIndex = i;
			}
		}
		bone.sAnim.iFirstTimeKey = iIndex;
	}

	unsigned int iParent = 0;
	while (iParent < this->asBones.size())
	{
		for (unsigned int iBone = 0; iBone < this->asBones.size();++iBone)
		{
			SMD::Bone& bone = this->asBones[iBone];
	
			if (iParent == bone.iParent)
			{
				SMD::Bone& parentBone = this->asBones[iParent];

			
				uint32_t iIndex = bone.sAnim.iFirstTimeKey;
				const aiMatrix4x4& mat = bone.sAnim.asKeys[iIndex].matrix;
				aiMatrix4x4& matOut = bone.sAnim.asKeys[iIndex].matrixAbsolute;

				// the same for the parent bone ...
				iIndex = parentBone.sAnim.iFirstTimeKey;
				const aiMatrix4x4& mat2 = parentBone.sAnim.asKeys[iIndex].matrix;

				// compute the absolute transformation matrix
				matOut = mat * mat2;
			}
		}
		++iParent;
	}

	// store the inverse of the absolute transformation matrix 
	// of the first key as bone offset matrix
	for (iParent = 0; iParent < this->asBones.size();++iParent)
	{
		SMD::Bone& bone = this->asBones[iParent];
		aiMatrix4x4& mat = bone.sAnim.asKeys[bone.sAnim.iFirstTimeKey].matrixAbsolute;
		bone.mOffsetMatrix = mat;
		bone.mOffsetMatrix.Inverse();
	}
}
// ------------------------------------------------------------------------------------------------
// create output materials
void SMDImporter::CreateOutputMaterials()
{
	this->pScene->mNumMaterials = (unsigned int)this->aszTextures.size();
	this->pScene->mMaterials = new aiMaterial*[std::max(1u, this->pScene->mNumMaterials)];

	for (unsigned int iMat = 0; iMat < this->pScene->mNumMaterials;++iMat)
	{
		MaterialHelper* pcMat = new MaterialHelper();
		this->pScene->mMaterials[iMat] = pcMat;

		aiString szName;
#if _MSC_VER >= 1400
		szName.length = (size_t)::sprintf_s(szName.data,"Texture_%i",iMat);
#else
		szName.length = (size_t)::sprintf(szName.data,"Texture_%i",iMat);
#endif
		pcMat->AddProperty(&szName,AI_MATKEY_NAME);

		::strcpy(szName.data, this->aszTextures[iMat].c_str() );
		szName.length = this->aszTextures[iMat].length();
		pcMat->AddProperty(&szName,AI_MATKEY_TEXTURE_DIFFUSE(0));
	}

	// create a default material if necessary
	if (0 == this->pScene->mNumMaterials)
	{
		this->pScene->mNumMaterials = 1;

		MaterialHelper* pcHelper = new MaterialHelper();
		this->pScene->mMaterials[0] = pcHelper;

		int iMode = (int)aiShadingMode_Gouraud;
		pcHelper->AddProperty<int>(&iMode, 1, AI_MATKEY_SHADING_MODEL);

		aiColor3D clr;
		clr.b = clr.g = clr.r = 0.7f;
		pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_DIFFUSE);
		pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_SPECULAR);

		clr.b = clr.g = clr.r = 0.05f;
		pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_AMBIENT);

		aiString szName;
		szName.Set(AI_DEFAULT_MATERIAL_NAME);
		pcHelper->AddProperty(&szName,AI_MATKEY_NAME);
	}
}
// ------------------------------------------------------------------------------------------------
// Parse the file
void SMDImporter::ParseFile()
{
	const char* szCurrent = this->mBuffer;

	// read line per line ...
	while (true)
	{
		if(!SkipSpacesAndLineEnd(szCurrent,&szCurrent)) break;

		// "version <n> \n", <n> should be 1 for hl and hl² SMD files
		if (0 == ASSIMP_strincmp(szCurrent,"version",7) &&
			IsSpaceOrNewLine(*(szCurrent+7)))
		{
			szCurrent += 8;
			if(!SkipSpaces(szCurrent,&szCurrent)) break;
			if (1 != strtol10(szCurrent,&szCurrent))
			{
				DefaultLogger::get()->warn("SMD.version is not 1. This "
					"file format is not known. Continuing happily ...");
			}
			//continue;
		}
		// "nodes\n" - Starts the node section
		if (0 == ASSIMP_strincmp(szCurrent,"nodes",5) &&
			IsSpaceOrNewLine(*(szCurrent+5)))
		{
			szCurrent += 6;
			this->ParseNodesSection(szCurrent,&szCurrent);
			//continue;
		}
		// "triangles\n" - Starts the triangle section
		if (0 == ASSIMP_strincmp(szCurrent,"triangles",9) &&
			IsSpaceOrNewLine(*(szCurrent+9)))
		{
			szCurrent += 10;
			this->ParseTrianglesSection(szCurrent,&szCurrent);
			//continue;
		}
		// "vertexanimation\n" - Starts the vertex animation section
		if (0 == ASSIMP_strincmp(szCurrent,"vertexanimation",15) &&
			IsSpaceOrNewLine(*(szCurrent+15)))
		{
			this->bHasUVs = false;
			szCurrent += 16;
			this->ParseVASection(szCurrent,&szCurrent);
			//continue;
		}
		// "skeleton\n" - Starts the skeleton section
		if (0 == ASSIMP_strincmp(szCurrent,"skeleton",8) &&
			IsSpaceOrNewLine(*(szCurrent+8)))
		{
			szCurrent += 9;
			this->ParseSkeletonSection(szCurrent,&szCurrent);
			//continue;
		}
		else SkipLine(szCurrent,&szCurrent);
	}
	return;
}
// ------------------------------------------------------------------------------------------------
unsigned int SMDImporter::GetTextureIndex(const std::string& filename)
{
	unsigned int iIndex = 0;
	for (std::vector<std::string>::const_iterator
		i =  this->aszTextures.begin();
		i != this->aszTextures.end();++i,++iIndex)
	{
		// case-insensitive ... just for safety
		if (0 == ASSIMP_stricmp ( filename.c_str(),(*i).c_str()))return iIndex;
	}
	iIndex = (unsigned int)this->aszTextures.size();
	this->aszTextures.push_back(filename);
	return iIndex;
}
// ------------------------------------------------------------------------------------------------
// Parse the nodes section of the file
void SMDImporter::ParseNodesSection(const char* szCurrent,
	const char** szCurrentOut)
{
	while (true)
	{
		// "end\n" - Ends the nodes section
		if (0 == ASSIMP_strincmp(szCurrent,"end",3) &&
			IsSpaceOrNewLine(*(szCurrent+3)))
		{
			szCurrent += 4;
			break;
		}
		this->ParseNodeInfo(szCurrent,&szCurrent);
	}
	*szCurrentOut = szCurrent;
	SkipSpacesAndLineEnd(szCurrent,&szCurrent);
}
// ------------------------------------------------------------------------------------------------
// Parse the triangles section of the file
void SMDImporter::ParseTrianglesSection(const char* szCurrent,
	const char** szCurrentOut)
{
	// parse a triangle, parse another triangle, parse the next triangle ...
	// and so on until we reach a token that looks quite similar to "end"
	while (true)
	{
		if(!SkipSpacesAndLineEnd(szCurrent,&szCurrent)) break;

		// "end\n" - Ends the triangles section
		if (0 == ASSIMP_strincmp(szCurrent,"end",3) &&
			IsSpaceOrNewLine(*(szCurrent+3)))
		{
			szCurrent += 4;
			break;
		}
		this->ParseTriangle(szCurrent,&szCurrent);
	}
	*szCurrentOut = szCurrent;
	SkipSpacesAndLineEnd(szCurrent,&szCurrent);
}
// ------------------------------------------------------------------------------------------------
// Parse the vertex animation section of the file
void SMDImporter::ParseVASection(const char* szCurrent,
	const char** szCurrentOut)
{
	unsigned int iCurIndex = 0;
	while (true)
	{
		if(!SkipSpacesAndLineEnd(szCurrent,&szCurrent)) break;

		// "end\n" - Ends the "vertexanimation" section
		if (0 == ASSIMP_strincmp(szCurrent,"end",3) &&
			IsSpaceOrNewLine(*(szCurrent+3)))
		{
			szCurrent += 4;
			SkipLine(szCurrent,&szCurrent);
			break;
		}
		// "time <n>\n" 
		if (0 == ASSIMP_strincmp(szCurrent,"time",4) &&
			IsSpaceOrNewLine(*(szCurrent+4)))
		{
			szCurrent += 5;
			// NOTE: The doc says that time values COULD be negative ...
			// note2: this is the shape key -> valve docs
			int iTime = 0;
			if(!this->ParseSignedInt(szCurrent,&szCurrent,iTime) || this->configFrameID != iTime)break;
			SkipLine(szCurrent,&szCurrent);
		}
		else 
		{
			this->ParseVertex(szCurrent,&szCurrent,this->asTriangles.back().avVertices[iCurIndex],true);
			if(3 == ++iCurIndex)
			{
				this->asTriangles.push_back(SMD::Face());
				iCurIndex = 0;
			}
		}
	}

	if (iCurIndex)
	{
		// no degenerates, so let this triangle
		this->aszTextures.pop_back();
	}

	*szCurrentOut = szCurrent;
	SkipSpacesAndLineEnd(szCurrent,&szCurrent);
}
// ------------------------------------------------------------------------------------------------
// Parse the skeleton section of the file
void SMDImporter::ParseSkeletonSection(const char* szCurrent,
	const char** szCurrentOut)
{
	int iTime = 0;
	while (true)
	{
		if(!SkipSpacesAndLineEnd(szCurrent,&szCurrent)) break;

		// "end\n" - Ends the skeleton section
		if (0 == ASSIMP_strincmp(szCurrent,"end",3) &&
			IsSpaceOrNewLine(*(szCurrent+3)))
		{
			szCurrent += 4;
			SkipLine(szCurrent,&szCurrent);
			break;
		}
		// "time <n>\n" - Specifies the current animation frame
		else if (0 == ASSIMP_strincmp(szCurrent,"time",4) &&
			IsSpaceOrNewLine(*(szCurrent+4)))
		{
			szCurrent += 5;
			// NOTE: The doc says that time values COULD be negative ...
			if(!this->ParseSignedInt(szCurrent,&szCurrent,iTime))break;

			this->iSmallestFrame = std::min(this->iSmallestFrame,iTime);
			SkipLine(szCurrent,&szCurrent);
		}
		else this->ParseSkeletonElement(szCurrent,&szCurrent,iTime);
	}
	*szCurrentOut = szCurrent;	
}

#define SMDI_PARSE_RETURN { \
	SkipLine(szCurrent,&szCurrent); \
	*szCurrentOut = szCurrent; \
	return; \
}

// ------------------------------------------------------------------------------------------------
// Parse a node line
void SMDImporter::ParseNodeInfo(const char* szCurrent,
	const char** szCurrentOut)
{
	unsigned int iBone  = 0;
	SkipSpacesAndLineEnd(szCurrent,&szCurrent);
	if(!this->ParseUnsignedInt(szCurrent,&szCurrent,iBone) || !SkipSpaces(szCurrent,&szCurrent))
	{
		this->LogErrorNoThrow("Unexpected EOF/EOL while parsing bone index");
		SMDI_PARSE_RETURN;
	}
	// add our bone to the list
	if (iBone >= this->asBones.size())this->asBones.resize(iBone+1);
	SMD::Bone& bone = this->asBones[iBone];

	bool bQuota = true;
	if ('\"' != *szCurrent)
	{
		this->LogWarning("Bone name is expcted to be enclosed in "
			"double quotation marks. ");
		bQuota = false;
	}
	else ++szCurrent;

	const char* szEnd = szCurrent;
	while (true)
	{
		if (bQuota && '\"' == *szEnd)
		{
			iBone = (unsigned int)(szEnd - szCurrent);
			++szEnd;
			break;
		}
		else if (IsSpaceOrNewLine(*szEnd))
		{
			iBone = (unsigned int)(szEnd - szCurrent);
			break;
		}
		else if (!(*szEnd))
		{
			this->LogErrorNoThrow("Unexpected EOF/EOL while parsing bone name");
			SMDI_PARSE_RETURN;
		}
		++szEnd;
	}
	bone.mName = std::string(szCurrent,iBone);
	szCurrent = szEnd;

	// the only negative bone parent index that could occur is -1 AFAIK
	if(!this->ParseSignedInt(szCurrent,&szCurrent,(int&)bone.iParent))
	{
		this->LogErrorNoThrow("Unexpected EOF/EOL while parsing bone parent index. Assuming -1");
		SMDI_PARSE_RETURN;
	}

	// go to the beginning of the next line
	SMDI_PARSE_RETURN;
}
// ------------------------------------------------------------------------------------------------
// Parse a skeleton element
void SMDImporter::ParseSkeletonElement(const char* szCurrent,
	const char** szCurrentOut,int iTime)
{
	aiVector3D vPos;
	aiVector3D vRot;

	unsigned int iBone  = 0;
	if(!this->ParseUnsignedInt(szCurrent,&szCurrent,iBone))
	{
		DefaultLogger::get()->error("Unexpected EOF/EOL while parsing bone index");
		SMDI_PARSE_RETURN;
	}
	if (iBone >= this->asBones.size())
	{
		this->LogErrorNoThrow("Bone index in skeleton section is out of range");
		SMDI_PARSE_RETURN;
	}
	SMD::Bone& bone = this->asBones[iBone];

	bone.sAnim.asKeys.push_back(SMD::Bone::Animation::MatrixKey());
	SMD::Bone::Animation::MatrixKey& key = bone.sAnim.asKeys.back();

	key.dTime = (double)iTime;
	if(!this->ParseFloat(szCurrent,&szCurrent,vPos.x))
	{
		this->LogErrorNoThrow("Unexpected EOF/EOL while parsing bone.pos.x");
		SMDI_PARSE_RETURN;
	}
	if(!this->ParseFloat(szCurrent,&szCurrent,vPos.y))
	{
		this->LogErrorNoThrow("Unexpected EOF/EOL while parsing bone.pos.y");
		SMDI_PARSE_RETURN;
	}
	if(!this->ParseFloat(szCurrent,&szCurrent,vPos.z))
	{
		this->LogErrorNoThrow("Unexpected EOF/EOL while parsing bone.pos.z");
		SMDI_PARSE_RETURN;
	}
	if(!this->ParseFloat(szCurrent,&szCurrent,vRot.x))
	{
		this->LogErrorNoThrow("Unexpected EOF/EOL while parsing bone.rot.x");
		SMDI_PARSE_RETURN;
	}
	if(!this->ParseFloat(szCurrent,&szCurrent,vRot.y))
	{
		this->LogErrorNoThrow("Unexpected EOF/EOL while parsing bone.rot.y");
		SMDI_PARSE_RETURN;
	}
	if(!this->ParseFloat(szCurrent,&szCurrent,vRot.z))
	{
		this->LogErrorNoThrow("Unexpected EOF/EOL while parsing bone.rot.z");
		SMDI_PARSE_RETURN;
	}
	// build the transformation matrix of the key
	key.matrix.FromEulerAngles(vRot.x,vRot.y,vRot.z);
	{
		aiMatrix4x4 mTemp;
		mTemp.a4 = vPos.x;
		mTemp.b4 = vPos.y;
		mTemp.c4 = vPos.z;
		key.matrix = key.matrix * mTemp;
	}

	// go to the beginning of the next line
	SMDI_PARSE_RETURN;
}
// ------------------------------------------------------------------------------------------------
// Parse a triangle
void SMDImporter::ParseTriangle(const char* szCurrent,
	const char** szCurrentOut)
{
	this->asTriangles.push_back(SMD::Face());
	SMD::Face& face = this->asTriangles.back();
	
	if(!SkipSpaces(szCurrent,&szCurrent))
	{
		this->LogErrorNoThrow("Unexpected EOF/EOL while parsing a triangle");
		return;
	}

	// read the texture file name
	const char* szLast = szCurrent;
	while (!IsSpaceOrNewLine(*szCurrent++));

	face.iTexture = this->GetTextureIndex(std::string(szLast,
		(uintptr_t)szCurrent-(uintptr_t)szLast));

	this->SkipLine(szCurrent,&szCurrent);

	// load three vertices
	for (unsigned int iVert = 0; iVert < 3;++iVert)
	{
		this->ParseVertex(szCurrent,&szCurrent,
			face.avVertices[iVert]);
	}
	*szCurrentOut = szCurrent;
}
// ------------------------------------------------------------------------------------------------
// Parse a float
bool SMDImporter::ParseFloat(const char* szCurrent,
	const char** szCurrentOut, float& out)
{
	if(!SkipSpaces(szCurrent,&szCurrent))
		return false;

	*szCurrentOut = fast_atof_move(szCurrent,out);
	return true;
}
// ------------------------------------------------------------------------------------------------
// Parse an unsigned int
bool SMDImporter::ParseUnsignedInt(const char* szCurrent,
	const char** szCurrentOut, uint32_t& out)
{
	if(!SkipSpaces(szCurrent,&szCurrent))
		return false;

	out = (uint32_t)strtol10(szCurrent,szCurrentOut);
	return true;
}
// ------------------------------------------------------------------------------------------------
// Parse a signed int
bool SMDImporter::ParseSignedInt(const char* szCurrent,
	const char** szCurrentOut, int32_t& out)
{
	if(!SkipSpaces(szCurrent,&szCurrent))
		return false;

	out = (int32_t)strtol10s(szCurrent,szCurrentOut);
	return true;
}
// ------------------------------------------------------------------------------------------------
// Parse a vertex
void SMDImporter::ParseVertex(const char* szCurrent,
	const char** szCurrentOut, SMD::Vertex& vertex,
	bool bVASection /*= false*/)
{
	if(!this->ParseSignedInt(szCurrent,&szCurrent,(int32_t&)vertex.iParentNode))
	{
		this->LogErrorNoThrow("Unexpected EOF/EOL while parsing vertex.parent");
		SMDI_PARSE_RETURN;
	}
	if(!this->ParseFloat(szCurrent,&szCurrent,vertex.pos.x))
	{
		this->LogErrorNoThrow("Unexpected EOF/EOL while parsing vertex.pos.x");
		SMDI_PARSE_RETURN;
	}
	if(!this->ParseFloat(szCurrent,&szCurrent,vertex.pos.y))
	{
		this->LogErrorNoThrow("Unexpected EOF/EOL while parsing vertex.pos.y");
		SMDI_PARSE_RETURN;
	}
	if(!this->ParseFloat(szCurrent,&szCurrent,vertex.pos.z))
	{
		this->LogErrorNoThrow("Unexpected EOF/EOL while parsing vertex.pos.z");
		SMDI_PARSE_RETURN;
	}
	if(!this->ParseFloat(szCurrent,&szCurrent,vertex.nor.x))
	{
		this->LogErrorNoThrow("Unexpected EOF/EOL while parsing vertex.nor.x");
		SMDI_PARSE_RETURN;
	}
	if(!this->ParseFloat(szCurrent,&szCurrent,vertex.nor.y))
	{
		this->LogErrorNoThrow("Unexpected EOF/EOL while parsing vertex.nor.y");
		SMDI_PARSE_RETURN;
	}
	if(!this->ParseFloat(szCurrent,&szCurrent,vertex.nor.z))
	{
		this->LogErrorNoThrow("Unexpected EOF/EOL while parsing vertex.nor.z");
		SMDI_PARSE_RETURN;
	}

	if (bVASection)SMDI_PARSE_RETURN;

	if(!this->ParseFloat(szCurrent,&szCurrent,vertex.uv.x))
	{
		this->LogErrorNoThrow("Unexpected EOF/EOL while parsing vertex.uv.x");
		SMDI_PARSE_RETURN;
	}
	if(!this->ParseFloat(szCurrent,&szCurrent,vertex.uv.y))
	{
		this->LogErrorNoThrow("Unexpected EOF/EOL while parsing vertex.uv.y");
		SMDI_PARSE_RETURN;
	}

	// now read the number of bones affecting this vertex
	// all elements from now are fully optional, we don't need them
	unsigned int iSize = 0;
	if(!this->ParseUnsignedInt(szCurrent,&szCurrent,iSize))SMDI_PARSE_RETURN;
	vertex.aiBoneLinks.resize(iSize,std::pair<unsigned int, float>(-1,0.0f));

	for (std::vector<std::pair<unsigned int, float> >::iterator
		i =  vertex.aiBoneLinks.begin();
		i != vertex.aiBoneLinks.end();++i)
	{
		if(!this->ParseUnsignedInt(szCurrent,&szCurrent,(*i).first))SMDI_PARSE_RETURN;
		if(!this->ParseFloat(szCurrent,&szCurrent,(*i).second))SMDI_PARSE_RETURN;
	}

	// go to the beginning of the next line
	SMDI_PARSE_RETURN;
}