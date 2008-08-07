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

/** @file Implementation of the LWO importer class */

// internal headers
#include "LWOLoader.h"
#include "MaterialSystem.h"
#include "ByteSwap.h"

// public assimp headers
#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"
#include "../include/aiAssert.h"
#include "../include/DefaultLogger.h"

// boost headers
#include <boost/scoped_ptr.hpp>

using namespace Assimp;


// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
LWOImporter::LWOImporter()
{
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
LWOImporter::~LWOImporter()
{
	mFaces.clear();
	mTempPoints.clear();
	mSurfaces.clear();
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool LWOImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
{
	// simple check of file extension is enough for the moment
	std::string::size_type pos = pFile.find_last_of('.');
	// no file extension - can't read
	if( pos == std::string::npos)return false;
	std::string extension = pFile.substr( pos);

	if (extension.length() < 4)return false;
	if (extension[0] != '.')return false;

	if (extension[1] != 'l' && extension[1] != 'L')return false;
	if (extension[2] != 'w' && extension[2] != 'W')return false;
	if (extension[3] != 'o' && extension[3] != 'O')return false;

	return true;
}
// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void LWOImporter::InternReadFile( const std::string& pFile, 
	aiScene* pScene, 
	IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile, "rb"));

	// Check whether we can read from the file
	if( file.get() == NULL)
		throw new ImportErrorException( "Failed to open LWO file " + pFile + ".");

	if((this->fileSize = (unsigned int)file->FileSize()) < 12)
		throw new ImportErrorException("LWO: The file is too small to contain the IFF header");

	// allocate storage and copy the contents of the file to a memory buffer
	std::vector< uint8_t > mBuffer(fileSize);
	file->Read( &mBuffer[0], 1, fileSize);
	this->pScene = pScene;

	// determine the type of the file
	uint32_t fileType;
	const char* sz = IFF::ReadHeader(&mBuffer[0],fileType);
	if (sz)throw new ImportErrorException(sz);

	mFileBuffer = &mBuffer[0] + 12;
	fileSize -= 12;

	try 
	{
		// old lightwave file format (prior to v6)
		if (AI_LWO_FOURCC_LWOB == fileType)
			this->LoadLWOBFile();
		// new lightwave format
		else if (AI_LWO_FOURCC_LWO2 == fileType)
			this->LoadLWO2File();
		// we don't know this format
		else 
		{
			char szBuff[5];
			szBuff[0] = (char)(fileType >> 24u);
			szBuff[1] = (char)(fileType >> 16u);
			szBuff[2] = (char)(fileType >> 8u);
			szBuff[3] = (char)(fileType);
			throw new ImportErrorException(std::string("Unknown LWO sub format: ") + szBuff);
		}

		// generate a default surface if necessary
		if (mSurfaces.empty())
			mSurfaces.push_back(LWO::Surface());

		// now sort all faces by the surfaces assigned to them
		typedef std::vector<unsigned int> SortedRep;
		std::vector<SortedRep> pSorted(mSurfaces.size());

		unsigned int i = 0;
		for (FaceList::iterator it = mFaces.begin(), end = mFaces.end();
			it != end;++it,++i)
		{
			if ((*it).surfaceIndex >= mSurfaces.size())
			{
				DefaultLogger::get()->warn("LWO: Invalid face surface index");
				(*it).surfaceIndex = mSurfaces.size()-1;
			}
			pSorted[(*it).surfaceIndex].push_back(i);
		}

		// now generate output meshes
		for (unsigned int p = 0; p < mSurfaces.size();++p)
			if (!pSorted[p].empty())pScene->mNumMeshes++;

		if (!(pScene->mNumMaterials = pScene->mNumMeshes))
			throw new ImportErrorException("LWO: There are no meshes");

		pScene->mMeshes = new aiMesh*[pScene->mNumMeshes];
		pScene->mMaterials = new aiMaterial*[pScene->mNumMaterials];
		for (unsigned int p = 0,i = 0;i < mSurfaces.size();++i)
		{
			SortedRep& sorted = pSorted[i];
			if (sorted.empty())continue;

			// generate the mesh 
			aiMesh* mesh = pScene->mMeshes[p] = new aiMesh();
			mesh->mNumFaces = sorted.size();

			for (SortedRep::const_iterator it = sorted.begin(), end = sorted.end();
				it != end;++it)
			{
				mesh->mNumVertices += mFaces[*it].mNumIndices;
			}

			mesh->mVertices = new aiVector3D[mesh->mNumVertices];
			mesh->mFaces = new aiFace[mesh->mNumFaces];
			mesh->mMaterialIndex = p;

			// now convert all faces
			unsigned int vert = 0;
			for (SortedRep::const_iterator it = sorted.begin(), end = sorted.end();
				it != end;++it)
			{
				LWO::Face& face = mFaces[*it];

				// copy all vertices
				for (unsigned int q = 0; q  < face.mNumIndices;++q)
				{
					*mesh->mVertices++ = mTempPoints[face.mIndices[q]];
					face.mIndices[q] = vert++;
				}

				mesh->mFaces->mIndices = face.mIndices;
				mesh->mFaces->mNumIndices = face.mNumIndices;
				face.mIndices = NULL; // make sure it won't be deleted

				mesh->mFaces++;
			}
			mesh->mFaces -= mesh->mNumFaces;
			mesh->mVertices -= mesh->mNumVertices;

			// generate the corresponding material
			MaterialHelper* pcMat = new MaterialHelper();
			pScene->mMaterials[p] = pcMat;

			// todo

			++p;
		}
	}
	// make sure the arrays are cleaned up ...
	catch (ImportErrorException* ex)
	{
		this->~LWOImporter();
		throw ex;
	}
	this->~LWOImporter();
}
// ------------------------------------------------------------------------------------------------
void LWOImporter::CountVertsAndFaces(unsigned int& verts, unsigned int& faces,
	LE_NCONST uint8_t*& cursor, const uint8_t* const end, unsigned int max)
{
	while (cursor < end && max--)
	{
		uint16_t numIndices = *((uint16_t*)cursor);cursor+=2;
		verts += numIndices;faces++;
		cursor += numIndices*2;
		int16_t surface = *((uint16_t*)cursor);cursor+=2;
		if (surface < 0)
		{
			// there are detail polygons
			numIndices = *((uint16_t*)cursor);cursor+=2;
			CountVertsAndFaces(verts,faces,cursor,end,numIndices);
		}
	}
}
// ------------------------------------------------------------------------------------------------
void LWOImporter::CopyFaceIndices(LWOImporter::FaceList::iterator& it,LE_NCONST uint8_t*& cursor, 
	const uint8_t* const end, unsigned int max)
{
	while (cursor < end && max--)
	{
		LWO::Face& face = *it;++it;
		face.mNumIndices = *((uint16_t*)cursor);
		if (cursor + face.mNumIndices*2 + 4 >= end)break;
		face.mIndices = new unsigned int[face.mNumIndices];
		for (unsigned int i = 0; i < face.mNumIndices;++i)
		{
			face.mIndices[i] = *((uint16_t*)(cursor+=2));
			if (face.mIndices[i] >= mTempPoints.size())
			{
				face.mIndices[i] = mTempPoints.size()-1;
				DefaultLogger::get()->warn("LWO: Face index is out of range");
			}
		}
		cursor+=2;
		int16_t surface = *((uint16_t*)cursor);cursor+=2;
		if (surface < 0)
		{
			surface *= -1;

			// there are detail polygons
			uint16_t numPolygons = *((uint16_t*)cursor);cursor+=2;
			if (cursor < end)CopyFaceIndices(it,cursor,end,numPolygons);
		}
		face.surfaceIndex = surface-1;
	}
}
// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWOBSurface(unsigned int size)
{
	uint32_t iCursor = 0;
	mSurfaces.push_back( LWO::Surface () );
	LWO::Surface& surf = mSurfaces.back();
	LWO::Texture* pTex = NULL;

	// at first we'll need to read the name of the surface
	const uint8_t* sz = mFileBuffer;
	while (*mFileBuffer)
	{
		if (++iCursor > size)throw new ImportErrorException("LWOB: Invalid file, surface name is too long");
		++mFileBuffer;
	}
	unsigned int len = unsigned int (mFileBuffer-sz);
	surf.mName = std::string((const char*)sz,len);
	mFileBuffer += 1-(len & 1); // skip one byte if the length of the string is odd
	while (true)
	{
		if ((iCursor += sizeof(IFF::ChunkHeader)) > size)break;
		LE_NCONST IFF::ChunkHeader* head = (LE_NCONST IFF::ChunkHeader*)mFileBuffer;
		AI_LSWAP4(head->length);
		AI_LSWAP4(head->type);
		if ((iCursor += head->length) > size)
		{
			throw new ImportErrorException("LWOB: Invalid file, the size attribute of "
				"a surface sub chunk points behind the end of the file");
		}
		mFileBuffer += sizeof(IFF::ChunkHeader);
		switch (head->type)
		{
			// diffuse color
		case AI_LWO_COLR:
			{
				if (head->length < 3)
				{
					DefaultLogger::get()->warn("LWO: COLR chunk is expected to be at least 3 bytes in size");
					break;
				}
				surf.mColor.r = *mFileBuffer++ / 255.0f;
				surf.mColor.g = *mFileBuffer++ / 255.0f;
				surf.mColor.b = *mFileBuffer   / 255.0f;
				break;
			}
			// diffuse strength ... hopefully
		case AI_LWO_DIFF:
			{
				AI_LSWAP2(mFileBuffer);
				surf.mDiffuseValue = *((int16_t*)mFileBuffer) / 255.0f;
				break;
			}
			// specular strength ... hopefully
		case AI_LWO_SPEC:
			{
				AI_LSWAP2(mFileBuffer);
				surf.mSpecularValue = *((int16_t*)mFileBuffer) / 255.0f;
				break;
			}
		// transparency
		case AI_LWO_TRAN:
			{
				AI_LSWAP2(mFileBuffer);
				surf.mTransparency = *((int16_t*)mFileBuffer) / 255.0f;
				break;
			}
		// glossiness
		case AI_LWO_GLOS:
			{
				AI_LSWAP2(mFileBuffer);
				surf.mGlossiness = float(*((int16_t*)mFileBuffer));
				break;
			}
		// color texture
		case AI_LWO_CTEX:
			{
				pTex = &surf.mColorTexture;
				break;
			}
		// diffuse texture
		case AI_LWO_DTEX:
			{
				pTex = &surf.mDiffuseTexture;
				break;
			}
		// specular texture
		case AI_LWO_STEX:
			{
				pTex = &surf.mSpecularTexture;
				break;
			}
		// bump texture
		case AI_LWO_BTEX:
			{
				pTex = &surf.mBumpTexture;
				break;
			}
		// transparency texture
		case AI_LWO_TTEX:
			{
				pTex = &surf.mTransparencyTexture;
				break;
			}
			// texture path
		case AI_LWO_TIMG:
			{
				if (pTex)
				{
					unsigned int iCursor = 0;
					while (*mFileBuffer)
					{
						if (++iCursor > head->length)
						{
							DefaultLogger::get()->warn("LWOB: Invalid file, texture name (TIMG) is too long");
							break;
						}
						++mFileBuffer;
					}
					unsigned int len = unsigned int (mFileBuffer-sz);
					pTex->mFileName = std::string((const char*)sz,len);
				}
				else DefaultLogger::get()->warn("LWOB: TIMG tag was encuntered although "
					"there was no xTEX tag before");
				break;
			}
		// texture strength
		case AI_LWO_TVAL:
			{
				if (pTex)pTex->mStrength = *mFileBuffer / 255.0f;
				else DefaultLogger::get()->warn("LWOB: TVAL tag was encuntered "
					"although there was no xTEX tag before");
				break;
			}
		}
		mFileBuffer += head->length;
	}
}
// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWOBFile()
{
	uint32_t iCursor = 0;
	while (true)
	{
		if ((iCursor += sizeof(IFF::ChunkHeader)) > this->fileSize)break;
		LE_NCONST IFF::ChunkHeader* head = (LE_NCONST IFF::ChunkHeader*)mFileBuffer;
		AI_LSWAP4(head->length);
		AI_LSWAP4(head->type);
		if ((iCursor += head->length) > this->fileSize)
		{
			//throw new ImportErrorException("LWOB: Invalid file, the size attribute of "
			//	"a chunk points behind the end of the file");
			break;
		}
		mFileBuffer += sizeof(IFF::ChunkHeader);
		switch (head->type)
		{
			// vertex list
		case AI_LWO_PNTS:
			{
				mTempPoints.resize( head->length / 12 );
#ifndef AI_BUILD_BIG_ENDIAN
				for (unsigned int i = 0; i < head->length>>2;++i)
					ByteSwap::Swap4( mFileBuffer + (i << 2));
#endif
				::memcpy(&mTempPoints[0],mFileBuffer,head->length);
				break;
			}
			// face list
		case AI_LWO_POLS:
			{
				// first find out how many faces and vertices we'll finally need
				const uint8_t* const end = mFileBuffer + head->length;
				LE_NCONST uint8_t* cursor = mFileBuffer;

#ifndef AI_BUILD_BIG_ENDIAN
				while (cursor < end)ByteSwap::Swap2(cursor++);
				cursor = mFileBuffer;
#endif

				unsigned int iNumFaces = 0,iNumVertices = 0;
				CountVertsAndFaces(iNumVertices,iNumFaces,cursor,end);

				// allocate the output array and copy face indices
				if (iNumFaces)
				{
					this->mTempPoints.resize(iNumVertices);
					this->mFaces.resize(iNumFaces);
					FaceList::iterator it = this->mFaces.begin();
					CopyFaceIndices(it,mFileBuffer,end);
				}
				break;
			}
			// surface chunk
		case AI_LWO_SURF:
			{
				LoadLWOBSurface(head->length);
				break;
			}
		}
		mFileBuffer += head->length;
	}
}
// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2File()
{
}