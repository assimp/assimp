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

// internal headers
#include "3DSLoader.h"
#include "MaterialSystem.h"
#include "TextureTransform.h"
#include "StringComparison.h"
#include "qnan.h"

// public ASSIMP headers
#include "../include/DefaultLogger.h"
#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"
#include "../include/aiAssert.h"


using namespace Assimp;


// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::ReplaceDefaultMaterial()
{
	// try to find an existing material that matches the
	// typical default material setting:
	// - no textures
	// - diffuse color (in grey!)
	// NOTE: This is here to workaround the fact that some
	// exporters are writing a default material, too.
	unsigned int iIndex = 0xcdcdcdcd;
	for (unsigned int i = 0; i < this->mScene->mMaterials.size();++i)
	{
	if (std::string::npos == this->mScene->mMaterials[i].mName.find("default") &&
		std::string::npos == this->mScene->mMaterials[i].mName.find("DEFAULT"))continue;

	if (this->mScene->mMaterials[i].mDiffuse.r !=
		this->mScene->mMaterials[i].mDiffuse.g ||
		this->mScene->mMaterials[i].mDiffuse.r !=
		this->mScene->mMaterials[i].mDiffuse.b)continue;

	if (this->mScene->mMaterials[i].sTexDiffuse.mMapName.length() != 0	||
		this->mScene->mMaterials[i].sTexBump.mMapName.length()!= 0		|| 
		this->mScene->mMaterials[i].sTexOpacity.mMapName.length() != 0	||
		this->mScene->mMaterials[i].sTexEmissive.mMapName.length() != 0	||
		this->mScene->mMaterials[i].sTexSpecular.mMapName.length() != 0	||
		this->mScene->mMaterials[i].sTexShininess.mMapName.length() != 0 )continue;

	iIndex = i;
	}
	if (0xcdcdcdcd == iIndex)iIndex = (unsigned int)this->mScene->mMaterials.size();

	// now iterate through all meshes and through all faces and
	// find all faces that are using the default material
	unsigned int iCnt = 0;
	for (std::vector<Dot3DS::Mesh>::iterator
		i =  this->mScene->mMeshes.begin();
		i != this->mScene->mMeshes.end();++i)
	{
		for (std::vector<unsigned int>::iterator
			a =  (*i).mFaceMaterials.begin();
			a != (*i).mFaceMaterials.end();++a)
		{
			// NOTE: The additional check seems to be necessary,
			// some exporters seem to generate invalid data here
			if (0xcdcdcdcd == (*a))
			{
				(*a) = iIndex;
				++iCnt;
			}
			else if ( (*a) >= this->mScene->mMaterials.size())
			{
				(*a) = iIndex;
				++iCnt;
				DefaultLogger::get()->warn("Material index overflow in 3DS file. Assigning "
					"default material ...");
			}
		}
	}
	if (iCnt && iIndex == this->mScene->mMaterials.size())
	{
		// we need to create our own default material
		Dot3DS::Material sMat;
		sMat.mDiffuse = aiColor3D(0.3f,0.3f,0.3f);
		sMat.mName = "%%%DEFAULT";
		this->mScene->mMaterials.push_back(sMat);
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::CheckIndices(Dot3DS::Mesh* sMesh)
{
	for (std::vector< Dot3DS::Face >::iterator
		 i =  sMesh->mFaces.begin();
		 i != sMesh->mFaces.end();++i)
	{
		// check whether all indices are in range
		if ((*i).mIndices[0] >= sMesh->mPositions.size())
		{
			DefaultLogger::get()->warn("Face index overflow in 3DS file (#1)");
			(*i).mIndices[0] = (uint32_t)sMesh->mPositions.size()-1;
		}
		if ((*i).mIndices[1] >= sMesh->mPositions.size())
		{
			DefaultLogger::get()->warn("Face index overflow in 3DS file (#2)");
			(*i).mIndices[1] = (uint32_t)sMesh->mPositions.size()-1;
		}
		if ((*i).mIndices[2] >= sMesh->mPositions.size())
		{
			DefaultLogger::get()->warn("Face index overflow in 3DS file (#3)");
			(*i).mIndices[2] = (uint32_t)sMesh->mPositions.size()-1;
		}
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::MakeUnique(Dot3DS::Mesh* sMesh)
{
	std::vector<aiVector3D> vNew;
	vNew.resize(sMesh->mFaces.size() * 3);

	std::vector<aiVector2D> vNew2;

	// TODO: Remove this step. By maintaining a small LUT it
	// would be possible to do this directly in the parsing step
	unsigned int iBase = 0;

	if (0 != sMesh->mTexCoords.size())
	{
		vNew2.resize(sMesh->mFaces.size() * 3);
		for (unsigned int i = 0; i < sMesh->mFaces.size();++i)
		{
			uint32_t iTemp1,iTemp2;

			// position and texture coordinates
			vNew[iBase]   = sMesh->mPositions[sMesh->mFaces[i].mIndices[2]];
			vNew2[iBase]   = sMesh->mTexCoords[sMesh->mFaces[i].mIndices[2]];
			iTemp1 = iBase++;

			vNew[iBase]   = sMesh->mPositions[sMesh->mFaces[i].mIndices[1]];
			vNew2[iBase]   = sMesh->mTexCoords[sMesh->mFaces[i].mIndices[1]];
			iTemp2 = iBase++;

			vNew[iBase]   = sMesh->mPositions[sMesh->mFaces[i].mIndices[0]];
			vNew2[iBase]   = sMesh->mTexCoords[sMesh->mFaces[i].mIndices[0]];
			sMesh->mFaces[i].mIndices[2] = iBase++;

			sMesh->mFaces[i].mIndices[0] = iTemp1;
			sMesh->mFaces[i].mIndices[1] = iTemp2;
		}
	}
	else
	{
		for (unsigned int i = 0; i < sMesh->mFaces.size();++i)
		{
			uint32_t iTemp1,iTemp2;

			// position only
			vNew[iBase]   = sMesh->mPositions[sMesh->mFaces[i].mIndices[2]];
			iTemp1 = iBase++;

			vNew[iBase]   = sMesh->mPositions[sMesh->mFaces[i].mIndices[1]];
			iTemp2 = iBase++;

			vNew[iBase]   = sMesh->mPositions[sMesh->mFaces[i].mIndices[0]];
			sMesh->mFaces[i].mIndices[2] = iBase++;

			sMesh->mFaces[i].mIndices[0] = iTemp1;
			sMesh->mFaces[i].mIndices[1] = iTemp2;
		}
	}
	sMesh->mPositions = vNew;
	sMesh->mTexCoords = vNew2;
	return;
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::ConvertMaterial(Dot3DS::Material& oldMat,
	MaterialHelper& mat)
{
	// NOTE: Pass the background image to the viewer by bypassing the
	// material system. This is an evil hack, never do it  again!
	if (0 != this->mBackgroundImage.length() && this->bHasBG)
		{
		aiString tex;
		tex.Set( this->mBackgroundImage);
		mat.AddProperty( &tex, AI_MATKEY_GLOBAL_BACKGROUND_IMAGE);

		// be sure this is only done for the first material
		this->mBackgroundImage = std::string("");
		}

	// At first add the base ambient color of the
	// scene to	the material
	oldMat.mAmbient.r += this->mClrAmbient.r;
	oldMat.mAmbient.g += this->mClrAmbient.g;
	oldMat.mAmbient.b += this->mClrAmbient.b;

	aiString name;
	name.Set( oldMat.mName);
	mat.AddProperty( &name, AI_MATKEY_NAME);

	// material colors
	mat.AddProperty( &oldMat.mAmbient, 1, AI_MATKEY_COLOR_AMBIENT);
	mat.AddProperty( &oldMat.mDiffuse, 1, AI_MATKEY_COLOR_DIFFUSE);
	mat.AddProperty( &oldMat.mSpecular, 1, AI_MATKEY_COLOR_SPECULAR);
	mat.AddProperty( &oldMat.mEmissive, 1, AI_MATKEY_COLOR_EMISSIVE);

	// phong shininess and shininess strength
	if (Dot3DS::Dot3DSFile::Phong == oldMat.mShading || 
		Dot3DS::Dot3DSFile::Metal == oldMat.mShading)
	{
		if (!oldMat.mSpecularExponent || !oldMat.mShininessStrength)
		{
			oldMat.mShading = Dot3DS::Dot3DSFile::Gouraud;
		}
		else
		{
			mat.AddProperty( &oldMat.mSpecularExponent, 1, AI_MATKEY_SHININESS);
			mat.AddProperty( &oldMat.mShininessStrength, 1, AI_MATKEY_SHININESS_STRENGTH);
		}
	}

	// opacity
	mat.AddProperty<float>( &oldMat.mTransparency,1,AI_MATKEY_OPACITY);

	// bump height scaling
	mat.AddProperty<float>( &oldMat.mBumpHeight,1,AI_MATKEY_BUMPSCALING);

	// two sided rendering?
	if (oldMat.mTwoSided)
	{
		int i = 0;
		mat.AddProperty<int>(&i,1,AI_MATKEY_TWOSIDED);
	}

	// shading mode
	aiShadingMode eShading = aiShadingMode_NoShading;
	switch (oldMat.mShading)
	{
		case Dot3DS::Dot3DSFile::Flat:
			eShading = aiShadingMode_Flat; break;

		// I don't know what "Wire" shading should be,
		// assume it is simple lambertian diffuse (L dot N) shading
		case Dot3DS::Dot3DSFile::Wire:
		case Dot3DS::Dot3DSFile::Gouraud:
			eShading = aiShadingMode_Gouraud; break;

		// assume cook-torrance shading for metals.
		// NOTE: I assume the real shader inside 3ds max is an anisotropic
		// Phong-Blinn shader, but this is a good approximation too
		case Dot3DS::Dot3DSFile::Phong :
			eShading = aiShadingMode_Phong; break;

		case Dot3DS::Dot3DSFile::Metal :
			eShading = aiShadingMode_CookTorrance; break;
	}
	mat.AddProperty<int>( (int*)&eShading,1,AI_MATKEY_SHADING_MODEL);

	if (Dot3DS::Dot3DSFile::Wire == oldMat.mShading)
	{
		// set the wireframe flag
		unsigned int iWire = 1;
		mat.AddProperty<int>( (int*)&iWire,1,AI_MATKEY_ENABLE_WIREFRAME);
	}

	// texture, if there is one
	if( oldMat.sTexDiffuse.mMapName.length() > 0)
	{
		aiString tex;
		tex.Set( oldMat.sTexDiffuse.mMapName);
		mat.AddProperty( &tex, AI_MATKEY_TEXTURE_DIFFUSE(0));

		if (is_not_qnan(oldMat.sTexDiffuse.mTextureBlend))
			mat.AddProperty<float>( &oldMat.sTexDiffuse.mTextureBlend, 1, AI_MATKEY_TEXBLEND_DIFFUSE(0));

		if (aiTextureMapMode_Clamp != oldMat.sTexDiffuse.mMapMode)
		{
			int i = (int)oldMat.sTexSpecular.mMapMode;
			mat.AddProperty<int>(&i,1,AI_MATKEY_MAPPINGMODE_U_DIFFUSE(0));
			mat.AddProperty<int>(&i,1,AI_MATKEY_MAPPINGMODE_V_DIFFUSE(0));
		}
	}
	if( oldMat.sTexSpecular.mMapName.length() > 0)
	{
		aiString tex;
		tex.Set( oldMat.sTexSpecular.mMapName);
		mat.AddProperty( &tex, AI_MATKEY_TEXTURE_SPECULAR(0));

		if (is_not_qnan(oldMat.sTexSpecular.mTextureBlend))
			mat.AddProperty<float>( &oldMat.sTexSpecular.mTextureBlend, 1, AI_MATKEY_TEXBLEND_SPECULAR(0));

		if (aiTextureMapMode_Clamp != oldMat.sTexSpecular.mMapMode)
		{
			int i = (int)oldMat.sTexSpecular.mMapMode;
			mat.AddProperty<int>(&i,1,AI_MATKEY_MAPPINGMODE_U_SPECULAR(0));
			mat.AddProperty<int>(&i,1,AI_MATKEY_MAPPINGMODE_V_SPECULAR(0));
		}
	}
	if( oldMat.sTexOpacity.mMapName.length() > 0)
	{
		aiString tex;
		tex.Set( oldMat.sTexOpacity.mMapName);
		mat.AddProperty( &tex, AI_MATKEY_TEXTURE_OPACITY(0));

		if (is_not_qnan(oldMat.sTexOpacity.mTextureBlend))
			mat.AddProperty<float>( &oldMat.sTexOpacity.mTextureBlend, 1,AI_MATKEY_TEXBLEND_OPACITY(0));
		if (aiTextureMapMode_Clamp != oldMat.sTexOpacity.mMapMode)
		{
			int i = (int)oldMat.sTexOpacity.mMapMode;
			mat.AddProperty<int>(&i,1,AI_MATKEY_MAPPINGMODE_U_OPACITY(0));
			mat.AddProperty<int>(&i,1,AI_MATKEY_MAPPINGMODE_V_OPACITY(0));
		}
	}
	if( oldMat.sTexEmissive.mMapName.length() > 0)
	{
		aiString tex;
		tex.Set( oldMat.sTexEmissive.mMapName);
		mat.AddProperty( &tex, AI_MATKEY_TEXTURE_EMISSIVE(0));

		if (is_not_qnan(oldMat.sTexEmissive.mTextureBlend))
			mat.AddProperty<float>( &oldMat.sTexEmissive.mTextureBlend, 1, AI_MATKEY_TEXBLEND_EMISSIVE(0));
		if (aiTextureMapMode_Clamp != oldMat.sTexEmissive.mMapMode)
		{
			int i = (int)oldMat.sTexEmissive.mMapMode;
			mat.AddProperty<int>(&i,1,AI_MATKEY_MAPPINGMODE_U_EMISSIVE(0));
			mat.AddProperty<int>(&i,1,AI_MATKEY_MAPPINGMODE_V_EMISSIVE(0));
		}
	}
	if( oldMat.sTexBump.mMapName.length() > 0)
	{
		aiString tex;
		tex.Set( oldMat.sTexBump.mMapName);
		mat.AddProperty( &tex, AI_MATKEY_TEXTURE_HEIGHT(0));

		if (is_not_qnan(oldMat.sTexBump.mTextureBlend))
			mat.AddProperty<float>( &oldMat.sTexBump.mTextureBlend, 1, AI_MATKEY_TEXBLEND_HEIGHT(0));
		if (aiTextureMapMode_Clamp != oldMat.sTexBump.mMapMode)
		{
			int i = (int)oldMat.sTexBump.mMapMode;
			mat.AddProperty<int>(&i,1,AI_MATKEY_MAPPINGMODE_U_HEIGHT(0));
			mat.AddProperty<int>(&i,1,AI_MATKEY_MAPPINGMODE_V_HEIGHT(0));
		}
	}
	if( oldMat.sTexShininess.mMapName.length() > 0)
	{
		aiString tex;
		tex.Set( oldMat.sTexShininess.mMapName);
		mat.AddProperty( &tex, AI_MATKEY_TEXTURE_SHININESS(0));

		if (is_not_qnan(oldMat.sTexShininess.mTextureBlend))
			mat.AddProperty<float>( &oldMat.sTexShininess.mTextureBlend, 1, AI_MATKEY_TEXBLEND_SHININESS(0));
		if (aiTextureMapMode_Clamp != oldMat.sTexShininess.mMapMode)
		{
			int i = (int)oldMat.sTexShininess.mMapMode;
			mat.AddProperty<int>(&i,1,AI_MATKEY_MAPPINGMODE_U_SHININESS(0));
			mat.AddProperty<int>(&i,1,AI_MATKEY_MAPPINGMODE_V_SHININESS(0));
		}
	}

	// store the name of the material itself, too
	if( oldMat.mName.length() > 0)
	{
		aiString tex;
		tex.Set( oldMat.mName);
		mat.AddProperty( &tex, AI_MATKEY_NAME);
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::ConvertMeshes(aiScene* pcOut)
{
	std::vector<aiMesh*> avOutMeshes;
	avOutMeshes.reserve(this->mScene->mMeshes.size() * 2);

	unsigned int iFaceCnt = 0;

	// we need to split all meshes by their materials
	for (std::vector<Dot3DS::Mesh>::iterator
		i =  this->mScene->mMeshes.begin();
		i != this->mScene->mMeshes.end();++i)
	{
		std::vector<unsigned int>* aiSplit = new std::vector<unsigned int>[
			this->mScene->mMaterials.size()];

		unsigned int iNum = 0;
		for (std::vector<unsigned int>::const_iterator
			a =  (*i).mFaceMaterials.begin();
			a != (*i).mFaceMaterials.end();++a,++iNum)
		{
			// check range
			if ((*a) >= this->mScene->mMaterials.size())
			{
				DefaultLogger::get()->error("3DS face material index is out of range");

				// use the last material instead
				aiSplit[this->mScene->mMaterials.size()-1].push_back(iNum);
			}
		else aiSplit[*a].push_back(iNum);
		}
		// now generate submeshes
		bool bFirst = true;
		for (unsigned int p = 0; p < this->mScene->mMaterials.size();++p)
		{
			if (aiSplit[p].size() != 0)
			{
				aiMesh* p_pcOut = new aiMesh();

				// be sure to setup the correct material index
				p_pcOut->mMaterialIndex = p;

				// use the color data as temporary storage
				p_pcOut->mColors[0] = (aiColor4D*)(&*i);
				avOutMeshes.push_back(p_pcOut);
				
				// convert vertices
				p_pcOut->mNumVertices = (unsigned int)aiSplit[p].size()*3;
				p_pcOut->mNumFaces = (unsigned int)aiSplit[p].size();

				// allocate enough storage for faces
				p_pcOut->mFaces = new aiFace[p_pcOut->mNumFaces];
				iFaceCnt += p_pcOut->mNumFaces;

				if (p_pcOut->mNumVertices != 0)
				{
					p_pcOut->mVertices = new aiVector3D[p_pcOut->mNumVertices];
					p_pcOut->mNormals = new aiVector3D[p_pcOut->mNumVertices];
					unsigned int iBase = 0;

					for (unsigned int q = 0; q < aiSplit[p].size();++q)
					{
						unsigned int iIndex = aiSplit[p][q];

						p_pcOut->mFaces[q].mIndices = new unsigned int[3];
						p_pcOut->mFaces[q].mNumIndices = 3;

						p_pcOut->mFaces[q].mIndices[2] = iBase;
						p_pcOut->mVertices[iBase] = (*i).mPositions[(*i).mFaces[iIndex].mIndices[0]];
						p_pcOut->mNormals[iBase++] = (*i).mNormals[(*i).mFaces[iIndex].mIndices[0]];

						p_pcOut->mFaces[q].mIndices[1] = iBase;
						p_pcOut->mVertices[iBase] = (*i).mPositions[(*i).mFaces[iIndex].mIndices[1]];
						p_pcOut->mNormals[iBase++] = (*i).mNormals[(*i).mFaces[iIndex].mIndices[1]];

						p_pcOut->mFaces[q].mIndices[0] = iBase;
						p_pcOut->mVertices[iBase] = (*i).mPositions[(*i).mFaces[iIndex].mIndices[2]];
						p_pcOut->mNormals[iBase++] = (*i).mNormals[(*i).mFaces[iIndex].mIndices[2]];
					}
				}
				// convert texture coordinates
				if ((*i).mTexCoords.size() != 0)
				{
					p_pcOut->mTextureCoords[0] = new aiVector3D[p_pcOut->mNumVertices];

					unsigned int iBase = 0;
					for (unsigned int q = 0; q < aiSplit[p].size();++q)
					{
						unsigned int iIndex2 = aiSplit[p][q];

						unsigned int iIndex = (*i).mFaces[iIndex2].mIndices[0];
						aiVector2D& pc = (*i).mTexCoords[iIndex];
						p_pcOut->mTextureCoords[0][iBase++] = aiVector3D(pc.x,pc.y,0.0f);

						iIndex = (*i).mFaces[iIndex2].mIndices[1];
						pc = (*i).mTexCoords[iIndex];
						p_pcOut->mTextureCoords[0][iBase++] = aiVector3D(pc.x,pc.y,0.0f);

						iIndex = (*i).mFaces[iIndex2].mIndices[2];
						pc = (*i).mTexCoords[iIndex];
						p_pcOut->mTextureCoords[0][iBase++] = aiVector3D(pc.x,pc.y,0.0f);
					}
					// apply texture coordinate scalings
					TextureTransform::BakeScaleNOffset ( p_pcOut, &this->mScene->mMaterials[
						p_pcOut->mMaterialIndex] );
				}
			}
		}
		delete[] aiSplit;
	}
	pcOut->mNumMeshes = (unsigned int)avOutMeshes.size();
	pcOut->mMeshes = new aiMesh*[pcOut->mNumMeshes]();
	for (unsigned int a = 0; a < pcOut->mNumMeshes;++a)
	{
		pcOut->mMeshes[a] = avOutMeshes[a];
	}

	if (0 == iFaceCnt)
	{
		throw new ImportErrorException("No faces loaded. The mesh is empty");
	}

	// for each material in the scene we need to setup the UV source
	// set for each texture
	for (unsigned int a = 0; a < pcOut->mNumMaterials;++a)
	{
		TextureTransform::SetupMatUVSrc( pcOut->mMaterials[a], &this->mScene->mMaterials[a] );
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::AddNodeToGraph(aiScene* pcSOut,aiNode* pcOut,Dot3DS::Node* pcIn)
{
	std::vector<unsigned int> iArray;
	iArray.reserve(3);

	if (pcIn->mName != "$$$DUMMY")
	{		
		for (unsigned int a = 0; a < pcSOut->mNumMeshes;++a)
		{
			const Dot3DS::Mesh* pcMesh = (const Dot3DS::Mesh*)pcSOut->mMeshes[a]->mColors[0];
			ai_assert(NULL != pcMesh);

			if (0 == ASSIMP_stricmp(pcIn->mName.c_str(),pcMesh->mName.c_str()))
			{
				iArray.push_back(a);
			}
		}
		if (!iArray.empty())
		{
			aiMatrix4x4& mTrafo = ((Dot3DS::Mesh*)pcSOut->mMeshes[iArray[0]]->mColors[0])->mMat;
			aiMatrix4x4 mInv = mTrafo;
			mInv.Inverse();

			pcOut->mName.Set(pcIn->mName);
			pcOut->mNumMeshes = (unsigned int)iArray.size();
			pcOut->mMeshes = new unsigned int[iArray.size()];
			for (unsigned int i = 0;i < iArray.size();++i)
			{
				const unsigned int iIndex = iArray[i];
				aiMesh* const mesh = pcSOut->mMeshes[iIndex];

				// http://www.zfx.info/DisplayThread.php?MID=235690#235690
				const aiVector3D& pivot = pcIn->vPivot;
				const aiVector3D* const pvEnd = mesh->mVertices+mesh->mNumVertices;
				aiVector3D* pvCurrent = mesh->mVertices;

				if(pivot.x || pivot.y || pivot.z)
				{
					while (pvCurrent != pvEnd)
					{
						*pvCurrent = mInv * (*pvCurrent);
						pvCurrent->x -= pivot.x;
						pvCurrent->y -= pivot.y;
						pvCurrent->z -= pivot.z;
						*pvCurrent = mTrafo * (*pvCurrent);
						std::swap( pvCurrent->y, pvCurrent->z );
						++pvCurrent;
					}
				}
				else
				{
					while (pvCurrent != pvEnd)
					{
						std::swap( pvCurrent->y, pvCurrent->z );
						//pvCurrent->y *= -1.0f;
						++pvCurrent;
					}
				}
				pcOut->mMeshes[i] = iIndex;
			}
		}
		/*else
		{
			DefaultLogger::get()->warn("A node that is not a dummy does not "
				"reference a valid mesh.");
		}*/
	}
	pcOut->mTransformation = aiMatrix4x4(); 
	pcOut->mNumChildren = (unsigned int)pcIn->mChildren.size();
	pcOut->mChildren = new aiNode*[pcIn->mChildren.size()];
	for (unsigned int i = 0; i < pcIn->mChildren.size();++i)
	{
		pcOut->mChildren[i] = new aiNode();
		pcOut->mChildren[i]->mParent = pcOut;
		AddNodeToGraph(pcSOut,pcOut->mChildren[i],
			pcIn->mChildren[i]);
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::GenerateNodeGraph(aiScene* pcOut)
{
	pcOut->mRootNode = new aiNode();

	if (0 == this->mRootNode->mChildren.size())
		{
		// seems the file has not even a hierarchy.
		// generate a flat hiearachy which looks like this:
		//
		//                ROOT_NODE
		//                   |
		//   ----------------------------------------
		//   |       |       |            |
		// MESH_0  MESH_1  MESH_2  ...  MESH_N
		//
		unsigned int iCnt = 0;

		DefaultLogger::get()->warn("No hierarchy information has been "
			"found in the file. A flat hierarchy tree is built ...");

		pcOut->mRootNode->mNumChildren = pcOut->mNumMeshes;
		pcOut->mRootNode->mChildren = new aiNode* [ pcOut->mNumMeshes ];

		for (unsigned int i = 0; i < pcOut->mNumMeshes;++i)
			{
			aiNode* pcNode = new aiNode();
			pcNode->mParent = pcOut->mRootNode;
			pcNode->mNumChildren = 0;
			pcNode->mChildren = 0;
			pcNode->mMeshes = new unsigned int[1];
			pcNode->mMeshes[0] = i;
			pcNode->mNumMeshes = 1;

			char szBuffer[128];
			int iLen;
#if _MSC_VER >= 1400
			iLen = sprintf_s(szBuffer,"UNNAMED_%i",i);
#else
			iLen = sprintf(szBuffer,"UNNAMED_%i",i);
#endif
			ai_assert(0 < iLen);
			::memcpy(pcNode->mName.data,szBuffer,iLen);
			pcNode->mName.data[iLen] = '\0';
			pcNode->mName.length = iLen;

			// add the new child to the parent node
			pcOut->mRootNode->mChildren[i] = pcNode;
			}
		}
	else this->AddNodeToGraph(pcOut,  pcOut->mRootNode, this->mRootNode);

	for (unsigned int a = 0; a < pcOut->mNumMeshes;++a)
		pcOut->mMeshes[a]->mColors[0] = NULL;

	// if the root node has only one child ... set the child as root node
	if (1 == pcOut->mRootNode->mNumChildren)
	{
		aiNode* pcOld = pcOut->mRootNode;
		pcOut->mRootNode = pcOut->mRootNode->mChildren[0];
		pcOut->mRootNode->mParent = NULL;
		pcOld->mChildren[0] = NULL;
		delete pcOld;
	}

	// if the root node is a default node setup a name for it
	if (pcOut->mRootNode->mName.data[0] == '$' && pcOut->mRootNode->mName.data[1] == '$')
	{
		pcOut->mRootNode->mName.Set("<root>");
	}
}
// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::ConvertScene(aiScene* pcOut)
{
	pcOut->mNumMaterials = (unsigned int)this->mScene->mMaterials.size();
	pcOut->mMaterials = new aiMaterial*[pcOut->mNumMaterials];

	for (unsigned int i = 0; i < pcOut->mNumMaterials;++i)
	{
		MaterialHelper* pcNew = new MaterialHelper();
		this->ConvertMaterial(this->mScene->mMaterials[i],*pcNew);
		pcOut->mMaterials[i] = pcNew;
	}
	this->ConvertMeshes(pcOut);
	return;
}
