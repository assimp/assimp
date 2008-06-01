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
	if (rcIn.mOffsetU && 0.0f == fmodf(rcIn.mOffsetU, 1.0f ))
	{
		DefaultLogger::get()->warn("Texture coordinate offset in the x direction "
			"is a multiple of 1. This is redundant ...");
		rcIn.mOffsetU = 1.0f;
	}
	if (rcIn.mOffsetV && 0.0f == fmodf(rcIn.mOffsetV, 1.0f ))
	{
		DefaultLogger::get()->warn("Texture coordinate offset in the y direction "
				"is a multiple of 1. This is redundant ...");
		rcIn.mOffsetV = 1.0f;
	}
	if (rcIn.mRotation)
	{
		const float f =  fmodf(rcIn.mRotation,2.0f * 3.141592653f );
		if (f <= 0.05f && f >= -0.05f)
		{
			DefaultLogger::get()->warn("Texture coordinate rotation is a multiple "
				"of 2 * PI. This is redundant");
			rcIn.mRotation = 0.0f;
		}
	}
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
			(*i).fRotation == pcTex->mRotation)
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
	sInfo.pcTextures.push_back(pcTex);

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
		if (1 == iCnt)
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
void TextureTransform::ApplyScaleNOffset(std::vector<Dot3DS::Material> materials)
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
	if (1 == pcSrc->iBakeUVTransform)
	{
		std::string s;
		std::stringstream ss(s);
		ss << "Transforming existing UV channel. Source UV: " << 0 
			<< " OffsetU: " << pcSrc->pcSingleTexture->mOffsetU
			<< " OffsetV: " << pcSrc->pcSingleTexture->mOffsetV
			<< " ScaleU: " << pcSrc->pcSingleTexture->mScaleU
			<< " ScaleV: " << pcSrc->pcSingleTexture->mScaleV
			<< " Rotation (rad): " << pcSrc->pcSingleTexture->mRotation;
		DefaultLogger::get()->info(s);

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
	else if (2 == pcSrc->iBakeUVTransform)
	{
		// now we need to find all textures in the material
		// which require scaling/offset operations
		std::vector<STransformVecInfo> sOps;
		AddToList(sOps,&pcSrc->sTexDiffuse);
		AddToList(sOps,&pcSrc->sTexSpecular);
		AddToList(sOps,&pcSrc->sTexEmissive);
		AddToList(sOps,&pcSrc->sTexOpacity);
		AddToList(sOps,&pcSrc->sTexBump);
		AddToList(sOps,&pcSrc->sTexShininess);
		AddToList(sOps,&pcSrc->sTexAmbient);

		const aiVector3D* _pvBase;
		if (0.0f == sOps[0].fOffsetU && 0.0f == sOps[0].fOffsetV &&
			1.0f == sOps[0].fScaleU  && 1.0f == sOps[0].fScaleV &&
			0.0f == sOps[0].fRotation)
		{
			// we'll have an unmodified set, so we can use *this* one
			_pvBase = pcMesh->mTextureCoords[0];
		}
		else
		{
			_pvBase = new aiVector3D[pcMesh->mNumVertices];
			memcpy(const_cast<aiVector3D*>(_pvBase),pcMesh->mTextureCoords[0],
				pcMesh->mNumVertices * sizeof(aiVector3D));
		}

		unsigned int iCnt = 0;
		for (std::vector<STransformVecInfo>::iterator
			i =  sOps.begin();
			i != sOps.end();++i,++iCnt)
		{
			if (!pcMesh->mTextureCoords[iCnt])
			{
				pcMesh->mTextureCoords[iCnt] = new aiVector3D[pcMesh->mNumVertices];
			}

			// more than 4 UV texture channels are not available
			if (iCnt >= AI_MAX_NUMBER_OF_TEXTURECOORDS)
			{
				for (std::vector<Dot3DS::Texture*>::iterator
					a =  (*i).pcTextures.begin();
					a != (*i).pcTextures.end();++a)
				{
					(*a)->iUVSrc = 0;
				}
				DefaultLogger::get()->error("There are too many "
					"combinations of different UV scaling/offset/rotation operations "
					"to generate an UV channel for each (maximum is 4). Using the "
					"first UV channel ...");
				continue;
			}

			std::string s;
			std::stringstream ss(s);
			ss << "Generating additional UV channel. Source UV: " << 0 
				<< " OffsetU: " << (*i).fOffsetU
				<< " OffsetV: " << (*i).fOffsetV
				<< " ScaleU: " << (*i).fScaleU
				<< " ScaleV: " << (*i).fScaleV
				<< " Rotation (rad): " << (*i).fRotation;
			DefaultLogger::get()->info(s);

			const aiVector3D* pvBase = _pvBase;

			if (0.0f == (*i).fRotation)
			{
				for (unsigned int n = 0; n < pcMesh->mNumVertices;++n)
				{
					// scaling
					pcMesh->mTextureCoords[iCnt][n].x = pvBase->x * (*i).fScaleU;
					pcMesh->mTextureCoords[iCnt][n].y = pvBase->y * (*i).fScaleV;

					// offset
					pcMesh->mTextureCoords[iCnt][n].x += (*i).fOffsetU;
					pcMesh->mTextureCoords[iCnt][n].y += (*i).fOffsetV;

					pvBase++;
				}
			}
			else
			{
				const float fSin = sinf((*i).fRotation);
				const float fCos = cosf((*i).fRotation);
				for (unsigned int n = 0; n < pcMesh->mNumVertices;++n)
				{
					// scaling
					pcMesh->mTextureCoords[iCnt][n].x = pvBase->x * (*i).fScaleU;
					pcMesh->mTextureCoords[iCnt][n].y = pvBase->y * (*i).fScaleV;

					// rotation
					pcMesh->mTextureCoords[iCnt][n].x *= fCos;
					pcMesh->mTextureCoords[iCnt][n].y *= fSin;

					// offset
					pcMesh->mTextureCoords[iCnt][n].x += (*i).fOffsetU;
					pcMesh->mTextureCoords[iCnt][n].y += (*i).fOffsetV;

					pvBase++;
				}
			}
			// setup the UV source index for each texture
			for (std::vector<Dot3DS::Texture*>::iterator
				a =  (*i).pcTextures.begin();
				a != (*i).pcTextures.end();++a)
			{
				(*a)->iUVSrc = iCnt;
			}
		}

		// release temporary storage
		if (_pvBase != pcMesh->mTextureCoords[0])
			delete[] _pvBase;
	}
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