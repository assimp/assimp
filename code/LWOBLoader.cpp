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

/** @file Implementation of the LWO importer class for the older LWOB 
    file formats, including materials */


#include "AssimpPCH.h"

// Internal headers
#include "LWOLoader.h"
using namespace Assimp;


// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWOBFile()
{
	LE_NCONST uint8_t* const end = mFileBuffer + fileSize;
	while (true)
	{
		if (mFileBuffer + sizeof(IFF::ChunkHeader) > end)break;
		LE_NCONST IFF::ChunkHeader* const head = IFF::LoadChunk(mFileBuffer);

		if (mFileBuffer + head->length > end)
		{
			throw new ImportErrorException("LWOB: Invalid chunk length");
			break;
		}
		uint8_t* const next = mFileBuffer+head->length;
		switch (head->type)
		{
			// vertex list
		case AI_LWO_PNTS:
			{
				if (!mCurLayer->mTempPoints.empty())
					DefaultLogger::get()->warn("LWO: PNTS chunk encountered twice");
				else LoadLWOPoints(head->length);
				break;
			}
			// face list
		case AI_LWO_POLS:
			{
				if (!mCurLayer->mFaces.empty())
					DefaultLogger::get()->warn("LWO: POLS chunk encountered twice");
				else LoadLWOBPolygons(head->length);
				break;
			}
			// list of tags
		case AI_LWO_SRFS:
			{
				if (!mTags->empty())
					DefaultLogger::get()->warn("LWO: SRFS chunk encountered twice");
				else LoadLWOTags(head->length);
				break;
			}

			// surface chunk
		case AI_LWO_SURF:
			{
				LoadLWOBSurface(head->length);
				break;
			}
		}
		mFileBuffer = next;
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWOBPolygons(unsigned int length)
{
	// first find out how many faces and vertices we'll finally need
	LE_NCONST uint16_t* const end	= (LE_NCONST uint16_t*)(mFileBuffer+length);
	LE_NCONST uint16_t* cursor		= (LE_NCONST uint16_t*)mFileBuffer;

	// perform endianess conversions
#ifndef AI_BUILD_BIG_ENDIAN
	while (cursor < end)ByteSwap::Swap2(cursor++);
	cursor = (LE_NCONST uint16_t*)mFileBuffer;
#endif

	unsigned int iNumFaces = 0,iNumVertices = 0;
	CountVertsAndFacesLWOB(iNumVertices,iNumFaces,cursor,end);

	// allocate the output array and copy face indices
	if (iNumFaces)
	{
		cursor = (LE_NCONST uint16_t*)mFileBuffer;

		mCurLayer->mFaces.resize(iNumFaces);
		FaceList::iterator it = mCurLayer->mFaces.begin();
		CopyFaceIndicesLWOB(it,cursor,end);
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::CountVertsAndFacesLWOB(unsigned int& verts, unsigned int& faces,
	LE_NCONST uint16_t*& cursor, const uint16_t* const end, unsigned int max)
{
	while (cursor < end && max--)
	{
		uint16_t numIndices = *cursor++;
		verts += numIndices;faces++;
		cursor += numIndices;
		int16_t surface = *cursor++;
		if (surface < 0)
		{
			// there are detail polygons
			numIndices = *cursor++;
			CountVertsAndFacesLWOB(verts,faces,cursor,end,numIndices);
		}
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::CopyFaceIndicesLWOB(FaceList::iterator& it,
	LE_NCONST uint16_t*& cursor, 
	const uint16_t* const end,
	unsigned int max)
{
	while (cursor < end && max--)
	{
		LWO::Face& face = *it;++it;
		if((face.mNumIndices = *cursor++))
		{
			if (cursor + face.mNumIndices >= end)break;
			face.mIndices = new unsigned int[face.mNumIndices];
			for (unsigned int i = 0; i < face.mNumIndices;++i)
			{
				unsigned int & mi = face.mIndices[i] = *cursor++;
				if (mi > mCurLayer->mTempPoints.size())
				{
					DefaultLogger::get()->warn("LWOB: face index is out of range");
					mi = (unsigned int)mCurLayer->mTempPoints.size()-1;
				}
			}
		}
		else DefaultLogger::get()->warn("LWOB: Face has 0 indices");
		int16_t surface = *cursor++;
		if (surface < 0)
		{
			surface = -surface;

			// there are detail polygons
			uint16_t numPolygons = *cursor++;
			if (cursor < end)CopyFaceIndicesLWOB(it,cursor,end,numPolygons);
		}
		face.surfaceIndex = surface-1;
	}
}

// ------------------------------------------------------------------------------------------------
LWO::Texture* LWOImporter::SetupNewTextureLWOB(LWO::TextureList& list,unsigned int size)
{
	list.push_back(LWO::Texture());
	LWO::Texture* tex = &list.back();

	std::string type;
	GetS0(type,size);

	return tex;
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWOBSurface(unsigned int size)
{
	LE_NCONST uint8_t* const end = mFileBuffer + size;

	mSurfaces->push_back( LWO::Surface () );
	LWO::Surface& surf = mSurfaces->back();
	LWO::Texture* pTex = NULL;

	GetS0(surf.mName,size);
	while (true)
	{
		if (mFileBuffer + 6 > end)break;

		LE_NCONST IFF::SubChunkHeader* const head = IFF::LoadSubChunk(mFileBuffer);

		if (mFileBuffer + head->length > end)
			throw new ImportErrorException("LWOB: Invalid surface chunk length");

		uint8_t* const next = mFileBuffer+head->length;
		switch (head->type)
		{
		// diffuse color
		case AI_LWO_COLR:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,COLR,3);
				surf.mColor.r = GetU1() / 255.0f;
				surf.mColor.g = GetU1() / 255.0f;
				surf.mColor.b = GetU1() / 255.0f;
				break;
			}
		// diffuse strength ...
		case AI_LWO_DIFF:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,DIFF,2);
				surf.mDiffuseValue = GetU2() / 255.0f;
				break;
			}
		// specular strength ... 
		case AI_LWO_SPEC:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,SPEC,2);
				surf.mSpecularValue = GetU2() / 255.0f;
				break;
			}
		// luminosity ... 
		case AI_LWO_LUMI:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,LUMI,2);
				surf.mLuminosity = GetU2() / 255.0f;
				break;
			}
		// transparency
		case AI_LWO_TRAN:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,TRAN,2);
				surf.mTransparency = GetU2() / 255.0f;
				break;
			}
		// surface flags
		case AI_LWO_FLAG:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,FLAG,2);
				uint16_t flag = GetU2();
				if (flag & 0x4 )   surf.mMaximumSmoothAngle = 1.56207f;
				if (flag & 0x8 )   surf.mColorHighlights = 1.f;
				if (flag & 0x100)  surf.bDoubleSided = true;
				break;
			}
		// maximum smoothing angle
		case AI_LWO_SMAN:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,SMAN,4);
				surf.mMaximumSmoothAngle = GetF4();
				break;
			}
		// glossiness
		case AI_LWO_GLOS:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,GLOS,2);
				surf.mGlossiness = (float)GetU2();
				break;
			}
		// color texture
		case AI_LWO_CTEX:
			{
				pTex = SetupNewTextureLWOB(surf.mColorTextures,
					head->length);
				break;
			}
		// diffuse texture
		case AI_LWO_DTEX:
			{
				pTex = SetupNewTextureLWOB(surf.mDiffuseTextures,
					head->length);
				break;
			}
		// specular texture
		case AI_LWO_STEX:
			{
				pTex = SetupNewTextureLWOB(surf.mSpecularTextures,
					head->length);
				break;
			}
		// bump texture
		case AI_LWO_BTEX:
			{
				pTex = SetupNewTextureLWOB(surf.mBumpTextures,
					head->length);
				break;
			}
		// transparency texture
		case AI_LWO_TTEX:
			{
				pTex = SetupNewTextureLWOB(surf.mOpacityTextures,
					head->length);
				break;
			}
		// texture path
		case AI_LWO_TIMG:
			{
				if (pTex)
				{
					GetS0(pTex->mFileName,head->length);	
				}
				else DefaultLogger::get()->warn("LWOB: TIMG tag was encuntered although "
					"there was no xTEX tag before");
				break;
			}
		// texture strength
		case AI_LWO_TVAL:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,TVAL,1);
				if (pTex)pTex->mStrength = (float)GetU1()/ 255.f;
				else DefaultLogger::get()->warn("LWOB: TVAL tag was encuntered "
					"although there was no xTEX tag before");
				break;
			}
		}
		mFileBuffer = next;
	}
}
