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

/** @file Implementation of the material oart of the LWO importer class */

// internal headers
#include "LWOLoader.h"
#include "MaterialSystem.h"
#include "ByteSwap.h"

// public assimp headers
#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiScene.h"
#include "../include/aiAssert.h"
#include "../include/DefaultLogger.h"

// boost headers
#include <boost/scoped_ptr.hpp>

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
void LWOImporter::ConvertMaterial(const LWO::Surface& surf,MaterialHelper* pcMat)
{
	// copy the name of the surface
	aiString st;
	st.Set(surf.mName);
	pcMat->AddProperty(&st,AI_MATKEY_NAME);

	int i = surf.bDoubleSided ? 1 : 0;
	pcMat->AddProperty<int>(&i,1,AI_MATKEY_TWOSIDED);
	
	if (surf.mSpecularValue && surf.mGlossiness)
	{
		// this is only an assumption, needs to be confirmed.
		// the values have been tweaked by hand and seem to be correct.
		float fGloss;
		if (mIsLWO2)fGloss = surf.mGlossiness * 0.8f;
		else
		{
			if (16.0f >= surf.mGlossiness)fGloss = 6.0f;
			else if (64.0f >= surf.mGlossiness)fGloss = 20.0f;
			else if (256.0f >= surf.mGlossiness)fGloss = 50.0f;
			else fGloss = 80.0f;
		}

		pcMat->AddProperty<float>(&surf.mSpecularValue,1,AI_MATKEY_SHININESS_STRENGTH);
		pcMat->AddProperty<float>(&fGloss,1,AI_MATKEY_SHININESS);
	}

	// (the diffuse value is just a scaling factor)
	aiColor3D clr = surf.mColor;
	clr.r *= surf.mDiffuseValue;
	clr.g *= surf.mDiffuseValue;
	clr.b *= surf.mDiffuseValue;
	pcMat->AddProperty<aiColor3D>(&clr,1,AI_MATKEY_COLOR_DIFFUSE);

	// specular color
	clr.b = clr.g  = clr.r = surf.mSpecularValue;
	pcMat->AddProperty<aiColor3D>(&clr,1,AI_MATKEY_COLOR_SPECULAR);

	// emissive color
	// (luminosity is not really the same but it affects the surface in 
	//  a similar way. However, some scalings seems to be necessary)
	clr.g = clr.b = clr.r = surf.mLuminosity*0.8f;
	pcMat->AddProperty<aiColor3D>(&clr,1,AI_MATKEY_COLOR_EMISSIVE);

	// opacity
	float f = 1.0f-surf.mTransparency;
	pcMat->AddProperty<float>(&f,1,AI_MATKEY_OPACITY);

	// now handle all textures ...
	// TODO
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::FindUVChannels(const LWO::Surface& surf, const LWO::Layer& layer,
	unsigned int out[AI_MAX_NUMBER_OF_TEXTURECOORDS])
{
	out[0] = 0xffffffff;
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::FindVCChannels(const LWO::Surface& surf, const LWO::Layer& layer,
	unsigned int out[AI_MAX_NUMBER_OF_COLOR_SETS])
{
	out[0] = 0xffffffff;
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2ImageMap(unsigned int size, LWO::Texture& tex )
{
	LE_NCONST uint8_t* const end = mFileBuffer + size;
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2Procedural(unsigned int size, LWO::Texture& tex )
{
	LE_NCONST uint8_t* const end = mFileBuffer + size;
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2Gradient(unsigned int size, LWO::Texture& tex  )
{
	LE_NCONST uint8_t* const end = mFileBuffer + size;
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2TextureHeader(unsigned int size, LWO::Texture& tex )
{
	LE_NCONST uint8_t* const end = mFileBuffer + size;
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2TextureBlock(uint32_t type, unsigned int size )
{
	LE_NCONST uint8_t* const end = mFileBuffer + size;

	LWO::Surface& surf = mSurfaces->back();
	LWO::Texture tex;

	// now get the exact type of the texture
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2Surface(unsigned int size)
{
	LE_NCONST uint8_t* const end = mFileBuffer + size;

	mSurfaces->push_back( LWO::Surface () );
	LWO::Surface& surf = mSurfaces->back();

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
			throw new ImportErrorException("LWO2: Invalid file, the size attribute of "
				"a surface sub chunk points behind the end of the file");
		}
		LE_NCONST uint8_t* const next = mFileBuffer+head_length;
		switch (head_type)
		{
			// diffuse color
		case AI_LWO_COLR:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head_length,COLR,12);
				surf.mColor.r = ((float*)mFileBuffer)[0];
				surf.mColor.g = ((float*)mFileBuffer)[1];
				surf.mColor.b = ((float*)mFileBuffer)[2];
				AI_LSWAP4(surf.mColor.r);
				AI_LSWAP4(surf.mColor.g);
				AI_LSWAP4(surf.mColor.b);
				break;
			}
			// diffuse strength ... hopefully
		case AI_LWO_DIFF:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head_length,DIFF,4);
				surf.mDiffuseValue = *((float*)mFileBuffer);
				AI_LSWAP4(surf.mDiffuseValue);
				break;
			}
			// specular strength ... hopefully
		case AI_LWO_SPEC:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head_length,SPEC,4);
				surf.mSpecularValue = *((float*)mFileBuffer);
				AI_LSWAP4(surf.mSpecularValue);
				break;
			}
			// transparency
		case AI_LWO_TRAN:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head_length,TRAN,4);
				surf.mTransparency = *((float*)mFileBuffer);
				AI_LSWAP4(surf.mTransparency);
				break;
			}
			// glossiness
		case AI_LWO_GLOS:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head_length,GLOS,4);
				surf.mGlossiness = *((float*)mFileBuffer);
				AI_LSWAP4(surf.mGlossiness);
				break;
			}
			// surface bock entry
		case AI_LWO_BLOK:
			{
				AI_LWO_VALIDATE_CHUNK_LENGTH(head_length,BLOK,4);
				uint32_t type = *((uint32_t*)mFileBuffer);
				AI_LSWAP4(type);

				switch (type)
				{
				case AI_LWO_IMAP:
				case AI_LWO_PROC:
				case AI_LWO_GRAD:
					mFileBuffer+=4;
					LoadLWO2TextureBlock(type,head_length-4);
					break;
				};

				break;
			}
		}
		mFileBuffer = next;
	}
}

// ------------------------------------------------------------------------------------------------
bool LWOImporter::ComputeGradientTexture(LWO::GradientInfo& grad,
	std::vector<aiTexture*>& out)
{
	aiTexture* tex = new aiTexture();

	tex->mHeight = configGradientResY;
	tex->mWidth = configGradientResX;
	unsigned int numPixels;
	tex->pcData = new aiTexel[numPixels = tex->mHeight * tex->mWidth];

	// to be implemented ...

	out.push_back(tex);
	return true;
}
