/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

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

----------------------------------------------------------------------
*/

/** @file A helper class that processes texture transformations */


#include "../include/aiTypes.h"
#include "../include/DefaultLogger.h"
#include "../include/aiAssert.h"

#include "MaterialSystem.h"
#include "TextureTransform.h"

namespace Assimp
{

// ------------------------------------------------------------------------------------------------
void TextureTransform::PreProcessUVTransform(
	Dot3DS::Texture& rcIn)
{
	char szTemp[512];
	int iField;

	if (rcIn.mOffsetU)
	{
		if ((iField = (int)rcIn.mOffsetU))
		{
			if (aiTextureMapMode_Wrap == rcIn.mMapMode)
			{
				float fNew = rcIn.mOffsetU-(float)iField;
				sprintf(szTemp,"[wrap] Found texture coordinate U offset %f. "
					"This can be optimized to %f",rcIn.mOffsetU,fNew);
	
				DefaultLogger::get()->info(szTemp);
				rcIn.mOffsetU = fNew;
			}
			else if (aiTextureMapMode_Mirror == rcIn.mMapMode)
			{
				if (0 != (iField % 2))iField--;
				float fNew = rcIn.mOffsetU-(float)iField;

				sprintf(szTemp,"[mirror] Found texture coordinate U offset %f. "
					"This can be optimized to %f",rcIn.mOffsetU,fNew);

				DefaultLogger::get()->info(szTemp);
				rcIn.mOffsetU = fNew;
			}
			else if (aiTextureMapMode_Clamp == rcIn.mMapMode)
			{
				sprintf(szTemp,"[clamp] Found texture coordinate U offset %f. "
					"This can be clamped to 1.0f",rcIn.mOffsetU);

				DefaultLogger::get()->info(szTemp);
				rcIn.mOffsetU = 1.0f;
			}
		}
	}
	if (rcIn.mOffsetV)
	{
		if ((iField = (int)rcIn.mOffsetV))
		{
			if (aiTextureMapMode_Wrap == rcIn.mMapMode)
			{
				float fNew = rcIn.mOffsetV-(float)iField;
				sprintf(szTemp,"[wrap] Found texture coordinate V offset %f. "
					"This can be optimized to %f",rcIn.mOffsetV,fNew);

				DefaultLogger::get()->info(szTemp);
				rcIn.mOffsetV = fNew;
			}
			else if (aiTextureMapMode_Mirror == rcIn.mMapMode)
			{
				if (0 != (iField % 2))iField--;
				float fNew = rcIn.mOffsetV-(float)iField;

				sprintf(szTemp,"[mirror] Found texture coordinate V offset %f. "
					"This can be optimized to %f",rcIn.mOffsetV,fNew);
				
				DefaultLogger::get()->info(szTemp);
				rcIn.mOffsetV = fNew;
			}
			else if (aiTextureMapMode_Clamp == rcIn.mMapMode)
			{
				sprintf(szTemp,"[clamp] Found texture coordinate U offset %f. "
					"This can be clamped to 1.0f",rcIn.mOffsetV);

				DefaultLogger::get()->info(szTemp);
				rcIn.mOffsetV = 1.0f;
			}
		}
	}
	if (rcIn.mRotation)
	{
		if ((iField = (int)(rcIn.mRotation / 3.141592654f)))
		{
			float fNew = rcIn.mRotation-(float)iField*3.141592654f;

			sprintf(szTemp,"[wrap] Found texture coordinate rotation %f. "
				"This can be optimized to %f",rcIn.mRotation,fNew);
			DefaultLogger::get()->info(szTemp);

			rcIn.mRotation = fNew;
		}
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void TextureTransform::AddToList(std::vector<STransformVecInfo>& rasVec,
	Dot3DS::Texture* pcTex)
{
	// check whether the texture is existing
	if (0 == pcTex->mMapName.length())return;

	// search for an identical transformation in our list
	for (std::vector<STransformVecInfo>::iterator
		i =  rasVec.begin();
		i != rasVec.end();++i)
	{
		if ((*i).fOffsetU == pcTex->mOffsetU &&
			(*i).fOffsetV == pcTex->mOffsetV && 
			(*i).fScaleU  == pcTex->mScaleU  &&
			(*i).fScaleV  == pcTex->mScaleV  &&
			(*i).fRotation == pcTex->mRotation &&
			(*i).iUVIndex == (unsigned int)pcTex->iUVSrc)
		{
			(*i).pcTextures.push_back(pcTex);
			return;
		}
	}
	// this is a new transformation, so add it to the list
	STransformVecInfo sInfo;
	sInfo.fScaleU = pcTex->mScaleU;
	sInfo.fScaleV = pcTex->mScaleV;
	sInfo.fOffsetU = pcTex->mOffsetU;
	sInfo.fOffsetV = pcTex->mOffsetV;
	sInfo.fRotation = pcTex->mRotation;
	sInfo.iUVIndex = pcTex->iUVSrc;

	// add the texture to the list
	sInfo.pcTextures.push_back(pcTex);

	// and add the transformation itself to the second list
	rasVec.push_back(sInfo);
}
// ------------------------------------------------------------------------------------------------
void TextureTransform::ApplyScaleNOffset(Dot3DS::Material& material)
{
	unsigned int iCnt = 0;
	Dot3DS::Texture* pcTexture = NULL;

	// diffuse texture
	if (material.sTexDiffuse.mMapName.length())
	{
		PreProcessUVTransform(material.sTexDiffuse);
		if (HasUVTransform(material.sTexDiffuse))
		{
			material.sTexDiffuse.bPrivate = true;
			pcTexture = &material.sTexDiffuse;
			++iCnt;
		}
	}
	// specular texture
	if (material.sTexSpecular.mMapName.length())
	{
		PreProcessUVTransform(material.sTexSpecular);
		if (HasUVTransform(material.sTexSpecular))
		{
			material.sTexSpecular.bPrivate = true;
			pcTexture = &material.sTexSpecular;
			++iCnt;
		}
	}
	// ambient texture
	if (material.sTexAmbient.mMapName.length())
	{
		PreProcessUVTransform(material.sTexAmbient);
		if (HasUVTransform(material.sTexAmbient))
		{
			material.sTexAmbient.bPrivate = true;
			pcTexture = &material.sTexAmbient;
			++iCnt;
		}
	}
	// emissive texture
	if (material.sTexEmissive.mMapName.length())
	{
		PreProcessUVTransform(material.sTexEmissive);
		if (HasUVTransform(material.sTexEmissive))
		{
			material.sTexEmissive.bPrivate = true;
			pcTexture = &material.sTexEmissive;
			++iCnt;
		}
	}
	// opacity texture
	if (material.sTexOpacity.mMapName.length())
	{
		PreProcessUVTransform(material.sTexOpacity);
		if (HasUVTransform(material.sTexOpacity))
		{
			material.sTexOpacity.bPrivate = true;
			pcTexture = &material.sTexOpacity;
			++iCnt;
		}
	}
	// bump texture
	if (material.sTexBump.mMapName.length())
	{
		PreProcessUVTransform(material.sTexBump);
		if (HasUVTransform(material.sTexBump))
		{
			material.sTexBump.bPrivate = true;
			pcTexture = &material.sTexBump;
			++iCnt;
		}
	}
	// shininess texture
	if (material.sTexShininess.mMapName.length())
	{
		PreProcessUVTransform(material.sTexShininess);
		if (HasUVTransform(material.sTexShininess))
		{
			material.sTexBump.bPrivate = true;
			pcTexture = &material.sTexShininess;
			++iCnt;
		}
	}
	if (0 != iCnt)
	{
		// if only one texture needs scaling/offset operations
		// we can apply them directly to the first texture
		// coordinate sets of all meshes referencing *this* material
		// However, we can't do it  now. We need to wait until
		// everything is sorted by materials.
		if (1 == iCnt && 0 == pcTexture->iUVSrc)
		{
			material.iBakeUVTransform = 1;
			material.pcSingleTexture = pcTexture;
		}
		// we will need to generate a separate new texture channel
		// for each texture. 
		// However, we can't do it  now. We need to wait until
		// everything is sorted by materials.
		else material.iBakeUVTransform = 2;
	}
}
// ------------------------------------------------------------------------------------------------
void TextureTransform::ApplyScaleNOffset(std::vector<Dot3DS::Material>& materials)
{
	unsigned int iNum = 0;
	for (std::vector<Dot3DS::Material>::iterator
		i =  materials.begin();
		i != materials.end();++i,++iNum)
	{
		ApplyScaleNOffset(*i);
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void TextureTransform::BakeScaleNOffset(
	aiMesh* pcMesh, Dot3DS::Material* pcSrc)
{
	// NOTE: we don't use a texture matrix to do the transformation
	// it is more efficient this way ... 

	if (!pcMesh->mTextureCoords[0])return;
	if (0x1 == pcSrc->iBakeUVTransform)
	{
		char szTemp[512];
		int iLen;
#if _MSC_VER >= 1400
		iLen = ::sprintf_s(szTemp,
#else
		iLen = ::sprintf(szTemp,
#endif
			"Transforming existing UV channel. Source UV: %i" 
			" OffsetU: %f" 
			" OffsetV: %f" 
			" ScaleU: %f" 
			" ScaleV: %f" 
			" Rotation (rad): %f",0,
			pcSrc->pcSingleTexture->mOffsetU,
			pcSrc->pcSingleTexture->mOffsetV,
			pcSrc->pcSingleTexture->mScaleU,
			pcSrc->pcSingleTexture->mScaleV,
			pcSrc->pcSingleTexture->mRotation);

		ai_assert(0 < iLen);
		DefaultLogger::get()->info(std::string(szTemp,iLen));

		if (!pcSrc->pcSingleTexture->mRotation)
		{
			for (unsigned int i = 0; i < pcMesh->mNumVertices;++i)
			{
				// scaling
				pcMesh->mTextureCoords[0][i].x *= pcSrc->pcSingleTexture->mScaleU;
				pcMesh->mTextureCoords[0][i].y *= pcSrc->pcSingleTexture->mScaleV;

				// offset
				pcMesh->mTextureCoords[0][i].x += pcSrc->pcSingleTexture->mOffsetU;
				pcMesh->mTextureCoords[0][i].y += pcSrc->pcSingleTexture->mOffsetV;
			}
		}
		else
		{
			const float fSin = sinf(pcSrc->pcSingleTexture->mRotation);
			const float fCos = cosf(pcSrc->pcSingleTexture->mRotation);
			for (unsigned int i = 0; i < pcMesh->mNumVertices;++i)
			{
				// scaling
				pcMesh->mTextureCoords[0][i].x *= pcSrc->pcSingleTexture->mScaleU;
				pcMesh->mTextureCoords[0][i].y *= pcSrc->pcSingleTexture->mScaleV;

				// rotation
				pcMesh->mTextureCoords[0][i].x *= fCos;
				pcMesh->mTextureCoords[0][i].y *= fSin;

				// offset
				pcMesh->mTextureCoords[0][i].x += pcSrc->pcSingleTexture->mOffsetU;
				pcMesh->mTextureCoords[0][i].y += pcSrc->pcSingleTexture->mOffsetV;
			}
		}
	}
	else if (0x2 == pcSrc->iBakeUVTransform)
	{
		// first save all texture coordinate sets
		aiVector3D* apvOriginalSets[AI_MAX_NUMBER_OF_TEXTURECOORDS];
		for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS;++i)
		{
			apvOriginalSets[i] = pcMesh->mTextureCoords[i];
		}
		unsigned int iNextEmpty = 0;
		while (pcMesh->mTextureCoords[++iNextEmpty]);

		aiVector3D* apvOutputSets[AI_MAX_NUMBER_OF_TEXTURECOORDS];
		for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS;++i)
			apvOutputSets[i] = NULL;

		// now we need to find all textures in the material
		// which require scaling/offset operations
		std::vector<STransformVecInfo> sOps;
		sOps.reserve(10);
		TextureTransform::AddToList(sOps,&pcSrc->sTexDiffuse);
		TextureTransform::AddToList(sOps,&pcSrc->sTexSpecular);
		TextureTransform::AddToList(sOps,&pcSrc->sTexEmissive);
		TextureTransform::AddToList(sOps,&pcSrc->sTexOpacity);
		TextureTransform::AddToList(sOps,&pcSrc->sTexBump);
		TextureTransform::AddToList(sOps,&pcSrc->sTexShininess);
		TextureTransform::AddToList(sOps,&pcSrc->sTexAmbient);

		// check the list and find out how many we won't be able
		// to generate.
		std::vector<STransformVecInfo*> sFilteredOps;
		unsigned int iNumUntransformed = 0;
		sFilteredOps.reserve(sOps.size());
		{
			std::vector<STransformVecInfo*> sWishList;
			sWishList.reserve(sOps.size());
			for (unsigned int iUV = 0; iUV < AI_MAX_NUMBER_OF_TEXTURECOORDS;++iUV)
			{
				for (std::vector<STransformVecInfo>::iterator
					i =  sOps.begin();
					i != sOps.end();++i)
				{
					if (iUV != (*i).iUVIndex)continue;
					if ((*i).IsUntransformed())
					{
						sFilteredOps.push_back(&(*i));
					}
					else sWishList.push_back(&(*i));
				}
			}
			// are we able to generate all?
			const int iDiff = AI_MAX_NUMBER_OF_TEXTURECOORDS-(int)
				(sWishList.size()+sFilteredOps.size());

			iNumUntransformed  = (unsigned int)sFilteredOps.size();
			if (0 >= iDiff)
			{
				DefaultLogger::get()->warn("There are too many combinations of different "
					"UV transformation operations to generate an own UV channel for each "
					"(maximum is AI_MAX_NUMBER_OF_TEXTURECOORDS = 4 or 6). "
					"An untransformed UV channel will be used for all remaining transformations");
				
				std::vector<STransformVecInfo*>::const_iterator nash =  sWishList.begin();
				for (;nash != sWishList.end()-iDiff;++nash)
				{
					sFilteredOps.push_back(*nash);
				}
			}
			else
			{
				for (std::vector<STransformVecInfo*>::const_iterator
					nash =  sWishList.begin();
					nash != sWishList.end();++nash)sFilteredOps.push_back(*nash);
			}
		}

		// now fill in all output IV indices
		unsigned int iNum = 0;
		for (std::vector<STransformVecInfo*>::iterator
			bogart =  sFilteredOps.begin();
			bogart != sFilteredOps.end();++bogart,++iNum)
		{
			(**bogart).iUVIndex = iNum;
		}

		iNum = 0;
		for (; iNum < iNumUntransformed; ++iNum)
			pcMesh->mTextureCoords[iNum] = apvOriginalSets[iNum];

		// now generate the texture coordinate sets
		for (std::vector<STransformVecInfo*>::iterator
			i =  sFilteredOps.begin()+iNumUntransformed;
			i != sFilteredOps.end();++i,++iNum)
		{
			const aiVector3D* _pvBase = apvOriginalSets[(**i).iUVIndex];
			aiVector3D* _pvOut = new aiVector3D[pcMesh->mNumVertices];
			pcMesh->mTextureCoords[iNum] = _pvOut;

			char szTemp[512];
			int iLen;
#if _MSC_VER >= 1400
			iLen = ::sprintf_s(szTemp,
#else
			iLen = ::sprintf(szTemp,
#endif
				"Generating additional UV channel. Source UV: %i" 
				" OffsetU: %f" 
				" OffsetV: %f" 
				" ScaleU: %f" 
				" ScaleV: %f" 
				" Rotation (rad): %f",0,
				(**i).fOffsetU,
				(**i).fOffsetV,
				(**i).fScaleU,
				(**i).fScaleV,
				(**i).fRotation);
			ai_assert(0 < iLen);
			DefaultLogger::get()->info(std::string(szTemp,iLen));

			const aiVector3D* pvBase = _pvBase;
			aiVector3D* pvOut = _pvOut;
			if (0.0f == (**i).fRotation)
			{
				for (unsigned int n = 0; n < pcMesh->mNumVertices;++n)
				{
					// scaling
					pvOut->x = pvBase->x * (**i).fScaleU;
					pvOut->y = pvBase->y * (**i).fScaleV;

					// offset
					pvOut->x += (**i).fOffsetU;
					pvOut->y += (**i).fOffsetV;

					pvBase++;
					pvOut++;
				}
			}
			else
			{
				const float fSin = sinf((**i).fRotation);
				const float fCos = cosf((**i).fRotation);
				for (unsigned int n = 0; n < pcMesh->mNumVertices;++n)
				{
					// scaling
					pvOut->x = pvBase->x * (**i).fScaleU;
					pvOut->y = pvBase->y * (**i).fScaleV;

					// rotation
					pvOut->x *= fCos;
					pvOut->y *= fSin;

					// offset
					pvOut->x += (**i).fOffsetU;
					pvOut->y += (**i).fOffsetV;

					pvBase++;
					pvOut++;
				}
			}
		}

		// now check which source texture coordinate sets
		// can be deleted because they're not anymore required
		for (iNum = 0; iNum < AI_MAX_NUMBER_OF_TEXTURECOORDS;++iNum)
		{
			for (unsigned int z = 0; z < iNumUntransformed;++z)
			{
				if (apvOriginalSets[iNum] == pcMesh->mTextureCoords[z])
				{
					apvOriginalSets[iNum] = NULL;
					break;
				}
			}
			if (apvOriginalSets[iNum])delete[] apvOriginalSets[iNum];
		}
	}

	// setup bitflags to indicate which texture coordinate
	// channels are used (this class works for 2d texture coordinates only)

	unsigned int iIndex = 0;
	while (pcMesh->HasTextureCoords(iIndex))pcMesh->mNumUVComponents[iIndex++] = 2;
	return;
}
// ------------------------------------------------------------------------------------------------
void TextureTransform::SetupMatUVSrc (aiMaterial* pcMat, const Dot3DS::Material* pcMatIn)
{
	ai_assert(NULL != pcMat);
	ai_assert(NULL != pcMatIn);
	
	MaterialHelper* pcHelper = (MaterialHelper*)pcMat;

	if(pcMatIn->sTexDiffuse.mMapName.length() > 0)
		pcHelper->AddProperty<int>(&pcMatIn->sTexDiffuse.iUVSrc,1,
			AI_MATKEY_UVWSRC_DIFFUSE(0));

	if(pcMatIn->sTexSpecular.mMapName.length() > 0)
		pcHelper->AddProperty<int>(&pcMatIn->sTexSpecular.iUVSrc,1,
			AI_MATKEY_UVWSRC_SPECULAR(0));

	if(pcMatIn->sTexEmissive.mMapName.length() > 0)
		pcHelper->AddProperty<int>(&pcMatIn->sTexEmissive.iUVSrc,1,
			AI_MATKEY_UVWSRC_EMISSIVE(0));

	if(pcMatIn->sTexBump.mMapName.length() > 0)
		pcHelper->AddProperty<int>(&pcMatIn->sTexBump.iUVSrc,1,
			AI_MATKEY_UVWSRC_HEIGHT(0));

	if(pcMatIn->sTexShininess.mMapName.length() > 0)
		pcHelper->AddProperty<int>(&pcMatIn->sTexShininess.iUVSrc,1,
			AI_MATKEY_UVWSRC_SHININESS(0));

	if(pcMatIn->sTexOpacity.mMapName.length() > 0)
		pcHelper->AddProperty<int>(&pcMatIn->sTexOpacity.iUVSrc,1,
			AI_MATKEY_UVWSRC_OPACITY(0));

	if(pcMatIn->sTexAmbient.mMapName.length() > 0)
		pcHelper->AddProperty<int>(&pcMatIn->sTexAmbient.iUVSrc,1,
			AI_MATKEY_UVWSRC_AMBIENT(0));
}
};
