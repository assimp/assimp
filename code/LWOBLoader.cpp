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

// internal headers
#include "LWOLoader.h"
#include "MaterialSystem.h"
#include "ByteSwap.h"

// public assimp headers
#include "../include/aiScene.h"
#include "../include/aiAssert.h"
#include "../include/DefaultLogger.h"
#include "../include/assimp.hpp"

using namespace Assimp;


// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWOBFile()
{
	LE_NCONST uint8_t* const end = mFileBuffer + fileSize;
	while (true)
	{
		if (mFileBuffer + sizeof(IFF::ChunkHeader) > end)break;
		LE_NCONST IFF::ChunkHeader* const head = (LE_NCONST IFF::ChunkHeader*)mFileBuffer;
		AI_LSWAP4(head->length);
		AI_LSWAP4(head->type);
		mFileBuffer += sizeof(IFF::ChunkHeader);
		if (mFileBuffer + head->length > end)
		{
			throw new ImportErrorException("LWOB: Invalid file, the size attribute of "
				"a chunk points behind the end of the file");
			break;
		}
		LE_NCONST uint8_t* const next = mFileBuffer+head->length;
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
				else LoadLWOPolygons(head->length);
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
				if (!mSurfaces->empty())
					DefaultLogger::get()->warn("LWO: SURF chunk encountered twice");
				else LoadLWOBSurface(head->length);
				break;
			}
		}
		mFileBuffer = next;
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
		if(face.mNumIndices = *cursor++)
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
void LWOImporter::LoadLWOBSurface(unsigned int size)
{
	LE_NCONST uint8_t* const end = mFileBuffer + size;

	mSurfaces->push_back( LWO::Surface () );
	LWO::Surface& surf = mSurfaces->back();
	LWO::Texture* pTex = NULL;

	ParseString(surf.mName,size);
	mFileBuffer+=surf.mName.length()+1;
	// skip one byte if the length of the surface name is odd
	if (!(surf.mName.length() & 1))++mFileBuffer; 
	while (true)
	{
		if (mFileBuffer + 6 > end)break;

		// no proper IFF header here - the chunk length is specified as int16
		uint32_t head_type		= *((LE_NCONST uint32_t*)mFileBuffer);mFileBuffer+=4;
		uint16_t head_length	= *((LE_NCONST uint16_t*)mFileBuffer);mFileBuffer+=2;
		AI_LSWAP4(head_type);
		AI_LSWAP2(head_length);
		if (mFileBuffer + head_length > end)
		{
			throw new ImportErrorException("LWOB: Invalid file, the size attribute of "
				"a surface sub chunk points behind the end of the file");
		}
		LE_NCONST uint8_t* const next = mFileBuffer+head_length;
		switch (head_type)
		{
		// diffuse color
		case AI_LWO_COLR:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head_length,COLR,3);
				surf.mColor.r = *mFileBuffer++ / 255.0f;
				surf.mColor.g = *mFileBuffer++ / 255.0f;
				surf.mColor.b = *mFileBuffer   / 255.0f;
				break;
			}
		// diffuse strength ...
		case AI_LWO_DIFF:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head_length,DIFF,2);
				AI_LSWAP2P(mFileBuffer);
				surf.mDiffuseValue = *((int16_t*)mFileBuffer) / 255.0f;
				break;
			}
		// specular strength ... 
		case AI_LWO_SPEC:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head_length,SPEC,2);
				AI_LSWAP2P(mFileBuffer);
				surf.mSpecularValue = *((int16_t*)mFileBuffer) / 255.0f;
				break;
			}
		// luminosity ... 
		case AI_LWO_LUMI:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head_length,LUMI,2);
				AI_LSWAP2P(mFileBuffer);
				surf.mLuminosity = *((int16_t*)mFileBuffer) / 255.0f;
				break;
			}
		// transparency
		case AI_LWO_TRAN:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head_length,TRAN,2);
				AI_LSWAP2P(mFileBuffer);
				surf.mTransparency = *((int16_t*)mFileBuffer) / 255.0f;
				break;
			}
		// glossiness
		case AI_LWO_GLOS:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head_length,GLOS,2);
				AI_LSWAP2P(mFileBuffer);
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
					ParseString(pTex->mFileName,head_length);	
					AdjustTexturePath(pTex->mFileName);
					mFileBuffer += pTex->mFileName.length();
				}
				else DefaultLogger::get()->warn("LWOB: TIMG tag was encuntered although "
					"there was no xTEX tag before");
				break;
			}
		// texture strength
		case AI_LWO_TVAL:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head_length,TVAL,1);
				if (pTex)pTex->mStrength = *mFileBuffer / 255.0f;
				else DefaultLogger::get()->warn("LWOB: TVAL tag was encuntered "
					"although there was no xTEX tag before");
				break;
			}
		}
		mFileBuffer = next;
	}
}
