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


#include "AssimpPCH.h"

// internal headers
#include "3DSLoader.h"
#include "TextureTransform.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Setup final material indices, generae a default material if necessary
void Discreet3DSImporter::ReplaceDefaultMaterial()
{
	// *******************************************************************
	// try to find an existing material that matches the
	// typical default material setting:
	// - no textures
	// - diffuse color (in grey!)
	// NOTE: This is here to workaround the fact that some
	// exporters are writing a default material, too.
	// *******************************************************************
	unsigned int idx = 0xcdcdcdcd;
	for (unsigned int i = 0; i < mScene->mMaterials.size();++i)
	{
		std::string s = mScene->mMaterials[i].mName;
		for (std::string::iterator it = s.begin(); it != s.end(); ++it)
			*it = ::tolower(*it);

		if (std::string::npos == s.find("default"))continue;

		if (mScene->mMaterials[i].mDiffuse.r !=
			mScene->mMaterials[i].mDiffuse.g ||
			mScene->mMaterials[i].mDiffuse.r !=
			mScene->mMaterials[i].mDiffuse.b)continue;

		if (mScene->mMaterials[i].sTexDiffuse.mMapName.length()   != 0	||
			mScene->mMaterials[i].sTexBump.mMapName.length()      != 0	|| 
			mScene->mMaterials[i].sTexOpacity.mMapName.length()   != 0	||
			mScene->mMaterials[i].sTexEmissive.mMapName.length()  != 0	||
			mScene->mMaterials[i].sTexSpecular.mMapName.length()  != 0	||
			mScene->mMaterials[i].sTexShininess.mMapName.length() != 0 )
		{
			continue;
		}
		idx = i;
	}
	if (0xcdcdcdcd == idx)idx = (unsigned int)mScene->mMaterials.size();

	// now iterate through all meshes and through all faces and
	// find all faces that are using the default material
	unsigned int cnt = 0;
	for (std::vector<D3DS::Mesh>::iterator
		i =  mScene->mMeshes.begin();
		i != mScene->mMeshes.end();++i)
	{
		for (std::vector<unsigned int>::iterator
			a =  (*i).mFaceMaterials.begin();
			a != (*i).mFaceMaterials.end();++a)
		{
			// NOTE: The additional check seems to be necessary,
			// some exporters seem to generate invalid data here
			if (0xcdcdcdcd == (*a))
			{
				(*a) = idx;
				++cnt;
			}
			else if ( (*a) >= mScene->mMaterials.size())
			{
				(*a) = idx;
				DefaultLogger::get()->warn("Material index overflow in 3DS file. Using default material");
				++cnt;
			}
		}
	}
	if (cnt && idx == mScene->mMaterials.size())
	{
		// We need to create our own default material
		D3DS::Material sMat;
		sMat.mDiffuse = aiColor3D(0.3f,0.3f,0.3f);
		sMat.mName = "%%%DEFAULT";
		mScene->mMaterials.push_back(sMat);

		DefaultLogger::get()->info("3DS: Generating default material");
	}
	return;
}

// ------------------------------------------------------------------------------------------------
// Check whether all indices are valid. Otherwise we'd crash before the validation step was reached
void Discreet3DSImporter::CheckIndices(D3DS::Mesh& sMesh)
{
	for (std::vector< D3DS::Face >::iterator
		 i =  sMesh.mFaces.begin();
		 i != sMesh.mFaces.end();++i)
	{
		// check whether all indices are in range
		for (unsigned int a = 0; a < 3;++a)
		{
			if ((*i).mIndices[a] >= sMesh.mPositions.size())
			{
				DefaultLogger::get()->warn("3DS: Vertex index overflow)");
				(*i).mIndices[a] = (uint32_t)sMesh.mPositions.size()-1;
			}
			if ( !sMesh.mTexCoords.empty() && (*i).mIndices[a] >= sMesh.mTexCoords.size())
			{
				DefaultLogger::get()->warn("3DS: Texture coordinate index overflow)");
				(*i).mIndices[a] = (uint32_t)sMesh.mTexCoords.size()-1;
			}
		}
	}
	return;
}

// ------------------------------------------------------------------------------------------------
// Generate out unique verbose format representation
void Discreet3DSImporter::MakeUnique(D3DS::Mesh& sMesh)
{
	unsigned int iBase = 0;

	// Allocate output storage
	std::vector<aiVector3D> vNew  (sMesh.mFaces.size() * 3);
	std::vector<aiVector2D> vNew2;
	if (sMesh.mTexCoords.size())vNew2.resize(sMesh.mFaces.size() * 3);

	for (unsigned int i = 0; i < sMesh.mFaces.size();++i)
	{
		uint32_t iTemp1,iTemp2;

		// positions
		vNew[iBase]   = sMesh.mPositions[sMesh.mFaces[i].mIndices[2]];
		iTemp1 = iBase++;
		vNew[iBase]   = sMesh.mPositions[sMesh.mFaces[i].mIndices[1]];
		iTemp2 = iBase++;
		vNew[iBase]   = sMesh.mPositions[sMesh.mFaces[i].mIndices[0]];

		// texture coordinates
		if (sMesh.mTexCoords.size())
		{
			vNew2[iTemp1]   = sMesh.mTexCoords[sMesh.mFaces[i].mIndices[2]];
			vNew2[iTemp2]   = sMesh.mTexCoords[sMesh.mFaces[i].mIndices[1]];
			vNew2[iBase]    = sMesh.mTexCoords[sMesh.mFaces[i].mIndices[0]];
		}

		sMesh.mFaces[i].mIndices[2] = iBase++;
		sMesh.mFaces[i].mIndices[0] = iTemp1;
		sMesh.mFaces[i].mIndices[1] = iTemp2;
	}
	sMesh.mPositions = vNew;
	sMesh.mTexCoords = vNew2;
	return;
}

// ------------------------------------------------------------------------------------------------
// Convert a 3DS material to an aiMaterial
void Discreet3DSImporter::ConvertMaterial(D3DS::Material& oldMat,
	MaterialHelper& mat)
{
	// NOTE: Pass the background image to the viewer by bypassing the
	// material system. This is an evil hack, never do it again!
	if (0 != mBackgroundImage.length() && bHasBG)
	{
		aiString tex;
		tex.Set( mBackgroundImage);
		mat.AddProperty( &tex, AI_MATKEY_GLOBAL_BACKGROUND_IMAGE);

		// be sure this is only done for the first material
		mBackgroundImage = std::string("");
	}

	// At first add the base ambient color of the
	// scene to	the material
	oldMat.mAmbient.r += mClrAmbient.r;
	oldMat.mAmbient.g += mClrAmbient.g;
	oldMat.mAmbient.b += mClrAmbient.b;

	aiString name;
	name.Set( oldMat.mName);
	mat.AddProperty( &name, AI_MATKEY_NAME);

	// material colors
	mat.AddProperty( &oldMat.mAmbient, 1, AI_MATKEY_COLOR_AMBIENT);
	mat.AddProperty( &oldMat.mDiffuse, 1, AI_MATKEY_COLOR_DIFFUSE);
	mat.AddProperty( &oldMat.mSpecular, 1, AI_MATKEY_COLOR_SPECULAR);
	mat.AddProperty( &oldMat.mEmissive, 1, AI_MATKEY_COLOR_EMISSIVE);

	// phong shininess and shininess strength
	if (D3DS::Discreet3DS::Phong == oldMat.mShading || 
		D3DS::Discreet3DS::Metal == oldMat.mShading)
	{
		if (!oldMat.mSpecularExponent || !oldMat.mShininessStrength)
		{
			oldMat.mShading = D3DS::Discreet3DS::Gouraud;
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
		int i = 1;
		mat.AddProperty<int>(&i,1,AI_MATKEY_TWOSIDED);
	}

	// shading mode
	aiShadingMode eShading = aiShadingMode_NoShading;
	switch (oldMat.mShading)
	{
		case D3DS::Discreet3DS::Flat:
			eShading = aiShadingMode_Flat; break;

		// I don't know what "Wire" shading should be,
		// assume it is simple lambertian diffuse (L dot N) shading
		case D3DS::Discreet3DS::Wire:
		case D3DS::Discreet3DS::Gouraud:
			eShading = aiShadingMode_Gouraud; break;

		// assume cook-torrance shading for metals.
		case D3DS::Discreet3DS::Phong :
			eShading = aiShadingMode_Phong; break;

		case D3DS::Discreet3DS::Metal :
			eShading = aiShadingMode_CookTorrance; break;

			// FIX to workaround a warning with GCC 4 who complained
			// about a missing case Blinn: here - Blinn isn't a valid
			// value in the 3DS Loader, it is just needed for ASE
		case D3DS::Discreet3DS::Blinn :
			eShading = aiShadingMode_Blinn; break;
	}
	mat.AddProperty<int>( (int*)&eShading,1,AI_MATKEY_SHADING_MODEL);

	if (D3DS::Discreet3DS::Wire == oldMat.mShading)
	{
		// set the wireframe flag
		unsigned int iWire = 1;
		mat.AddProperty<int>( (int*)&iWire,1,AI_MATKEY_ENABLE_WIREFRAME);
	}

	// texture, if there is one
	// DIFFUSE texture
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
	// SPECULAR texture
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
	// OPACITY texture
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
	// EMISSIVE texture
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
	// BUMP texturee
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
	// SHININESS texture
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

	// Store the name of the material itself, too
	if( oldMat.mName.length())
	{
		aiString tex;
		tex.Set( oldMat.mName);
		mat.AddProperty( &tex, AI_MATKEY_NAME);
	}
	return;
}

// ------------------------------------------------------------------------------------------------
// Split meshes by their materials and generate output aiMesh'es
void Discreet3DSImporter::ConvertMeshes(aiScene* pcOut)
{
	std::vector<aiMesh*> avOutMeshes;
	avOutMeshes.reserve(mScene->mMeshes.size() * 2);

	unsigned int iFaceCnt = 0;

	// we need to split all meshes by their materials
	for (std::vector<D3DS::Mesh>::iterator i =  mScene->mMeshes.begin();
		i != mScene->mMeshes.end();++i)
	{
		std::vector<unsigned int>* aiSplit = new std::vector<unsigned int>[
			mScene->mMaterials.size()];

		unsigned int iNum = 0;
		for (std::vector<unsigned int>::const_iterator a =  (*i).mFaceMaterials.begin();
			a != (*i).mFaceMaterials.end();++a,++iNum)
		{
			aiSplit[*a].push_back(iNum);
		}
		// now generate submeshes
		for (unsigned int p = 0; p < mScene->mMaterials.size();++p)
		{
			if (aiSplit[p].size() != 0)
			{
				aiMesh* p_pcOut = new aiMesh();
				p_pcOut->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

				// be sure to setup the correct material index
				p_pcOut->mMaterialIndex = p;

				// use the color data as temporary storage
				p_pcOut->mColors[0] = (aiColor4D*)(&*i);
				avOutMeshes.push_back(p_pcOut);
				
				// convert vertices
				p_pcOut->mNumFaces = (unsigned int)aiSplit[p].size();
				p_pcOut->mNumVertices = p_pcOut->mNumFaces*3;

				// allocate enough storage for faces
				p_pcOut->mFaces = new aiFace[p_pcOut->mNumFaces];
				iFaceCnt += p_pcOut->mNumFaces;

				if (p_pcOut->mNumVertices)
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
						p_pcOut->mVertices[iBase]  = (*i).mPositions[(*i).mFaces[iIndex].mIndices[0]];
						p_pcOut->mNormals[iBase++] = (*i).mNormals[(*i).mFaces[iIndex].mIndices[0]];

						p_pcOut->mFaces[q].mIndices[1] = iBase;
						p_pcOut->mVertices[iBase]  = (*i).mPositions[(*i).mFaces[iIndex].mIndices[1]];
						p_pcOut->mNormals[iBase++] = (*i).mNormals[(*i).mFaces[iIndex].mIndices[1]];

						p_pcOut->mFaces[q].mIndices[0] = iBase;
						p_pcOut->mVertices[iBase]  = (*i).mPositions[(*i).mFaces[iIndex].mIndices[2]];
						p_pcOut->mNormals[iBase++] = (*i).mNormals[(*i).mFaces[iIndex].mIndices[2]];
					}
				}
				// convert texture coordinates
				if ((*i).mTexCoords.size())
				{
					p_pcOut->mTextureCoords[0] = new aiVector3D[p_pcOut->mNumVertices];

					unsigned int iBase = 0;
					for (unsigned int q = 0; q < aiSplit[p].size();++q)
					{
						unsigned int iIndex2 = aiSplit[p][q];

						aiVector2D* pc = &(*i).mTexCoords[(*i).mFaces[iIndex2].mIndices[0]];
						p_pcOut->mTextureCoords[0][iBase++] = aiVector3D(pc->x,pc->y,0.0f);

						pc = &(*i).mTexCoords[(*i).mFaces[iIndex2].mIndices[1]];
						p_pcOut->mTextureCoords[0][iBase++] = aiVector3D(pc->x,pc->y,0.0f);

						pc = &(*i).mTexCoords[(*i).mFaces[iIndex2].mIndices[2]];
						p_pcOut->mTextureCoords[0][iBase++] = aiVector3D(pc->x,pc->y,0.0f);
					}
					// apply texture coordinate scalings
					TextureTransform::BakeScaleNOffset ( p_pcOut, &mScene->mMaterials[
						p_pcOut->mMaterialIndex] );
				}
			}
		}
		delete[] aiSplit;
	}

	// Copy them to the output array
	pcOut->mNumMeshes = (unsigned int)avOutMeshes.size();
	pcOut->mMeshes = new aiMesh*[pcOut->mNumMeshes]();
	for (unsigned int a = 0; a < pcOut->mNumMeshes;++a)
		pcOut->mMeshes[a] = avOutMeshes[a];

	// We should have at least one face here
	if (!iFaceCnt)
		throw new ImportErrorException("No faces loaded. The mesh is empty");

	// for each material in the scene we need to setup the UV source
	// set for each texture
	for (unsigned int a = 0; a < pcOut->mNumMaterials;++a)
	{
		TextureTransform::SetupMatUVSrc( pcOut->mMaterials[a], &mScene->mMaterials[a] );
	}
	return;
}

// ------------------------------------------------------------------------------------------------
// Add a node to the scenegraph and setup its final transformation
void Discreet3DSImporter::AddNodeToGraph(aiScene* pcSOut,aiNode* pcOut,D3DS::Node* pcIn)
{
	std::vector<unsigned int> iArray;
	iArray.reserve(3);

	if (pcIn->mName == "$$$DUMMY")
	{
		// append the "real" name of the dummy to the string
		pcIn->mName.append(pcIn->mDummyName);
	}
	else // if (pcIn->mName != "$$$DUMMY")
	{		
		for (unsigned int a = 0; a < pcSOut->mNumMeshes;++a)
		{
			const D3DS::Mesh* pcMesh = (const D3DS::Mesh*)pcSOut->mMeshes[a]->mColors[0];
			ai_assert(NULL != pcMesh);

			// do case independent comparisons here, just for safety
			if (!ASSIMP_stricmp(pcIn->mName,pcMesh->mName))
				iArray.push_back(a);
		}
		if (!iArray.empty())
		{
			aiMatrix4x4& mTrafo = ((D3DS::Mesh*)pcSOut->mMeshes[iArray[0]]->mColors[0])->mMat;
			aiMatrix4x4 mInv = mTrafo;
			if (!configSkipPivot)
				mInv.Inverse();

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

				if(pivot.x || pivot.y || pivot.z && !configSkipPivot)
				{
					while (pvCurrent != pvEnd)
					{
						*pvCurrent = mInv * (*pvCurrent);
						pvCurrent->x -= pivot.x;
						pvCurrent->y -= pivot.y;
						pvCurrent->z -= pivot.z;
						*pvCurrent = mTrafo * (*pvCurrent);
						//std::swap( pvCurrent->y, pvCurrent->z );
						++pvCurrent;
					}
				}
#if 0
				else
				{
					while (pvCurrent != pvEnd)
					{
						std::swap( pvCurrent->y, pvCurrent->z );
						++pvCurrent;
					}
				}
#endif
				pcOut->mMeshes[i] = iIndex;
			}
		}
	}

	// Setup the name of the node
	pcOut->mName.Set(pcIn->mName);

	// Setup the transformation matrix of the node
	pcOut->mTransformation = aiMatrix4x4(); 

	// Allocate storage for children
	pcOut->mNumChildren = (unsigned int)pcIn->mChildren.size();
	pcOut->mChildren = new aiNode*[pcIn->mChildren.size()];

	// Recursively process all children
	for (unsigned int i = 0; i < pcIn->mChildren.size();++i)
	{
		pcOut->mChildren[i] = new aiNode();
		pcOut->mChildren[i]->mParent = pcOut;
		AddNodeToGraph(pcSOut,pcOut->mChildren[i],pcIn->mChildren[i]);
	}
	return;
}

// ------------------------------------------------------------------------------------------------
// Generate the output node graph
void Discreet3DSImporter::GenerateNodeGraph(aiScene* pcOut)
{
	pcOut->mRootNode = new aiNode();

	if (0 == mRootNode->mChildren.size())
	{
		// seems the file has not even a hierarchy.
		// generate a flat hiearachy which looks like this:
		//
		//                ROOT_NODE
		//                   |
		//   ----------------------------------------
		//   |       |       |            |         |  
		// MESH_0  MESH_1  MESH_2  ...  MESH_N    CAMERA_0 ....
		//
		DefaultLogger::get()->warn("No hierarchy information has been found in the file. ");

		pcOut->mRootNode->mNumChildren = pcOut->mNumMeshes + 
			mScene->mCameras.size() + mScene->mLights.size();

		pcOut->mRootNode->mChildren = new aiNode* [ pcOut->mRootNode->mNumChildren ];
		pcOut->mRootNode->mName.Set("<3DSDummyRoot>");

		// Build dummy nodes for all meshes
		unsigned int a = 0;
		for (unsigned int i = 0; i < pcOut->mNumMeshes;++i,++a)
		{
			aiNode* pcNode = pcOut->mRootNode->mChildren[a] = new aiNode();
			pcNode->mParent = pcOut->mRootNode;
			pcNode->mMeshes = new unsigned int[1];
			pcNode->mMeshes[0] = i;
			pcNode->mNumMeshes = 1;

			// Build a name for the node
			pcNode->mName.length = sprintf(pcNode->mName.data,"3DSMesh_%i",i);	
		}

		// Build dummy nodes for all cameras
		for (unsigned int i = 0; i < (unsigned int )mScene->mCameras.size();++i,++a)
		{
			aiNode* pcNode = pcOut->mRootNode->mChildren[a] = new aiNode();
			pcNode->mParent = pcOut->mRootNode;

			// Build a name for the node
			pcNode->mName = mScene->mCameras[i]->mName;
		}

		// Build dummy nodes for all lights
		for (unsigned int i = 0; i < (unsigned int )mScene->mLights.size();++i,++a)
		{
			aiNode* pcNode = pcOut->mRootNode->mChildren[a] = new aiNode();
			pcNode->mParent = pcOut->mRootNode;

			// Build a name for the node
			pcNode->mName = mScene->mLights[i]->mName;
		}
	}
	else AddNodeToGraph(pcOut,  pcOut->mRootNode, mRootNode);

	// We used the first vertex color set to store some
	// temporary values, so we need to cleanup here
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
		pcOut->mRootNode->mName.Set("<root>");

	// modify the transformation of the root node to change
	// the coordinate system of the whole scene from Max' to OpenGL
	pcOut->mRootNode->mTransformation.a3 *= -1.f;
	pcOut->mRootNode->mTransformation.b3 *= -1.f;
	pcOut->mRootNode->mTransformation.c3 *= -1.f;
}

// ------------------------------------------------------------------------------------------------
// Convert all meshes in the scene and generate the final output scene.
void Discreet3DSImporter::ConvertScene(aiScene* pcOut)
{
	// Allocate enough storage for all output materials
	pcOut->mNumMaterials = (unsigned int)mScene->mMaterials.size();
	pcOut->mMaterials    = new aiMaterial*[pcOut->mNumMaterials];

	//  ... and convert the 3DS materials to aiMaterial's
	for (unsigned int i = 0; i < pcOut->mNumMaterials;++i)
	{
		MaterialHelper* pcNew = new MaterialHelper();
		ConvertMaterial(mScene->mMaterials[i],*pcNew);
		pcOut->mMaterials[i] = pcNew;
	}

	// Generate the output mesh list
	ConvertMeshes(pcOut);

	// Now copy all light sources to the output scene
	pcOut->mNumLights = (unsigned int)mScene->mLights.size();
	if (pcOut->mNumLights)
	{
		pcOut->mLights = new aiLight*[pcOut->mNumLights];
		::memcpy(pcOut->mLights,&mScene->mLights[0],sizeof(void*)*pcOut->mNumLights);
	}

	// Now copy all cameras to the output scene
	pcOut->mNumCameras = (unsigned int)mScene->mCameras.size();
	if (pcOut->mNumCameras)
	{
		pcOut->mCameras = new aiCamera*[pcOut->mNumCameras];
		::memcpy(pcOut->mCameras,&mScene->mCameras[0],sizeof(void*)*pcOut->mNumCameras);
	}

	return;
}
