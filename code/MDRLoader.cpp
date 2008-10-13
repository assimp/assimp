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

/** @file Implementation of the MDR importer class */

#include "AssimpPCH.h"
#include "MDRLoader.h"

using namespace Assimp;
using namespace Assimp::MDR;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
MDRImporter::MDRImporter()
{
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
MDRImporter::~MDRImporter()
{
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool MDRImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
{
	// simple check of file extension is enough for the moment
	std::string::size_type pos = pFile.find_last_of('.');
	// no file extension - can't read
	if( pos == std::string::npos)
		return false;
	std::string extension = pFile.substr( pos);

	return !(extension.length() != 4 || extension[0] != '.' ||
			 extension[1] != 'm' && extension[1] != 'M' ||
			 extension[2] != 'd' && extension[2] != 'D' ||
			 extension[3] != 'r' && extension[3] != 'R');
}

// ------------------------------------------------------------------------------------------------
// Validate the header of the given MDR file
void MDRImporter::ValidateHeader()
{
	AI_SWAP4(pcLOD->version);
	AI_SWAP4(pcLOD->numBones);
	AI_SWAP4(pcLOD->numTags);
	AI_SWAP4(pcLOD->numFrames);
	AI_SWAP4(pcLOD->ofsFrames);
	AI_SWAP4(pcLOD->ofsTags);
	AI_SWAP4(pcLOD->numLODs);
	AI_SWAP4(pcLOD->ofsLODs);

	if (pcHeader->ident != AI_MDR_MAGIC_NUMBER_BE &&
		pcHeader->ident != AI_MDR_MAGIC_NUMBER_LE)
	{
		char szBuffer[5];
		szBuffer[0] = ((char*)&pcHeader->ident)[0];
		szBuffer[1] = ((char*)&pcHeader->ident)[1];
		szBuffer[2] = ((char*)&pcHeader->ident)[2];
		szBuffer[3] = ((char*)&pcHeader->ident)[3];
		szBuffer[4] = '\0';

		throw new ImportErrorException("Invalid MDR magic word: should be 5MDR, the "
			"magic word found is " + std::string( szBuffer ));
	}

	if (pcHeader->version != AI_MDR_VERSION)
		DefaultLogger::get()->warn("Unsupported MDR file version (2 (AI_MDR_VERSION) was expected)");

	if (!pcHeader->numBones)
		DefaultLogger::get()->warn("MDR: At least one bone must be there");

	// validate all LODs
	if (pcHeader->ofsLODs > fileSize) 

	// validate all frames
	if (pcHeader->ofsFrames + sizeof(MDR::Frame) * (pcHeader->numBones-1) *
		sizeof(MDR::Bone) * pcHeader->numFrames > fileSize)
	{
		throw new ImportErrorException("MDR: header is invalid - frame out of range");
	}

	// check whether the requested frame is existing
	if (this->configFrameID >= pcHeader->numFrames)
		throw new ImportErrorException("The requested frame is not available");
}

// ------------------------------------------------------------------------------------------------
// Validate the header of a given MDR file LOD
void MDRImporter::ValidateLODHeader(BE_NCONST MDR::LOD* pcLOD)
{
	AI_SWAP4(pcLOD->ofsSurfaces);
	AI_SWAP4(pcLOD->numSurfaces);
	AI_SWAP4(pcLOD->ofsEnd);

    const unsigned int iMax = this->fileSize - (unsigned int)((int8_t*)pcLOD-(int8_t*)pcHeader);

	if (!pcLOD->numSurfaces)
		throw new ImportErrorException("MDR: LOD has zero surfaces assigned");
}

// ------------------------------------------------------------------------------------------------
// Validate the header of a given MDR file surface
void MDRImporter::ValidateSurfaceHeader(BE_NCONST MDR::Surface* pcSurf)
{
	AI_SWAP4(pcSurf->ident);
	AI_SWAP4(pcSurf->numBoneReferences);
	AI_SWAP4(pcSurf->numTriangles);
	AI_SWAP4(pcSurf->numVerts);
	AI_SWAP4(pcSurf->ofsBoneReferences);
	AI_SWAP4(pcSurf->ofsEnd);
	AI_SWAP4(pcSurf->ofsTriangles);
	AI_SWAP4(pcSurf->ofsVerts);
	AI_SWAP4(pcSurf->shaderIndex);

    const unsigned int iMax = this->fileSize - (unsigned int)((int8_t*)pcSurf-(int8_t*)pcHeader);

	// not exact - there could be extra data in the vertices.
	if (pcSurf->ofsTriangles + pcSurf->numTriangles*sizeof(MDR::Triangle) > iMax ||
		pcSurf->ofsVerts + pcSurf->numVerts*sizeof(MDR::Vertex) > iMax)
    {
		throw new ImportErrorException("MDR: Surface header is invalid");
    }
}

// ------------------------------------------------------------------------------------------------
// Setup configuration properties
void MDRImporter::SetupProperties(const Importer* pImp)
{
	// The AI_CONFIG_IMPORT_MDR_KEYFRAME option overrides the
	//     AI_CONFIG_IMPORT_GLOBAL_KEYFRAME option.
	if(0xffffffff == (this->configFrameID = pImp->GetPropertyInteger(
		AI_CONFIG_IMPORT_MDR_KEYFRAME,0xffffffff)))
	{
		this->configFrameID = pImp->GetPropertyInteger(AI_CONFIG_IMPORT_GLOBAL_KEYFRAME,0);
	}
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void MDRImporter::InternReadFile( const std::string& pFile, 
	aiScene* pScene, IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile));

	// Check whether we can read from the file
	if( file.get() == NULL)
		throw new ImportErrorException( "Failed to open MDR file " + pFile + ".");

	// check whether the mdr file is large enough to contain the file header
	fileSize = (unsigned int)file->FileSize();
	if( fileSize < sizeof(MDR::Header))
		throw new ImportErrorException( "MDR File is too small.");

	std::vector<unsigned char> mBuffer2(fileSize);
	file->Read( &mBuffer2[0], 1, fileSize);
	mBuffer = &mBuffer2[0];

	// validate the file header and do BigEndian byte swapping for all sub headers
	this->pcHeader = (BE_NCONST MDR::Header*)this->mBuffer;
	this->ValidateHeader();

	// go to the first LOD
	LE_NCONST MDR::LOD* lod = (LE_NCONST MDR::LOD*)((uint8_t*)this->pcHeader+pcHeader->ofsLODs);
	std::vector<aiMesh*> outMeshes;
	outMeshes.reserve(lod->numSurfaces);

	// get a pointer to the first surface
	LE_NCONST MDR::Surface* surf = (LE_NCONST MDR::Surface*)((uint8_t*)lod+lod->ofsSurfaces);
	for (uint32_t i = 0; i < lod->numSurfaces; ++i)
	{
		if (surf->numTriangles && surf->numVerts && surf->numBoneReferences)
		{
			outMeshes.push_back(new aiMesh());
			aiMesh* mesh = outMeshes.back();

			mesh->mNumFaces = surf->numTriangles;
			mesh->mNumVertices = mesh->mNumFaces*3;
			mesh->mNumBones = surf->numBoneReferences;

			mesh->mFaces = new aiFace[mesh->mNumFaces];
			mesh->mVertices = new aiVector3D[mesh->mNumVertices];
			mesh->mTextureCoords[0] = new aiVector3D[mesh->mNumVertices];
			mesh->mBones = new aiBone*[mesh->mNumBones];

			// allocate output bones
			for (unsigned int p = 0; p < mesh->mNumBones;++p)
			{
				aiBone* bone = mesh->mBones[p] = new aiBone();
				bone->mName.length = ::sprintf(  bone->mName.data, "B_%i",p);
			}

			std::vector<BoneWeightInfo> mWeights;
			mWeights.reserve(surf->numVerts << 1);

			std::vector<VertexInfo> mVertices(surf->numVerts);

			// get a pointer to the first vertex
			LE_NCONST MDR::Vertex* v = (LE_NCONST MDR::Vertex*)((uint8_t*)surf+surf->ofsVerts);
			for (unsigned int m = 0; m < surf->numVerts; ++m)
			{
				// get a pointer to the next vertex
				v = (LE_NCONST MDR::Vertex*)((uint8_t*)(v+1) + v->numWeights*sizeof(MDR::Weight));

				AI_SWAP4(v->numWeights);
				AI_SWAP4(v->normal.x);AI_SWAP4(v->normal.y);AI_SWAP4(v->normal.z);
				AI_SWAP4(v->texCoords.x);AI_SWAP4(v->texCoords.y);

				VertexInfo& vert = mVertices[m];
				vert.uv.x = v->texCoords.x;  vert.uv.y = v->texCoords.y; 
				vert.normal = v->normal;
				vert.start  = (unsigned int)mWeights.size();
				vert.num    = v->numWeights;

				for (unsigned int l = 0; l < vert.num; ++l)
				{
				}
			}

			// find out how large the output weight buffers must be
			LE_NCONST MDR::Triangle* tri = (LE_NCONST MDR::Triangle*)((uint8_t*)surf+surf->ofsTriangles);
			LE_NCONST MDR::Triangle* const triEnd = tri + surf->numTriangles;
			for (; tri != triEnd; ++tri)
			{
				for (unsigned int o = 0; o < 3;++o)
				{
					AI_SWAP4(tri->indexes[o]);
					register unsigned int temp = tri->indexes[o];
					if (temp >= surf->numVerts)
						throw new ImportErrorException("MDR: Vertex index is out of range");

					VertexInfo& vert = mVertices[temp];
					for (unsigned int l = vert.start; l < vert.start + vert.num; ++l)
					{
						if (mWeights[l].first >= surf->numBoneReferences)
							throw new ImportErrorException("MDR: Bone index is out of range");

						++mesh->mBones[mWeights[l].first]->mNumWeights;
					}
				}
			}

			// allocate storage for output bone weights
			for (unsigned int p = 0; p < mesh->mNumBones;++p)
			{
				aiBone* bone = mesh->mBones[p];
				ai_assert(0 != bone->mNumWeights);
				bone->mWeights = new aiVertexWeight[bone->mNumWeights];
			}

			// and build the final output buffers
		}

		// get a pointer to the next surface
		surf = (LE_NCONST MDR::Surface*)((uint8_t*)surf + surf->ofsEnd);	
	}

	pScene->mNumMeshes = (unsigned int) outMeshes.size();
	pScene->mMeshes = new aiMesh*[pScene->mNumMeshes];
	::memcpy(pScene->mMeshes,&outMeshes[0],sizeof(void*)*pScene->mNumMeshes);
}
