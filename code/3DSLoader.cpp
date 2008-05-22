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

/** @file Implementation of the 3ds importer class */
#include "3DSLoader.h"
#include "MaterialSystem.h"

#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"
#include "../include/aiAssert.h"

#include <boost/scoped_ptr.hpp>

using namespace Assimp;

#define ASSIMP_3DS_WARN_CHUNK_OVERFLOW_MSG			\
	"WARNING: Size of chunk data plus size of "		\
	"subordinate chunks is larger than the size "	\
	"specified in the higher-level chunk header."	\



// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
Dot3DSImporter::Dot3DSImporter()
{
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
Dot3DSImporter::~Dot3DSImporter()
{
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool Dot3DSImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
{
	// simple check of file extension is enough for the moment
	std::string::size_type pos = pFile.find_last_of('.');
	// no file extension - can't read
	if( pos == std::string::npos)
		return false;
	std::string extension = pFile.substr( pos);

	// not brilliant but working ;-)
	if( extension == ".3ds" || extension == ".3DS" || 
		extension == ".3Ds" || extension == ".3dS")
		return true;

	return false;
}
// ------------------------------------------------------------------------------------------------
// recursively delete a given node
void DeleteNodeRecursively (aiNode* p_piNode)
{
	if (!p_piNode)return;

	if (p_piNode->mChildren)
	{
		for (unsigned int i = 0 ; i < p_piNode->mNumChildren;++i)
		{
			DeleteNodeRecursively(p_piNode->mChildren[i]);
		}
	}
	delete p_piNode;
	return;
}
// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void Dot3DSImporter::InternReadFile( 
	const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile));

	// Check whether we can read from the file
	if( file.get() == NULL)
	{
		throw new ImportErrorException( "Failed to open file " + pFile + ".");
	}

	// check whether the .3ds file is large enough to contain
	// at least one chunk.
	size_t fileSize = file->FileSize();
	if( fileSize < 16)
	{
		throw new ImportErrorException( ".3ds File is too small.");
	}

	this->mScene = new Dot3DS::Scene();

	// allocate storage and copy the contents of the file to a memory buffer
	this->mBuffer = new unsigned char[fileSize];
	file->Read( mBuffer, 1, fileSize);
	this->mCurrent = this->mBuffer;
	this->mLast = this->mBuffer+fileSize;

	// initialize members
	this->mLastNodeIndex = -1;
	this->mCurrentNode = new Dot3DS::Node();
	this->mRootNode = this->mCurrentNode;
	this->mRootNode->mHierarchyPos = -1;
	this->mRootNode->mHierarchyIndex = -1;
	this->mRootNode->mParent = NULL;
	this->mMasterScale = 1.0f;
	this->mBackgroundImage = "";
	this->bHasBG = false;

	int iRemaining = (unsigned int)fileSize;
	this->ParseMainChunk(&iRemaining);

	// Generate an unique set of vertices/indices for
	// all meshes contained in the file
	for (std::vector<Dot3DS::Mesh>::iterator
		i =  this->mScene->mMeshes.begin();
		i != this->mScene->mMeshes.end();++i)
	{
		// TODO: see function body
		this->CheckIndices(&(*i));
		this->MakeUnique(&(*i));

		// first generate normals for the mesh
		this->GenNormals(&(*i));
	}

	// Apply scaling and offsets to all texture coordinates
	this->ApplyScaleNOffset();

	// Replace all occurences of the default material with a valid material.
	// Generate it if no material containing DEFAULT in its name has been
	// found in the file
	this->ReplaceDefaultMaterial();

	try 
		{
		// Convert the scene from our internal representation to an aiScene object
		this->ConvertScene(pScene);
		}
	catch (ImportErrorException ex)
		{
		// delete the scene itself
		if (pScene->mMeshes)
			{
			for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
				delete pScene->mMeshes[i];
			delete[] pScene->mMeshes;
			}
		if (pScene->mMaterials)
			{
			for (unsigned int i = 0; i < pScene->mNumMaterials;++i)
				delete pScene->mMaterials[i];
			delete[] pScene->mMaterials;
			}
		// there are no animations
		if (pScene->mRootNode)DeleteNodeRecursively(pScene->mRootNode);
		throw ex;
		}

	// Generate the node graph for the scene. This is a little bit
	// tricky since we'll need to split some meshes into submeshes
	this->GenerateNodeGraph(pScene);

	// Now apply a master scaling factor to the scene
	this->ApplyMasterScale(pScene);

	delete[] this->mBuffer;
	delete this->mScene;
	return;
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::ApplyMasterScale(aiScene* pScene)
{
	// NOTE: Some invalid files have masterscale set to 0.0
	if (0.0f == this->mMasterScale)
		{
		this->mMasterScale = 1.0f;
		}
	else this->mMasterScale = 1.0f / this->mMasterScale;

	// construct an uniform scaling matrix and multiply with it
	pScene->mRootNode->mTransformation *= aiMatrix4x4( 
		this->mMasterScale,0.0f, 0.0f, 0.0f,
		0.0f, this->mMasterScale,0.0f, 0.0f,
		0.0f, 0.0f, this->mMasterScale,0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::ReadChunk(const Dot3DSFile::Chunk** p_ppcOut)
{
	ai_assert(p_ppcOut != NULL);

	// read chunk
	if ((unsigned int)this->mCurrent >= (unsigned int)this->mLast)
	{
		*p_ppcOut = NULL;
		return;
	}
	const unsigned int iDiff = (unsigned int)this->mLast - (unsigned int)this->mCurrent;
	if (iDiff < sizeof(Dot3DSFile::Chunk)) 
	{
		*p_ppcOut = NULL;
		return;
	}

	*p_ppcOut = (const Dot3DSFile::Chunk*) this->mCurrent;
	this->mCurrent += sizeof(Dot3DSFile::Chunk);
	return;
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::ParseMainChunk(int* piRemaining)
{
	const Dot3DSFile::Chunk* psChunk;

	this->ReadChunk(&psChunk);
	if (NULL == psChunk)return;

	const unsigned char* pcCur = this->mCurrent;
	const unsigned char* pcCurNext = pcCur + (psChunk->Size 
		- sizeof(Dot3DSFile::Chunk));

	// get chunk type
	int iRemaining = (psChunk->Size - sizeof(Dot3DSFile::Chunk));
	switch (psChunk->Flag)
	{
	case Dot3DSFile::CHUNK_MAIN:
	//case 0x444d: // bugfix

		this->ParseEditorChunk(&iRemaining);
		break;
	};
	if ((unsigned int)pcCurNext < (unsigned int)this->mCurrent)
	{
		// place an error message. If we crash the programmer
		// will be able to find it
		this->mErrorText = ASSIMP_3DS_WARN_CHUNK_OVERFLOW_MSG;
		pcCurNext = this->mCurrent;
	}
	// Go to the starting position of the next top-level chunk
	this->mCurrent = pcCurNext;
	*piRemaining -= psChunk->Size;
	if (0 >= *piRemaining)return;
	return this->ParseMainChunk(piRemaining);
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::ParseEditorChunk(int* piRemaining)
{
	const Dot3DSFile::Chunk* psChunk;

	this->ReadChunk(&psChunk);
	if (NULL == psChunk)return;

	const unsigned char* pcCur = this->mCurrent;
	const unsigned char* pcCurNext = pcCur + (psChunk->Size 
		- sizeof(Dot3DSFile::Chunk));

	// get chunk type
	int iRemaining = (psChunk->Size - sizeof(Dot3DSFile::Chunk));
	switch (psChunk->Flag)
	{
	case Dot3DSFile::CHUNK_OBJMESH:

		this->ParseObjectChunk(&iRemaining);
		break;

	// NOTE: In several documentations in the internet this
	// chunk appears at different locations
	case Dot3DSFile::CHUNK_KEYFRAMER:

		this->ParseKeyframeChunk(&iRemaining);
		break;
	};
	if ((unsigned int)pcCurNext < (unsigned int)this->mCurrent)
	{
		// place an error message. If we crash the programmer
		// will be able to find it
		this->mErrorText = ASSIMP_3DS_WARN_CHUNK_OVERFLOW_MSG;
		pcCurNext = this->mCurrent;
	}
	// Go to the starting position of the next top-level chunk
	this->mCurrent = pcCurNext;
	*piRemaining -= psChunk->Size;
	if (0 >= *piRemaining)return;
	return this->ParseEditorChunk(piRemaining);
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::ParseObjectChunk(int* piRemaining)
{
	const Dot3DSFile::Chunk* psChunk;

	this->ReadChunk(&psChunk);
	if (NULL == psChunk)return;

	const unsigned char* pcCur = this->mCurrent;
	const unsigned char* pcCurNext = pcCur + (psChunk->Size 
		- sizeof(Dot3DSFile::Chunk));

	const unsigned char* sz = this->mCurrent;
	unsigned int iCnt = 0;

	// get chunk type
	int iRemaining = (psChunk->Size - sizeof(Dot3DSFile::Chunk));
	switch (psChunk->Flag)
	{
	case Dot3DSFile::CHUNK_OBJBLOCK:

		this->mScene->mMeshes.push_back(Dot3DS::Mesh());

		// at first we need to parse the name of the
		// geometry object

		while (*sz++ != '\0')
		{
			if (sz > pcCurNext-1)break;
			++iCnt;
		}
		this->mScene->mMeshes.back().mName = std::string(
			(const char*)this->mCurrent,iCnt);
		++iCnt;

		this->mCurrent += iCnt;
		iRemaining -= iCnt;
		this->ParseChunk(&iRemaining);
		break;

	case Dot3DSFile::CHUNK_MAT_MATERIAL:

		this->mScene->mMaterials.push_back(Dot3DS::Material());
		this->ParseMaterialChunk(&iRemaining);
		break;

	case Dot3DSFile::CHUNK_AMBCOLOR:

		// This is the ambient base color of the scene.
		// We add it to the ambient color of all materials
		this->ParseColorChunk(&this->mClrAmbient,true);
		if (is_qnan(this->mClrAmbient.r))
			{
			this->mClrAmbient.r = 0.0f;
			this->mClrAmbient.g = 0.0f;
			this->mClrAmbient.b = 0.0f;
			}
		break;


	case Dot3DSFile::CHUNK_BIT_MAP:

		this->mBackgroundImage = std::string((const char*)this->mCurrent);
		break;


	case Dot3DSFile::CHUNK_BIT_MAP_EXISTS:
		bHasBG = true;
		break;


	case Dot3DSFile::CHUNK_MASTER_SCALE:

		this->mMasterScale = *((float*)this->mCurrent);
		this->mCurrent += sizeof(float);
		break;

	// NOTE: In several documentations in the internet this
	// chunk appears at different locations
	case Dot3DSFile::CHUNK_KEYFRAMER:

		this->ParseKeyframeChunk(&iRemaining);
		break;

	};
	if ((unsigned int)pcCurNext < (unsigned int)this->mCurrent)
	{
		// place an error message. If we crash the programmer
		// will be able to find it
		this->mErrorText = ASSIMP_3DS_WARN_CHUNK_OVERFLOW_MSG;
		pcCurNext = this->mCurrent;
	}
	// Go to the starting position of the next top-level chunk
	this->mCurrent = pcCurNext;
	*piRemaining -= psChunk->Size;
	if (0 >= *piRemaining)return;
	return this->ParseObjectChunk(piRemaining);
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::SkipChunk()
{
	const Dot3DSFile::Chunk* psChunk;
	this->ReadChunk(&psChunk);
	if (NULL == psChunk)return;
	this->mCurrent += psChunk->Size - sizeof(Dot3DSFile::Chunk);
	return;
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::ParseChunk(int* piRemaining)
{
	const Dot3DSFile::Chunk* psChunk;

	this->ReadChunk(&psChunk);
	if (NULL == psChunk)return;

	const unsigned char* pcCur = this->mCurrent;
	const unsigned char* pcCurNext = pcCur + (psChunk->Size 
		- sizeof(Dot3DSFile::Chunk));

	// get chunk type
	int iRemaining = (psChunk->Size - sizeof(Dot3DSFile::Chunk));
	switch (psChunk->Flag)
	{
	case Dot3DSFile::CHUNK_TRIMESH:
		// this starts a new mesh
		this->ParseMeshChunk(&iRemaining);
		break;
	};
	if ((unsigned int)pcCurNext < (unsigned int)this->mCurrent)
	{
		// place an error message. If we crash the programmer
		// will be able to find it
		this->mErrorText = ASSIMP_3DS_WARN_CHUNK_OVERFLOW_MSG;
		pcCurNext = this->mCurrent;
	}
	// Go to the starting position of the next top-level chunk
	this->mCurrent = pcCurNext;

	*piRemaining -= psChunk->Size;
	if (0 >= *piRemaining)return;
	return this->ParseChunk(piRemaining);
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::ParseKeyframeChunk(int* piRemaining)
{
	const Dot3DSFile::Chunk* psChunk;

	this->ReadChunk(&psChunk);
	if (NULL == psChunk)return;

	const unsigned char* pcCur = this->mCurrent;
	const unsigned char* pcCurNext = pcCur + (psChunk->Size 
		- sizeof(Dot3DSFile::Chunk));

	// get chunk type
	int iRemaining = (psChunk->Size - sizeof(Dot3DSFile::Chunk));
	switch (psChunk->Flag)
	{
	case Dot3DSFile::CHUNK_TRACKINFO:
		// this starts a new mesh
		this->ParseHierarchyChunk(&iRemaining);
		break;
	};
	if ((unsigned int)pcCurNext < (unsigned int)this->mCurrent)
	{
		// place an error message. If we crash the programmer
		// will be able to find it
		this->mErrorText = ASSIMP_3DS_WARN_CHUNK_OVERFLOW_MSG;
		pcCurNext = this->mCurrent;
	}
	// Go to the starting position of the next top-level chunk
	this->mCurrent = pcCurNext;

	*piRemaining -= psChunk->Size;
	if (0 >= *piRemaining)return;
	return this->ParseKeyframeChunk(piRemaining);
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::InverseNodeSearch(Dot3DS::Node* pcNode,Dot3DS::Node* pcCurrent)
{
	if (NULL == pcCurrent)
	{
		this->mRootNode->push_back(pcNode);
		return;
	}
	if (pcCurrent->mHierarchyPos == pcNode->mHierarchyPos)
	{
		if(NULL != pcCurrent->mParent)
			pcCurrent->mParent->push_back(pcNode);
		else pcCurrent->push_back(pcNode);
		return;
	}
	return this->InverseNodeSearch(pcNode,pcCurrent->mParent);
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::ParseHierarchyChunk(int* piRemaining)
{
	const Dot3DSFile::Chunk* psChunk;

	this->ReadChunk(&psChunk);
	if (NULL == psChunk)return;

	const unsigned char* pcCur = this->mCurrent;
	const unsigned char* pcCurNext = pcCur + (psChunk->Size 
		- sizeof(Dot3DSFile::Chunk));

	// get chunk type
	const unsigned char* sz = (unsigned char*)this->mCurrent;
	unsigned int iCnt = 0;
	uint16_t iHierarchy;
	//uint16_t iTemp;
	Dot3DS::Node* pcNode;
	switch (psChunk->Flag)
	{
	case Dot3DSFile::CHUNK_TRACKOBJNAME:
		
		// get object name
		while (*sz++ != '\0')
		{
			if (sz > pcCurNext-1)break;
			++iCnt;
		}
		pcNode = new Dot3DS::Node();
		pcNode->mName = std::string((const char*)this->mCurrent,iCnt);

		iCnt++;
		// there are two unknown values which we can safely ignore
		this->mCurrent += iCnt + sizeof(uint16_t)*2;
		iHierarchy = *((uint16_t*)this->mCurrent);
		iHierarchy++;
		pcNode->mHierarchyPos = iHierarchy;
		pcNode->mHierarchyIndex = this->mLastNodeIndex;
		if (iHierarchy > this->mLastNodeIndex)
		{
			// place it at the current position in the hierarchy
			this->mCurrentNode->push_back(pcNode);
		}
		else
		{
			// need to go back to the specified position in the hierarchy.
			this->InverseNodeSearch(pcNode,this->mCurrentNode);
		}
		this->mLastNodeIndex++;
		this->mCurrentNode = pcNode;
		break;

#if 0

	case Dot3DSFile::CHUNK_TRACKPIVOT:

		this->mCurrentNode->vPivot = *((aiVector3D*)this->mCurrent);
		this->mCurrent += sizeof(aiVector3D);
		break;


	case Dot3DSFile::CHUNK_TRACKPOS:

		/*
		+2 short flags; 
		+8 short unknown[4];
		+2 short keys;
		+2 short unknown;
		struct {
		+2 short framenum;
		+4 long unknown;
		float pos_x, pos_y, pos_z; 
		}  pos[keys]; 	
		*/
		this->mCurrent += 10;
		iTemp = *((uint16_t*)mCurrent);

		this->mCurrent += sizeof(uint16_t) * 2;

		if (0 != iTemp)
			{
			for (unsigned int i = 0; i < (unsigned int)iTemp;++i)
				{
				uint16_t sNum = *((uint16_t*)mCurrent);
				this->mCurrent += sizeof(uint16_t);

				if (0 == sNum)
					{
					this->mCurrent += sizeof(uint32_t);
					this->mCurrentNode->vPosition = *((aiVector3D*)this->mCurrent);
					this->mCurrent += sizeof(aiVector3D);
					}
				else this->mCurrent += sizeof(uint32_t) + sizeof(aiVector3D);
				}
			}
		break;

	case Dot3DSFile::CHUNK_TRACKROTATE:

		/*
		+2 short flags; 
		+8 short unknown[4];
		+2 short keys;
		+2 short unknown;
		struct {
		+2 short framenum;
		+4 long unknown;
		float rad , pos_x, pos_y, pos_z; 
		}  pos[keys]; 	
		*/
		this->mCurrent += 10;
		iTemp = *((uint16_t*)mCurrent);

		this->mCurrent += sizeof(uint16_t) * 2;
		if (0 != iTemp)
			{
			bool neg = false;
			unsigned int iNum0 = 0;
			for (unsigned int i = 0; i < (unsigned int)iTemp;++i)
				{
				uint16_t sNum = *((uint16_t*)mCurrent);
				this->mCurrent += sizeof(uint16_t);

				if (0 == sNum)
					{
					this->mCurrent		+= sizeof(uint32_t);
					float fRadians		= *((float*)this->mCurrent);
					this->mCurrent		+= sizeof(float);
					aiVector3D vAxis	= *((aiVector3D*)this->mCurrent);
					this->mCurrent		+= sizeof(aiVector3D);

					// some idiotic files have rotations with fRadians = 0 ...
					if (0.0f != fRadians)
						{

						// if the radians go beyond PI then the rotations 
						// thereafter must be inversed
#if 0
						if (neg)fRadians *= -1.0f;
						if ((fRadians >= 3.1415926f || fRadians <= -3.1415926f))
							{
							neg = !neg;
							}
#endif 


						// get the rotation matrix around the axis
						const float fSin = sinf(-fRadians);
						const float fCos = cosf(-fRadians);
						const float fOneMinusCos = 1.0f - fCos;

						std::swap(vAxis.z,vAxis.y);
						vAxis.Normalize();

						aiMatrix4x4 mRot = aiMatrix4x4(
							(vAxis.x * vAxis.x) * fOneMinusCos + fCos,
							(vAxis.x * vAxis.y) * fOneMinusCos - (vAxis.z * fSin),
							(vAxis.x * vAxis.z) * fOneMinusCos + (vAxis.y * fSin),
							0.0f,
							(vAxis.y * vAxis.x) * fOneMinusCos + (vAxis.z * fSin),
							(vAxis.y * vAxis.y) * fOneMinusCos + fCos,
							(vAxis.y * vAxis.z) * fOneMinusCos - (vAxis.x * fSin),
							0.0f,
							(vAxis.z * vAxis.x) * fOneMinusCos - (vAxis.y * fSin),
							(vAxis.z * vAxis.y) * fOneMinusCos + (vAxis.x * fSin),
							(vAxis.z * vAxis.z) * fOneMinusCos + fCos,
							0.0f,0.0f,0.0f,0.0f,1.0f);
						//mRot.Transpose();

						// build a chain of concatenated rotation matrix'
						// if there are multiple track chunks for the same frame
						if (0 != iNum0)
							{
							this->mCurrentNode->mRotation = this->mCurrentNode->mRotation * mRot;
							}
						else
							{
							// for the first time simply set the rotation matrix
							this->mCurrentNode->mRotation = mRot;
							}
						iNum0++;
						}
					}
				else this->mCurrent += sizeof(uint32_t) + sizeof(aiVector3D) + sizeof(float);
				}
			}
		break;

	case Dot3DSFile::CHUNK_TRACKSCALE:

		/*
		+2 short flags; 
		+8 short unknown[4];
		+2 short keys;
		+2 short unknown;
		struct {
		+2 short framenum;
		+4 long unknown;
		float pos_x, pos_y, pos_z; 
		}  pos[keys]; 	
		*/
		this->mCurrent += 10;
		iTemp = *((uint16_t*)mCurrent);

		this->mCurrent += sizeof(uint16_t) * 2;

		if (0 != iTemp)
			{
			for (unsigned int i = 0; i < (unsigned int)iTemp;++i)
				{
				uint16_t sNum = *((uint16_t*)mCurrent);
				this->mCurrent += sizeof(uint16_t);
				if (0 == sNum)
					{
					this->mCurrent += sizeof(uint32_t);
					aiVector3D vMe = *((aiVector3D*)this->mCurrent);
					// ignore zero scalings
					if (0.0f != vMe.x && 0.0f != vMe.y && 0.0f != vMe.z)
						{
						this->mCurrentNode->vScaling.x *= vMe.x;
						this->mCurrentNode->vScaling.y *= vMe.y;
						this->mCurrentNode->vScaling.z *= vMe.z;
						}
					this->mCurrent += sizeof(aiVector3D);
					}
				else this->mCurrent += sizeof(uint32_t) + sizeof(aiVector3D);
				}
			}
		break;

#endif // 0
	};
	if ((unsigned int)pcCurNext < (unsigned int)this->mCurrent)
	{
		// place an error message. If we crash the programmer
		// will be able to find it
		this->mErrorText = ASSIMP_3DS_WARN_CHUNK_OVERFLOW_MSG;
		pcCurNext = this->mCurrent;
	}
	// Go to the starting position of the next top-level chunk
	this->mCurrent = pcCurNext;

	*piRemaining -= psChunk->Size;
	if (0 >= *piRemaining)return;
	return this->ParseHierarchyChunk(piRemaining);
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::ParseFaceChunk(int* piRemaining)
{
	const Dot3DSFile::Chunk* psChunk;

	Dot3DS::Mesh& mMesh = this->mScene->mMeshes.back();

	this->ReadChunk(&psChunk);
	if (NULL == psChunk)return;

	const unsigned char* pcCur = this->mCurrent;
	const unsigned char* pcCurNext = pcCur + (psChunk->Size 
		- sizeof(Dot3DSFile::Chunk));

	// get chunk type
	const unsigned char* sz = this->mCurrent;
	uint32_t iCnt = 0,iTemp;
	switch (psChunk->Flag)
	{
	case Dot3DSFile::CHUNK_SMOOLIST:

		// one int32 for each face
		for (std::vector<Dot3DS::Face>::iterator
			i =  mMesh.mFaces.begin();
			i != mMesh.mFaces.end();++i)
		{
			// nth bit is set for nth smoothing group
			(*i).iSmoothGroup = *((uint32_t*)this->mCurrent);
#if 0
			for (unsigned int x = 0, a = 1; x < 32;++x,a <<= 1)
			{
				if ((*i).iSmoothGroup & a)
					mMesh.bSmoothGroupRequired[x] = true;
			}
#endif
			this->mCurrent += sizeof(uint32_t);
		}
		break;

	case Dot3DSFile::CHUNK_FACEMAT:

		// at fist an asciiz with the material name
		while (*sz++ != '\0')
		{
			if (sz > pcCurNext-1)break;
		}

		// find the index of the material
		unsigned int iIndex = 0xFFFFFFFF;
		iCnt = 0;
		for (std::vector<Dot3DS::Material>::const_iterator
			i =  this->mScene->mMaterials.begin();
			i != this->mScene->mMaterials.end();++i,++iCnt)
		{
			// compare case-independent to be sure it works
			if (0 == ASSIMP_stricmp((const char*)this->mCurrent,
				(const char*)((*i).mName.c_str())))
			{
			iIndex = iCnt;
			break;
			}
		}
		if (iIndex == 0xFFFFFFFF)
		{
			// this material is not known. Ignore this. We will later
			// assign the default material to all faces using *this*
			// material. Use 0xcdcdcdcd as special value to indicate
			// this.
			iIndex = 0xcdcdcdcd;
		}
		this->mCurrent = sz;
		iCnt = (int)(*((uint16_t*)this->mCurrent));
		this->mCurrent += sizeof(uint16_t);

		for (unsigned int i = 0; i < iCnt;++i)
		{
			iTemp = (uint16_t)*((uint16_t*)this->mCurrent);

			// check range
			if (iTemp >= mMesh.mFaceMaterials.size())
				{
				mMesh.mFaceMaterials[mMesh.mFaceMaterials.size()-1] = iIndex;
				}
			else
				{
				mMesh.mFaceMaterials[iTemp] = iIndex;
				}
			this->mCurrent += sizeof(uint16_t);
		}

		break;
	};
	if ((unsigned int)pcCurNext < (unsigned int)this->mCurrent)
	{
		// place an error message. If we crash the programmer
		// will be able to find it
		this->mErrorText = ASSIMP_3DS_WARN_CHUNK_OVERFLOW_MSG;
		pcCurNext = this->mCurrent;
	}
	// Go to the starting position of the next chunk on this level
	this->mCurrent = pcCurNext;

	*piRemaining -= psChunk->Size;
	if (0 >= *piRemaining)return;
	return ParseFaceChunk(piRemaining);
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::ParseMeshChunk(int* piRemaining)
{
	const Dot3DSFile::Chunk* psChunk;

	Dot3DS::Mesh& mMesh = this->mScene->mMeshes.back();

	this->ReadChunk(&psChunk);
	if (NULL == psChunk)return;

	const unsigned char* pcCur = this->mCurrent;
	const unsigned char* pcCurNext = pcCur + (psChunk->Size 
		- sizeof(Dot3DSFile::Chunk));

	// get chunk type
	const unsigned char* sz = this->mCurrent;
	unsigned int iCnt = 0;
	int iRemaining;
	uint16_t iNum = 0;
	float* pf;
	switch (psChunk->Flag)
	{
	case Dot3DSFile::CHUNK_VERTLIST:

		iNum = *((short*)this->mCurrent);
		this->mCurrent += sizeof(short);
		while (iNum-- > 0)
		{
			mMesh.mPositions.push_back(*((aiVector3D*)this->mCurrent));
			mMesh.mPositions.back().z *= -1.0f;
			this->mCurrent += sizeof(aiVector3D);
		}
		break;

	case Dot3DSFile::CHUNK_TRMATRIX:
		{
		// http://www.gamedev.net/community/forums/topic.asp?topic_id=263063
		// http://www.gamedev.net/community/forums/topic.asp?topic_id=392310
		pf = (float*)this->mCurrent;
		this->mCurrent += 12 * sizeof(float);

		mMesh.mMat.a1 = pf[0];
		mMesh.mMat.a2 = pf[1];
		mMesh.mMat.a3 = pf[2];
		mMesh.mMat.b1 = pf[3];
		mMesh.mMat.b2 = pf[4];
		mMesh.mMat.b3 = pf[5];
		mMesh.mMat.c1 = pf[6];
		mMesh.mMat.c2 = pf[7];
		mMesh.mMat.c3 = pf[8];
		mMesh.mMat.d1 = pf[9];
		mMesh.mMat.d2 = pf[10];
		mMesh.mMat.d3 = pf[11];

		std::swap((float&)mMesh.mMat.d2, (float&)mMesh.mMat.d3);
		std::swap((float&)mMesh.mMat.a2, (float&)mMesh.mMat.a3);
		std::swap((float&)mMesh.mMat.b1, (float&)mMesh.mMat.c1);
		std::swap((float&)mMesh.mMat.c2, (float&)mMesh.mMat.b3);
		std::swap((float&)mMesh.mMat.b2, (float&)mMesh.mMat.c3);

		mMesh.mMat.Transpose();

		//aiMatrix4x4 mInv = mMesh.mMat;
		//mInv.Inverse();

		//// invert the matrix and transform all vertices with it
		//// (the origin of all vertices is 0|0|0 now)
		//for (register unsigned int i = 0; i < mMesh.mPositions.size();++i)
		//	{
		//	aiVector3D a,c;
		//	a = mMesh.mPositions[i];
		//	c[0]= mInv[0][0]*a[0] + mInv[1][0]*a[1] + mInv[2][0]*a[2] + mInv[3][0];
		//	c[1]= mInv[0][1]*a[0] + mInv[1][1]*a[1] + mInv[2][1]*a[2] + mInv[3][1];
		//	c[2]= mInv[0][2]*a[0] + mInv[1][2]*a[1] + mInv[2][2]*a[2] + mInv[3][2];
		//	mMesh.mPositions[i] = c;
		//	}

		// now check whether the matrix has got a negative determinant
		// If yes, we need to flip all vertices x axis ....
		// From lib3ds, mesh.c
		if (mMesh.mMat.Determinant() < 0.0f)
			{
			aiMatrix4x4 mInv = mMesh.mMat;
			mInv.Inverse();

			aiMatrix4x4 mMe = mMesh.mMat;
			mMe.a1 *= -1.0f;
			mMe.a2 *= -1.0f;
			mMe.a3 *= -1.0f;
			mMe.a4 *= -1.0f;
			mInv = mMe * mInv;
			for (register unsigned int i = 0; i < mMesh.mPositions.size();++i)
				{
				aiVector3D a,c;
				a = mMesh.mPositions[i];
				c[0]= mInv[0][0]*a[0] + mInv[1][0]*a[1] + mInv[2][0]*a[2] + mInv[3][0];
				c[1]= mInv[0][1]*a[0] + mInv[1][1]*a[1] + mInv[2][1]*a[2] + mInv[3][1];
				c[2]= mInv[0][2]*a[0] + mInv[1][2]*a[1] + mInv[2][2]*a[2] + mInv[3][2];
				mMesh.mPositions[i] = c;
				}
			}
		}
		break;

	case Dot3DSFile::CHUNK_MAPLIST:

		iNum = *((uint16_t*)this->mCurrent);
		this->mCurrent += sizeof(uint16_t);
		while (iNum-- > 0)
		{
			mMesh.mTexCoords.push_back(*((aiVector2D*)this->mCurrent));
			this->mCurrent += sizeof(aiVector2D);
		}
		break;

#if (defined _DEBUG)

	case Dot3DSFile::CHUNK_TXTINFO:

		// for debugging purposes. Read two bytes to determine the mapping type
		iNum = *((uint16_t*)this->mCurrent);
		this->mCurrent += sizeof(uint16_t);
	break;
#endif

	case Dot3DSFile::CHUNK_FACELIST:

		iNum = *((uint16_t*)this->mCurrent);
		this->mCurrent += sizeof(uint16_t);
		while (iNum-- > 0)
		{
			Dot3DS::Face sFace;
			sFace.i1 = *((uint16_t*)this->mCurrent);
			this->mCurrent += sizeof(uint16_t);
			sFace.i2 = *((uint16_t*)this->mCurrent);
			this->mCurrent += sizeof(uint16_t);
			sFace.i3 = *((uint16_t*)this->mCurrent);
			this->mCurrent += 2*sizeof(uint16_t);
			mMesh.mFaces.push_back(sFace);

			//if (sFace.i1 < sFace.i2)sFace.bDirection = false;
		}

		// resize the material array (0xcdcdcdcd marks the
		// default material; so if a face is not referenced
		// by a material $$DEFAULT will be assigned to it)
		mMesh.mFaceMaterials.resize(mMesh.mFaces.size(),0xcdcdcdcd);

		iRemaining = (int)pcCurNext - (int)this->mCurrent;
		if (iRemaining > 0)this->ParseFaceChunk(&iRemaining);
		break;

	};
	if ((unsigned int)pcCurNext < (unsigned int)this->mCurrent)
	{
		// place an error message. If we crash the programmer
		// will be able to find it
		this->mErrorText = ASSIMP_3DS_WARN_CHUNK_OVERFLOW_MSG;
		pcCurNext = this->mCurrent;
	}
	// Go to the starting position of the next chunk on this level
	this->mCurrent = pcCurNext;

	*piRemaining -= psChunk->Size;
	if (0 >= *piRemaining)return;
	return ParseMeshChunk(piRemaining);
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::ParseMaterialChunk(int* piRemaining)
{
	const Dot3DSFile::Chunk* psChunk;

	this->ReadChunk(&psChunk);
	if (NULL == psChunk)return;

	const unsigned char* pcCur = this->mCurrent;
	const unsigned char* pcCurNext = pcCur + (psChunk->Size 
		- sizeof(Dot3DSFile::Chunk));

	// get chunk type
	const unsigned char* sz = this->mCurrent;
	unsigned int iCnt = 0;
	int iRemaining;
	aiColor3D* pc;
	float* pcf;
	switch (psChunk->Flag)
	{
	case Dot3DSFile::CHUNK_MAT_MATNAME:

		// string in file is zero-terminated, 
		// this should be no problem. However, validate whether
		// it overlaps the end of the chunk, if yes we should
		// truncate it.
		while (*sz++ != '\0')
		{
			if (sz > pcCurNext-1)break;
			++iCnt;
		}
		this->mScene->mMaterials.back().mName = std::string(
			(const char*)this->mCurrent,iCnt);
		break;
	case Dot3DSFile::CHUNK_MAT_DIFFUSE:
		pc = &this->mScene->mMaterials.back().mDiffuse;
		this->ParseColorChunk(pc);
		if (is_qnan(pc->r))
		{
			// color chunk is invalid. Simply ignore it
			pc->r = pc->g = pc->b = 1.0f;
		}
		break;
	case Dot3DSFile::CHUNK_MAT_SPECULAR:
		pc = &this->mScene->mMaterials.back().mSpecular;
		this->ParseColorChunk(pc);
		if (is_qnan(pc->r))
		{
			// color chunk is invalid. Simply ignore it
			pc->r = pc->g = pc->b = 1.0f;
		}
		break;
	case Dot3DSFile::CHUNK_MAT_AMBIENT:
		pc = &this->mScene->mMaterials.back().mAmbient;
		this->ParseColorChunk(pc);
		if (is_qnan(pc->r))
		{
			// color chunk is invalid. Simply ignore it
			pc->r = pc->g = pc->b = 1.0f;
		}
		break;
	case Dot3DSFile::CHUNK_MAT_SELF_ILLUM:
		pc = &this->mScene->mMaterials.back().mEmissive;
		this->ParseColorChunk(pc);
		if (is_qnan(pc->r))
		{
			// color chunk is invalid. Simply ignore it
			// EMISSSIVE TO 0|0|0
			pc->r = pc->g = pc->b = 0.0f;
		}
		break;
	case Dot3DSFile::CHUNK_MAT_TRANSPARENCY:
		pcf = &this->mScene->mMaterials.back().mTransparency;
		*pcf = this->ParsePercentageChunk();
		// NOTE: transparency, not opacity
		*pcf = 1.0f - *pcf;
		if (is_qnan(*pcf))
			*pcf = 0.0f;
		break;

	case Dot3DSFile::CHUNK_MAT_SHADING:
		
		this->mScene->mMaterials.back().mShading =
			(Dot3DS::Dot3DSFile::shadetype3ds)*((uint16_t*)this->mCurrent);

		this->mCurrent += sizeof(uint16_t);
		break;

	case Dot3DSFile::CHUNK_MAT_SHININESS:
		pcf = &this->mScene->mMaterials.back().mSpecularExponent;
		*pcf = this->ParsePercentageChunk();
		if (is_qnan(*pcf))
			*pcf = 0.0f;
		else *pcf *= (float)0xFFFF;
		break;

	case Dot3DSFile::CHUNK_MAT_SELF_ILPCT:
		pcf = &this->mScene->mMaterials.back().sTexEmissive.mTextureBlend;
		*pcf = this->ParsePercentageChunk();
		if (is_qnan(*pcf))
			*pcf = 1.0f;
		break;

	// parse texture chunks
	case Dot3DSFile::CHUNK_MAT_TEXTURE:
		iRemaining = (psChunk->Size - sizeof(Dot3DSFile::Chunk));
		this->ParseTextureChunk(&iRemaining,&this->mScene->mMaterials.back().sTexDiffuse);
		break;
	case Dot3DSFile::CHUNK_MAT_BUMPMAP:
		iRemaining = (psChunk->Size - sizeof(Dot3DSFile::Chunk));
		this->ParseTextureChunk(&iRemaining,&this->mScene->mMaterials.back().sTexBump);
		break;
	case Dot3DSFile::CHUNK_MAT_OPACMAP:
		iRemaining = (psChunk->Size - sizeof(Dot3DSFile::Chunk));
		this->ParseTextureChunk(&iRemaining,&this->mScene->mMaterials.back().sTexOpacity);
		break;
	case Dot3DSFile::CHUNK_MAT_MAT_SHINMAP:
		iRemaining = (psChunk->Size - sizeof(Dot3DSFile::Chunk));
		this->ParseTextureChunk(&iRemaining,&this->mScene->mMaterials.back().sTexShininess);
		break;
	case Dot3DSFile::CHUNK_MAT_SPECMAP:
		iRemaining = (psChunk->Size - sizeof(Dot3DSFile::Chunk));
		this->ParseTextureChunk(&iRemaining,&this->mScene->mMaterials.back().sTexSpecular);
		break;
	case Dot3DSFile::CHUNK_MAT_SELFIMAP:
		iRemaining = (psChunk->Size - sizeof(Dot3DSFile::Chunk));
		this->ParseTextureChunk(&iRemaining,&this->mScene->mMaterials.back().sTexEmissive);
		break;
	};
	if ((unsigned int)pcCurNext < (unsigned int)this->mCurrent)
	{
		// place an error message. If we crash the programmer
		// will be able to find it
		this->mErrorText = ASSIMP_3DS_WARN_CHUNK_OVERFLOW_MSG;
		pcCurNext = this->mCurrent;
	}
	// Go to the starting position of the next chunk on this level
	this->mCurrent = pcCurNext;

	*piRemaining -= psChunk->Size;
	if (0 >= *piRemaining)return;
	return ParseMaterialChunk(piRemaining);
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::ParseTextureChunk(int* piRemaining,Dot3DS::Texture* pcOut)
{
	const Dot3DSFile::Chunk* psChunk;

	this->ReadChunk(&psChunk);
	if (NULL == psChunk)return;

	const unsigned char* pcCur = this->mCurrent;
	const unsigned char* pcCurNext = pcCur + (psChunk->Size 
		- sizeof(Dot3DSFile::Chunk));

	// get chunk type
	const unsigned char* sz = this->mCurrent;
	unsigned int iCnt = 0;
	switch (psChunk->Flag)
	{
	case Dot3DSFile::CHUNK_MAPFILE:
		// string in file is zero-terminated, 
		// this should be no problem. However, validate whether
		// it overlaps the end of the chunk, if yes we should
		// truncate it.
		while (*sz++ != '\0')
		{
			if (sz > pcCurNext-1)break;
			++iCnt;
		}
		pcOut->mMapName = std::string((const char*)this->mCurrent,iCnt);
		break;
	// manually parse the blend factor
	case Dot3DSFile::CHUNK_PERCENTF:
		pcOut->mTextureBlend = *((float*)this->mCurrent);
		break;
	// manually parse the blend factor
	case Dot3DSFile::CHUNK_PERCENTW:
		pcOut->mTextureBlend = (float)(*((short*)this->mCurrent)) / (float)100;
		break;

	case Dot3DSFile::CHUNK_MAT_MAP_USCALE:
		pcOut->mScaleU = *((float*)this->mCurrent);
		break;
	case Dot3DSFile::CHUNK_MAT_MAP_VSCALE:
		pcOut->mScaleV = *((float*)this->mCurrent);
		break;
	case Dot3DSFile::CHUNK_MAT_MAP_UOFFSET:
		pcOut->mOffsetU = *((float*)this->mCurrent);
		break;
	case Dot3DSFile::CHUNK_MAT_MAP_VOFFSET:
		pcOut->mOffsetV = *((float*)this->mCurrent);
		break;
	case Dot3DSFile::CHUNK_MAT_MAP_ANG:
		pcOut->mRotation = *((float*)this->mCurrent);
		break;
	};

	// Go to the starting position of the next chunk on this level
	this->mCurrent = pcCurNext;

	*piRemaining -= psChunk->Size;
	if (0 >= *piRemaining)return;
	return ParseTextureChunk(piRemaining,pcOut);
}
// ------------------------------------------------------------------------------------------------
float Dot3DSImporter::ParsePercentageChunk()
{
	const Dot3DSFile::Chunk* psChunk;
	this->ReadChunk(&psChunk);
	if (NULL == psChunk)return std::numeric_limits<float>::quiet_NaN();

	if (Dot3DSFile::CHUNK_PERCENTF == psChunk->Flag)
	{
		if (sizeof(float) > psChunk->Size)
			return std::numeric_limits<float>::quiet_NaN();
		return *((float*)this->mCurrent);
	}
	else if (Dot3DSFile::CHUNK_PERCENTW == psChunk->Flag)
	{
		if (2 > psChunk->Size)
			return std::numeric_limits<float>::quiet_NaN();
		return (float)(*((short*)this->mCurrent)) / (float)0xFFFF;
	}
	this->mCurrent += psChunk->Size - sizeof(Dot3DSFile::Chunk);
	return std::numeric_limits<float>::quiet_NaN();
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::ParseColorChunk(aiColor3D* p_pcOut,
	bool p_bAcceptPercent)
{
	ai_assert(p_pcOut != NULL);

	// error return value
	static const aiColor3D clrError = aiColor3D(std::numeric_limits<float>::quiet_NaN(),
		std::numeric_limits<float>::quiet_NaN(),
		std::numeric_limits<float>::quiet_NaN());

	const Dot3DSFile::Chunk* psChunk;
	this->ReadChunk(&psChunk);
	if (NULL == psChunk)
	{
		*p_pcOut = clrError;
		return;
	}
	const unsigned char* pcCur = this->mCurrent;
	this->mCurrent += psChunk->Size - sizeof(Dot3DSFile::Chunk);
	bool bGamma = false;
	switch(psChunk->Flag)
	{
	case Dot3DSFile::CHUNK_LINRGBF:
		bGamma = true;
	case Dot3DSFile::CHUNK_RGBF:
		if (sizeof(float) * 3 > psChunk->Size - sizeof(Dot3DSFile::Chunk))
		{
			*p_pcOut = clrError;
			return;
		}
		p_pcOut->r = ((float*)pcCur)[0];
		p_pcOut->g = ((float*)pcCur)[1];
		p_pcOut->b = ((float*)pcCur)[2];
		break;

	case Dot3DSFile::CHUNK_LINRGBB:
		bGamma = true;
	case Dot3DSFile::CHUNK_RGBB:
		if (sizeof(char) * 3 > psChunk->Size - sizeof(Dot3DSFile::Chunk))
		{
			*p_pcOut = clrError;
			return;
		}
		p_pcOut->r = (float)pcCur[0] / 255.0f;
		p_pcOut->g = (float)pcCur[1] / 255.0f;
		p_pcOut->b = (float)pcCur[2] / 255.0f;
		break;

	// percentage chunks: accepted to be compatible with various
	// .3ds files with very curious content
	case Dot3DSFile::CHUNK_PERCENTF:
		if (p_bAcceptPercent && 4 <= psChunk->Size - sizeof(Dot3DSFile::Chunk))
		{
			p_pcOut->r = *((float*)pcCur);
			p_pcOut->g = *((float*)pcCur);
			p_pcOut->b = *((float*)pcCur);
			break;
		}
		*p_pcOut = clrError;
		return;
	case Dot3DSFile::CHUNK_PERCENTW:
		if (p_bAcceptPercent && 1 <= psChunk->Size - sizeof(Dot3DSFile::Chunk))
		{
			p_pcOut->r = (float)pcCur[0] / 255.0f;
			p_pcOut->g = (float)pcCur[0] / 255.0f;
			p_pcOut->b = (float)pcCur[0] / 255.0f;
			break;
		}
		*p_pcOut = clrError;
		return;

	default:
		// skip unknown chunks, hope this won't cause any problems.
		return this->ParseColorChunk(p_pcOut,p_bAcceptPercent);
	};
	// assume input gamma = 1.0, output gamma = 2.2 
	// Not sure whether this is correct, too tired to 
	// think about it ;-)
	if (bGamma)
	{
		p_pcOut->r = powf(p_pcOut->r, 1.0f / 2.2f);
		p_pcOut->g = powf(p_pcOut->g, 1.0f / 2.2f);
		p_pcOut->b = powf(p_pcOut->b, 1.0f / 2.2f);
	}
	return;
}
