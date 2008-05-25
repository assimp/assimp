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

/** @file Implementation of the MDL importer class */

#include "MaterialSystem.h"
#include "MDLLoader.h"
#include "MDLDefaultColorMap.h"
#include "DefaultLogger.h"

#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"
#include "../include/aiAssert.h"

#include <boost/scoped_ptr.hpp>

using namespace Assimp;

extern float g_avNormals[162][3];


// ------------------------------------------------------------------------------------------------
inline bool is_qnan(float p_fIn)
{
	// NOTE: Comparison against qnan is generally problematic
	// because qnan == qnan is false AFAIK
	union FTOINT
	{
		float fFloat;
		int32_t iInt;
	} one, two;
	one.fFloat = std::numeric_limits<float>::quiet_NaN();
	two.fFloat = p_fIn;

	return (one.iInt == two.iInt);
}
// ------------------------------------------------------------------------------------------------
inline bool is_not_qnan(float p_fIn)
{
	return !is_qnan(p_fIn);
}

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
MDLImporter::MDLImporter()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
MDLImporter::~MDLImporter()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool MDLImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
{
	// simple check of file extension is enough for the moment
	std::string::size_type pos = pFile.find_last_of('.');
	// no file extension - can't read
	if( pos == std::string::npos)
		return false;
	std::string extension = pFile.substr( pos);

	if (extension.length() < 4)return false;
	if (extension[0] != '.')return false;
	if (extension[1] != 'm' && extension[1] != 'M')return false;
	if (extension[2] != 'd' && extension[2] != 'D')return false;
	if (extension[3] != 'l' && extension[3] != 'L')return false;

	return true;
}
// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void MDLImporter::InternReadFile( 
	const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile));

	// Check whether we can read from the file
	if( file.get() == NULL)
	{
		throw new ImportErrorException( "Failed to open MDL file " + pFile + ".");
	}

	// check whether the ply file is large enough to contain
	// at least the file header
	size_t fileSize = file->FileSize();
	if( fileSize < sizeof(MDL::Header))
	{
		throw new ImportErrorException( ".mdl File is too small.");
	}

	// allocate storage and copy the contents of the file to a memory buffer
	this->pScene = pScene;
	this->pIOHandler = pIOHandler;
	this->mBuffer = new unsigned char[fileSize+1];
	file->Read( (void*)mBuffer, 1, fileSize);

	// determine the file subtype and call the appropriate member function

	// Original Quake1 format
	this->m_pcHeader = (const MDL::Header*)this->mBuffer;
	if (AI_MDL_MAGIC_NUMBER_BE == this->m_pcHeader->ident ||
		AI_MDL_MAGIC_NUMBER_LE == this->m_pcHeader->ident)
	{
		DefaultLogger::get()->debug("MDL subtype: Quake 1, magic word is IDPO");
		this->InternReadFile_Quake1();
	}
	// GameStudio A4 MDL3 format
	else if (AI_MDL_MAGIC_NUMBER_BE_GS4 == this->m_pcHeader->ident ||
			 AI_MDL_MAGIC_NUMBER_LE_GS4 == this->m_pcHeader->ident)
	{
		DefaultLogger::get()->debug("MDL subtype: 3D GameStudio A4, magic word is MDL3");
		this->iGSFileVersion = 3;
		this->InternReadFile_GameStudio();
	}
	// GameStudio A5+ MDL4 format
	else if (AI_MDL_MAGIC_NUMBER_BE_GS5a == this->m_pcHeader->ident ||
			 AI_MDL_MAGIC_NUMBER_LE_GS5a == this->m_pcHeader->ident)
	{
		DefaultLogger::get()->debug("MDL subtype: 3D GameStudio A4, magic word is MDL4");
		this->iGSFileVersion = 4;
		this->InternReadFile_GameStudio();
	}
	// GameStudio A5+ MDL5 format
	else if (AI_MDL_MAGIC_NUMBER_BE_GS5b == this->m_pcHeader->ident ||
			 AI_MDL_MAGIC_NUMBER_LE_GS5b == this->m_pcHeader->ident)
	{
		DefaultLogger::get()->debug("MDL subtype: 3D GameStudio A5, magic word is MDL5");
		this->iGSFileVersion = 5;
		this->InternReadFile_GameStudio();
	}
	// GameStudio A6+ MDL6 format
	else if (AI_MDL_MAGIC_NUMBER_BE_GS6 == this->m_pcHeader->ident ||
			 AI_MDL_MAGIC_NUMBER_LE_GS6 == this->m_pcHeader->ident)
	{
		DefaultLogger::get()->debug("MDL subtype: 3D GameStudio A6, magic word is MDL6");
		this->iGSFileVersion = 6;
		this->InternReadFile_GameStudio();
	}
	// GameStudio A7 MDL7 format
	else if (AI_MDL_MAGIC_NUMBER_BE_GS7 == this->m_pcHeader->ident ||
			 AI_MDL_MAGIC_NUMBER_LE_GS7 == this->m_pcHeader->ident)
	{
		DefaultLogger::get()->debug("MDL subtype: 3D GameStudio A7, magic word is MDL7");
		this->iGSFileVersion = 7;
		this->InternReadFile_GameStudioA7();
	}
	// IDST/IDSQ Format (CS:S/HL², etc ...)
	else if (AI_MDL_MAGIC_NUMBER_BE_HL2a == this->m_pcHeader->ident ||
			 AI_MDL_MAGIC_NUMBER_LE_HL2a == this->m_pcHeader->ident ||
			 AI_MDL_MAGIC_NUMBER_BE_HL2b == this->m_pcHeader->ident ||
			 AI_MDL_MAGIC_NUMBER_LE_HL2b == this->m_pcHeader->ident)
	{
		DefaultLogger::get()->debug("MDL subtype: CS:S\\HL², magic word is IDST/IDSQ");
		this->InternReadFile_HL2();
	}
	else
	{
		// we're definitely unable to load this file
		throw new ImportErrorException( "Unknown MDL subformat " + pFile +
			". Magic word is not known");
	}

	// make sure that the normals are facing outwards
	for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
		this->FlipNormals(pScene->mMeshes[i]);

	// delete the file buffer
	delete[] this->mBuffer;
	return;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::SearchPalette(const unsigned char** pszColorMap)
{
	// now try to find the color map in the current directory
	IOStream* pcStream = this->pIOHandler->Open("colormap.lmp","rb");

	const unsigned char* szColorMap = (const unsigned char*)::g_aclrDefaultColorMap;
	if(pcStream)
	{
		if (pcStream->FileSize() >= 768)
		{
			szColorMap = new unsigned char[256*3];
			pcStream->Read(const_cast<unsigned char*>(szColorMap),256*3,1);

			DefaultLogger::get()->info("Found valid colormap.lmp in directory. "
				"It will be used to decode embedded textures in palletized formats.");
		}
		delete pcStream;
		pcStream = NULL;
	}
	*pszColorMap = szColorMap;
	return;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::FreePalette(const unsigned char* szColorMap)
{
	if (szColorMap != (const unsigned char*)::g_aclrDefaultColorMap)
	{
		delete[] szColorMap;
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::CreateTextureARGB8(const unsigned char* szData)
{
	// allocate a new texture object
	aiTexture* pcNew = new aiTexture();
	pcNew->mWidth = this->m_pcHeader->skinwidth;
	pcNew->mHeight = this->m_pcHeader->skinheight;

	pcNew->pcData = new aiTexel[pcNew->mWidth * pcNew->mHeight];

	const unsigned char* szColorMap;
	this->SearchPalette(&szColorMap);

	// copy texture data
	for (unsigned int i = 0; i < pcNew->mWidth*pcNew->mHeight;++i)
	{
		const unsigned char val = szData[i];
		const unsigned char* sz = &szColorMap[val*3];

		pcNew->pcData[i].a = 0xFF;
		pcNew->pcData[i].r = *sz++;
		pcNew->pcData[i].g = *sz++;
		pcNew->pcData[i].b = *sz;
	}

	this->FreePalette(szColorMap);

	// store the texture
	aiTexture** pc = this->pScene->mTextures;
	this->pScene->mTextures = new aiTexture*[this->pScene->mNumTextures+1];
	for (unsigned int i = 0; i < this->pScene->mNumTextures;++i)
		this->pScene->mTextures[i] = pc[i];

	this->pScene->mTextures[this->pScene->mNumTextures] = pcNew;
	this->pScene->mNumTextures++;
	delete[] pc;
	return;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::CreateTextureARGB8_GS4(const unsigned char* szData, 
	unsigned int iType,
	unsigned int* piSkip)
{
	ai_assert(NULL != piSkip);

	// allocate a new texture object
	aiTexture* pcNew = new aiTexture();
	pcNew->mWidth = this->m_pcHeader->skinwidth;
	pcNew->mHeight = this->m_pcHeader->skinheight;

	pcNew->pcData = new aiTexel[pcNew->mWidth * pcNew->mHeight];

	// 8 Bit paletized. Use Q1 default palette.
	if (0 == iType)
	{
		const unsigned char* szColorMap;
		this->SearchPalette(&szColorMap);

		// copy texture data
		unsigned int i = 0;
		for (; i < pcNew->mWidth*pcNew->mHeight;++i)
		{
			const unsigned char val = szData[i];
			const unsigned char* sz = &szColorMap[val*3];

			pcNew->pcData[i].a = 0xFF;
			pcNew->pcData[i].r = *sz++;
			pcNew->pcData[i].g = *sz++;
			pcNew->pcData[i].b = *sz;
		}
		*piSkip = i;

		this->FreePalette(szColorMap);
	}
	// R5G6B5 format
	else if (2 == iType)
	{
		// copy texture data
		unsigned int i = 0;
		for (i = 0; i < pcNew->mWidth*pcNew->mHeight;++i)
		{
			MDL::RGB565 val = ((MDL::RGB565*)szData)[i];

			pcNew->pcData[i].a = 0xFF;
			pcNew->pcData[i].r = (unsigned char)val.b << 3;
			pcNew->pcData[i].g = (unsigned char)val.g << 2;
			pcNew->pcData[i].b = (unsigned char)val.r << 3;
		}
		*piSkip = i * 2;
	}
	// ARGB4 format
	else if (3 == iType)
	{
		// copy texture data
		unsigned int i = 0;
		for (i = 0; i < pcNew->mWidth*pcNew->mHeight;++i)
		{
			MDL::ARGB4 val = ((MDL::ARGB4*)szData)[i];

			pcNew->pcData[i].a = (unsigned char)val.a << 4;
			pcNew->pcData[i].r = (unsigned char)val.r << 4;
			pcNew->pcData[i].g = (unsigned char)val.g << 4;
			pcNew->pcData[i].b = (unsigned char)val.b << 4;
		}
		*piSkip = i * 2;
	}
	// store the texture
	aiTexture** pc = this->pScene->mTextures;
	this->pScene->mTextures = new aiTexture*[this->pScene->mNumTextures+1];
	for (unsigned int i = 0; i < this->pScene->mNumTextures;++i)
		this->pScene->mTextures[i] = pc[i];

	this->pScene->mTextures[this->pScene->mNumTextures] = pcNew;
	this->pScene->mNumTextures++;
	delete[] pc;
	return;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::ParseTextureColorData(const unsigned char* szData, 
	unsigned int iType,
	unsigned int* piSkip,
	aiTexture* pcNew)
{
	// allocate storage for the texture image
	pcNew->pcData = new aiTexel[pcNew->mWidth * pcNew->mHeight];

	// R5G6B5 format (with or without MIPs)
	if (2 == iType || 10 == iType)
	{
		// copy texture data
		unsigned int i = 0;
		for (i = 0; i < pcNew->mWidth*pcNew->mHeight;++i)
		{
			MDL::RGB565 val = ((MDL::RGB565*)szData)[i];

			pcNew->pcData[i].a = 0xFF;
			pcNew->pcData[i].r = (unsigned char)val.b << 3;
			pcNew->pcData[i].g = (unsigned char)val.g << 2;
			pcNew->pcData[i].b = (unsigned char)val.r << 3;
		}
		*piSkip = i * 2;

		// apply MIP maps
		if (10 == iType)
		{
			*piSkip += ((i >> 2) + (i >> 4) + (i >> 6)) << 1;
		}
	}
	// ARGB4 format (with or without MIPs)
	else if (3 == iType || 11 == iType)
	{
		// copy texture data
		unsigned int i = 0;
		for (i = 0; i < pcNew->mWidth*pcNew->mHeight;++i)
		{
			MDL::ARGB4 val = ((MDL::ARGB4*)szData)[i];

			pcNew->pcData[i].a = (unsigned char)val.a << 4;
			pcNew->pcData[i].r = (unsigned char)val.r << 4;
			pcNew->pcData[i].g = (unsigned char)val.g << 4;
			pcNew->pcData[i].b = (unsigned char)val.b << 4;
		}
		*piSkip = i * 2;

		// apply MIP maps
		if (11 == iType)
		{
			*piSkip += ((i >> 2) + (i >> 4) + (i >> 6)) << 1;
		}
	}
	// RGB8 format (with or without MIPs)
	else if (4 == iType || 12 == iType)
	{
		// copy texture data
		unsigned int i = 0;
		for (i = 0; i < pcNew->mWidth*pcNew->mHeight;++i)
		{
			const unsigned char* _szData = &szData[i*3];

			pcNew->pcData[i].a = 0xFF;
			pcNew->pcData[i].b = *_szData++;
			pcNew->pcData[i].g = *_szData++;
			pcNew->pcData[i].r = *_szData;
		}
		// apply MIP maps
		*piSkip = i * 3;
		if (12 == iType)
		{
			*piSkip += ((i >> 2) + (i >> 4) + (i >> 6)) *3;
		}
	}
	// ARGB8 format (with ir without MIPs)
	else if (5 == iType || 13 == iType)
	{
		// copy texture data
		unsigned int i = 0;
		for (i = 0; i < pcNew->mWidth*pcNew->mHeight;++i)
		{
			const unsigned char* _szData = &szData[i*4];

			pcNew->pcData[i].b = *_szData++;
			pcNew->pcData[i].g = *_szData++;
			pcNew->pcData[i].r = *_szData++;
			pcNew->pcData[i].a = *_szData;
		}
		// apply MIP maps
		*piSkip = i << 2;
		if (13 == iType)
		{
			*piSkip += (i + (i >> 2) + (i >> 4) + (i >> 6)) << 2;
		}
	}
	// palletized 8 bit texture. As for Quake 1
	else if (0 == iType)
	{
		const unsigned char* szColorMap;
		this->SearchPalette(&szColorMap);

		// copy texture data
		unsigned int i = 0;
		for (; i < pcNew->mWidth*pcNew->mHeight;++i)
		{
			const unsigned char val = szData[i];
			const unsigned char* sz = &szColorMap[val*3];

			pcNew->pcData[i].a = 0xFF;
			pcNew->pcData[i].r = *sz++;
			pcNew->pcData[i].g = *sz++;
			pcNew->pcData[i].b = *sz;
		}
		*piSkip = i;

		this->FreePalette(szColorMap);
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::CreateTextureARGB8_GS5(const unsigned char* szData, 
	unsigned int iType,
	unsigned int* piSkip)
{
	ai_assert(NULL != piSkip);

	// allocate a new texture object
	aiTexture* pcNew = new aiTexture();

	// first read the size of the texture
	pcNew->mWidth = *((uint32_t*)szData);
	szData += sizeof(uint32_t);

	pcNew->mHeight = *((uint32_t*)szData);
	szData += sizeof(uint32_t);

	if (6 == iType)
	{
		// this is a compressed texture in DDS format
		*piSkip = pcNew->mWidth;

		pcNew->mHeight = 0;
		pcNew->achFormatHint[0] = 'd';
		pcNew->achFormatHint[1] = 'd';
		pcNew->achFormatHint[2] = 's';
		pcNew->achFormatHint[3] = '\0';

		pcNew->pcData = (aiTexel*) new unsigned char[pcNew->mWidth];
		memcpy(pcNew->pcData,szData,pcNew->mWidth);
	}
	else
	{
		// parse the color data of the texture
		this->ParseTextureColorData(szData,iType,
			piSkip,pcNew);
	}
	*piSkip += sizeof(uint32_t) * 2;

	// store the texture
	aiTexture** pc = this->pScene->mTextures;
	this->pScene->mTextures = new aiTexture*[this->pScene->mNumTextures+1];
	for (unsigned int i = 0; i < this->pScene->mNumTextures;++i)
		this->pScene->mTextures[i] = pc[i];

	this->pScene->mTextures[this->pScene->mNumTextures] = pcNew;
	this->pScene->mNumTextures++;
	delete[] pc;
	return;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::InternReadFile_Quake1( )
{
	ai_assert(NULL != pScene);

	if(0 == this->m_pcHeader->num_frames)
	{
		throw new ImportErrorException( "[Quake 1 MDL] No frames found");
	}

	// allocate enough storage to hold all vertices and triangles
	aiMesh* pcMesh = new aiMesh();

	// current cursor position in the file
	const unsigned char* szCurrent = (const unsigned char*)(this->m_pcHeader+1);

	// need to read all textures
	for (unsigned int i = 0; i < (unsigned int)this->m_pcHeader->num_skins;++i)
	{
		union{const MDL::Skin* pcSkin;const MDL::GroupSkin* pcGroupSkin;};
		pcSkin = (const MDL::Skin*)szCurrent;
		if (0 == pcSkin->group)
		{
			// create one output image
			this->CreateTextureARGB8((unsigned char*)pcSkin + sizeof(uint32_t));

			// need to skip one image
			szCurrent += this->m_pcHeader->skinheight * this->m_pcHeader->skinwidth+ sizeof(uint32_t);
		}
		else
		{
			// need to skip multiple images
			const unsigned int iNumImages = (unsigned int)pcGroupSkin->nb;
			szCurrent += sizeof(uint32_t) * 2;

			if (0 != iNumImages)
			{
				// however, create only one output image (the first)
				this->CreateTextureARGB8(szCurrent + iNumImages * sizeof(float));

				for (unsigned int a = 0; a < iNumImages;++a)
				{
					szCurrent += this->m_pcHeader->skinheight * this->m_pcHeader->skinwidth +
						sizeof(float);
				}
			}
		}
	}
	// get a pointer to the texture coordinates
	const MDL::TexCoord* pcTexCoords = (const MDL::TexCoord*)szCurrent;
	szCurrent += sizeof(MDL::TexCoord) * this->m_pcHeader->num_verts;

	// get a pointer to the triangles
	const MDL::Triangle* pcTriangles = (const MDL::Triangle*)szCurrent;
	szCurrent += sizeof(MDL::Triangle) * this->m_pcHeader->num_tris;

	// now get a pointer to the first frame in the file
	const MDL::Frame* pcFrames = (const MDL::Frame*)szCurrent;
	const MDL::SimpleFrame* pcFirstFrame;

	if (0 == pcFrames->type)
	{
		// get address of single frame
		pcFirstFrame = &pcFrames->frame;
	}
	else
	{
		// get the first frame in the group
		const MDL::GroupFrame* pcFrames2 = (const MDL::GroupFrame*)pcFrames;
		pcFirstFrame = (const MDL::SimpleFrame*)(&pcFrames2->time + pcFrames->type);
	}
	const MDL::Vertex* pcVertices = (const MDL::Vertex*) ((pcFirstFrame->name) +
		sizeof(pcFirstFrame->name));

	pcMesh->mNumVertices = this->m_pcHeader->num_tris * 3;
	pcMesh->mNumFaces = this->m_pcHeader->num_tris;
	pcMesh->mVertices = new aiVector3D[pcMesh->mNumVertices];
	pcMesh->mTextureCoords[0] = new aiVector3D[pcMesh->mNumVertices];
	pcMesh->mFaces = new aiFace[pcMesh->mNumFaces];
	pcMesh->mNormals = new aiVector3D[pcMesh->mNumVertices];
	pcMesh->mNumUVComponents[0] = 2;

	// there won't be more than one mesh inside the file
	pScene->mNumMaterials = 1;
	pScene->mRootNode = new aiNode();
	pScene->mRootNode->mNumMeshes = 1;
	pScene->mRootNode->mMeshes = new unsigned int[1];
	pScene->mRootNode->mMeshes[0] = 0;
	pScene->mMaterials = new aiMaterial*[1];
	pScene->mMaterials[0] = new MaterialHelper();
	pScene->mNumMeshes = 1;
	pScene->mMeshes = new aiMesh*[1];
	pScene->mMeshes[0] = pcMesh;

	// setup the material properties
	const int iMode = (int)aiShadingMode_Gouraud;
	MaterialHelper* pcHelper = (MaterialHelper*)pScene->mMaterials[0];
	pcHelper->AddProperty<int>(&iMode, 1, AI_MATKEY_SHADING_MODEL);

	aiColor3D clr;
	clr.b = clr.g = clr.r = 1.0f;
	pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_DIFFUSE);
	pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_SPECULAR);

	clr.b = clr.g = clr.r = 0.05f;
	pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_AMBIENT);

	if (0 != this->m_pcHeader->num_skins)
	{
		aiString szString;
		memcpy(szString.data,AI_MAKE_EMBEDDED_TEXNAME(0),3);
		szString.length = 2;
		pcHelper->AddProperty(&szString,AI_MATKEY_TEXTURE_DIFFUSE(0));
	}

	// now iterate through all triangles
	unsigned int iCurrent = 0;
	for (unsigned int i = 0; i < (unsigned int) this->m_pcHeader->num_tris;++i)
	{
		pcMesh->mFaces[i].mIndices = new unsigned int[3];
		pcMesh->mFaces[i].mNumIndices = 3;

		unsigned int iTemp = iCurrent;
		for (unsigned int c = 0; c < 3;++c,++iCurrent)
		{
			pcMesh->mFaces[i].mIndices[c] = iCurrent;

			// read vertices
			unsigned int iIndex = pcTriangles->vertex[c];
			if (iIndex >= (unsigned int)this->m_pcHeader->num_verts)
			{
				iIndex = this->m_pcHeader->num_verts-1;
				
				DefaultLogger::get()->warn("Index overflow in Q1-MDL vertex list.");
			}

			aiVector3D& vec = pcMesh->mVertices[iCurrent];
			vec.x = (float)pcVertices[iIndex].v[0] * this->m_pcHeader->scale[0];
			vec.x += this->m_pcHeader->translate[0];

			// (flip z and y component)
			vec.z = (float)pcVertices[iIndex].v[1] * this->m_pcHeader->scale[1];
			vec.z += this->m_pcHeader->translate[1];

			vec.y = (float)pcVertices[iIndex].v[2] * this->m_pcHeader->scale[2];
			vec.y += this->m_pcHeader->translate[2];

			// flip the Z-axis
			//pcMesh->mVertices[iBase+c].z *= -1.0f;

			// read the normal vector from the precalculated normal table
			pcMesh->mNormals[iCurrent] = *((const aiVector3D*)(&g_avNormals[std::min(
				int(pcVertices[iIndex].normalIndex),
				int(sizeof(g_avNormals) / sizeof(g_avNormals[0]))-1)]));

			//pcMesh->mNormals[iBase+c].z *= -1.0f;
			std::swap ( pcMesh->mNormals[iCurrent].y,pcMesh->mNormals[iCurrent].z );

			// read texture coordinates
			float s = (float)pcTexCoords[iIndex].s;
			float t = (float)pcTexCoords[iIndex].t;

			// translate texture coordinates
			if (0 == pcTriangles->facesfront &&
				0 != pcTexCoords[iIndex].onseam)
			{
				s += this->m_pcHeader->skinwidth * 0.5f; 
			}

			// Scale s and t to range from 0.0 to 1.0 
			pcMesh->mTextureCoords[0][iCurrent].x = (s + 0.5f) / this->m_pcHeader->skinwidth;
			pcMesh->mTextureCoords[0][iCurrent].y = 1.0f-(t + 0.5f) / this->m_pcHeader->skinheight;

		}
		pcMesh->mFaces[i].mIndices[0] = iTemp+2;
		pcMesh->mFaces[i].mIndices[1] = iTemp+1;
		pcMesh->mFaces[i].mIndices[2] = iTemp+0;
		pcTriangles++;
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::InternReadFile_GameStudio( )
{
	ai_assert(NULL != pScene);

	if(0 == this->m_pcHeader->num_frames)
	{
		throw new ImportErrorException( "[3DGS MDL] No frames found");
	}

	// allocate enough storage to hold all vertices and triangles
	aiMesh* pcMesh = new aiMesh();

	// current cursor position in the file
	const unsigned char* szCurrent = (const unsigned char*)(this->m_pcHeader+1);

	// need to read all textures
	for (unsigned int i = 0; i < (unsigned int)this->m_pcHeader->num_skins;++i)
	{
		union{const MDL::Skin* pcSkin;const MDL::GroupSkin* pcGroupSkin;};
		pcSkin = (const MDL::Skin*)szCurrent;
		
		// create one output image
		unsigned int iSkip = 0;
		if (5 <= this->iGSFileVersion)
		{
			// MDL5 format could contain MIPmaps
			this->CreateTextureARGB8_GS5((unsigned char*)pcSkin + sizeof(uint32_t),
				pcSkin->group,&iSkip);
		}
		else
		{
			this->CreateTextureARGB8_GS4((unsigned char*)pcSkin + sizeof(uint32_t),
				pcSkin->group,&iSkip);
		}
		// need to skip one image
		szCurrent += iSkip + sizeof(uint32_t);
		
	}
	// get a pointer to the texture coordinates
	const MDL::TexCoord_MDL3* pcTexCoords = (const MDL::TexCoord_MDL3*)szCurrent;
	szCurrent += sizeof(MDL::TexCoord_MDL3) * this->m_pcHeader->synctype;

	// NOTE: for MDLn formats syntype corresponds to the number of UV coords

	// get a pointer to the triangles
	const MDL::Triangle_MDL3* pcTriangles = (const MDL::Triangle_MDL3*)szCurrent;
	szCurrent += sizeof(MDL::Triangle_MDL3) * this->m_pcHeader->num_tris;

	pcMesh->mNumVertices = this->m_pcHeader->num_tris * 3;
	pcMesh->mNumFaces = this->m_pcHeader->num_tris;
	pcMesh->mFaces = new aiFace[pcMesh->mNumFaces];
	pcMesh->mNumUVComponents[0] = 2;

	// there won't be more than one mesh inside the file
	pScene->mNumMaterials = 1;
	pScene->mRootNode = new aiNode();
	pScene->mRootNode->mNumMeshes = 1;
	pScene->mRootNode->mMeshes = new unsigned int[1];
	pScene->mRootNode->mMeshes[0] = 0;
	pScene->mMaterials = new aiMaterial*[1];
	pScene->mMaterials[0] = new MaterialHelper();
	pScene->mNumMeshes = 1;
	pScene->mMeshes = new aiMesh*[1];
	pScene->mMeshes[0] = pcMesh;

	std::vector<aiVector3D> vPositions;
	std::vector<aiVector3D> vTexCoords;
	std::vector<aiVector3D> vNormals;

	vPositions.resize(pScene->mMeshes[0]->mNumFaces*3,aiVector3D());
	vTexCoords.resize(pScene->mMeshes[0]->mNumFaces*3,aiVector3D());
	vNormals.resize(pScene->mMeshes[0]->mNumFaces*3,aiVector3D());

	// setup the material properties
	const int iMode = (int)aiShadingMode_Gouraud;
	MaterialHelper* pcHelper = (MaterialHelper*)pScene->mMaterials[0];
	pcHelper->AddProperty<int>(&iMode, 1, AI_MATKEY_SHADING_MODEL);

	aiColor3D clr;
	clr.b = clr.g = clr.r = 1.0f;
	pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_DIFFUSE);
	pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_SPECULAR);

	clr.b = clr.g = clr.r = 0.05f;
	pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_AMBIENT);

	if (0 != this->m_pcHeader->num_skins)
	{
		aiString szString;
		memcpy(szString.data,AI_MAKE_EMBEDDED_TEXNAME(0),3);
		szString.length = 2;
		pcHelper->AddProperty(&szString,AI_MATKEY_TEXTURE_DIFFUSE(0));
	}

	// now get a pointer to the first frame in the file
	const MDL::Frame* pcFrames = (const MDL::Frame*)szCurrent;

	// byte packed vertices
	if (0 == pcFrames->type || 3 == this->iGSFileVersion)
	{
		const MDL::SimpleFrame* pcFirstFrame = (const MDL::SimpleFrame*)
			(szCurrent + sizeof(uint32_t));

		// get a pointer to the vertices
		const MDL::Vertex* pcVertices = (const MDL::Vertex*) ((pcFirstFrame->name) +
			sizeof(pcFirstFrame->name));

		// now iterate through all triangles
		unsigned int iCurrent = 0;
		for (unsigned int i = 0; i < (unsigned int) this->m_pcHeader->num_tris;++i)
		{
			pcMesh->mFaces[i].mIndices = new unsigned int[3];
			pcMesh->mFaces[i].mNumIndices = 3;

			unsigned int iTemp = iCurrent;
			for (unsigned int c = 0; c < 3;++c,++iCurrent)
			{
				// read vertices
				unsigned int iIndex = pcTriangles->index_xyz[c];
				if (iIndex >= (unsigned int)this->m_pcHeader->num_verts)
				{
					iIndex = this->m_pcHeader->num_verts-1;
					DefaultLogger::get()->warn("Index overflow in MDL3/4/5/6 vertex list");
				}

				aiVector3D& vec = vPositions[iCurrent];
				vec.x = (float)pcVertices[iIndex].v[0] * this->m_pcHeader->scale[0];
				vec.x += this->m_pcHeader->translate[0];

				// (flip z and y component)
				vec.z = (float)pcVertices[iIndex].v[1] * this->m_pcHeader->scale[1];
				vec.z += this->m_pcHeader->translate[1];

				vec.y = (float)pcVertices[iIndex].v[2] * this->m_pcHeader->scale[2];
				vec.y += this->m_pcHeader->translate[2];

				// read the normal vector from the precalculated normal table
				vNormals[iCurrent] = *((const aiVector3D*)(&g_avNormals[std::min(
					int(pcVertices[iIndex].normalIndex),
					int(sizeof(g_avNormals) / sizeof(g_avNormals[0]))-1)]));

				//vNormals[iBase+c].z *= -1.0f;
				std::swap ( vNormals[iCurrent].y,vNormals[iCurrent].z );

				// read texture coordinates
				iIndex = pcTriangles->index_uv[c];

				// validate UV indices
				if (iIndex >= (unsigned int)this->m_pcHeader->synctype)
				{
					iIndex = this->m_pcHeader->synctype-1;
					DefaultLogger::get()->warn("Index overflow in MDL3/4/5/6 UV coord list");
				}

				float s = (float)pcTexCoords[iIndex].u;
				float t = (float)pcTexCoords[iIndex].v;

				// Scale s and t to range from 0.0 to 1.0 
				if (5 != this->iGSFileVersion && 
					this->m_pcHeader->skinwidth && this->m_pcHeader->skinheight)
				{
					s = (s + 0.5f) / this->m_pcHeader->skinwidth;
					t = 1.0f-(t + 0.5f) / this->m_pcHeader->skinheight;
				}
				
				vTexCoords[iCurrent].x = s;
				vTexCoords[iCurrent].y = t;
			}
			pcMesh->mFaces[i].mIndices[0] = iTemp+2;
			pcMesh->mFaces[i].mIndices[1] = iTemp+1;
			pcMesh->mFaces[i].mIndices[2] = iTemp+0;
			pcTriangles++;
		}

	}
	// short packed vertices (duplicating the code is smaller than using templates ....)
	else
	{
		// now get a pointer to the first frame in the file
		const MDL::SimpleFrame_MDLn_SP* pcFirstFrame = (const MDL::SimpleFrame_MDLn_SP*)
			(szCurrent + sizeof(uint32_t));

		// get a pointer to the vertices
		const MDL::Vertex_MDL4* pcVertices = (const MDL::Vertex_MDL4*) ((pcFirstFrame->name) +
			sizeof(pcFirstFrame->name));

		// now iterate through all triangles
		unsigned int iCurrent = 0;
		for (unsigned int i = 0; i < (unsigned int) this->m_pcHeader->num_tris;++i)
		{
			pcMesh->mFaces[i].mIndices = new unsigned int[3];
			pcMesh->mFaces[i].mNumIndices = 3;

			unsigned int iTemp = iCurrent;
			for (unsigned int c = 0; c < 3;++c,++iCurrent)
			{
				// read vertices
				unsigned int iIndex = pcTriangles->index_xyz[c];
				if (iIndex >= (unsigned int)this->m_pcHeader->num_verts)
				{
					iIndex = this->m_pcHeader->num_verts-1;
					DefaultLogger::get()->warn("Index overflow in MDL3/4/5/6 vertex list");
				}

				aiVector3D& vec = vPositions[iCurrent];
				vec.x = (float)pcVertices[iIndex].v[0] * this->m_pcHeader->scale[0];
				vec.x += this->m_pcHeader->translate[0];

				// (flip z and y component)
				vec.z = (float)pcVertices[iIndex].v[1] * this->m_pcHeader->scale[1];
				vec.z += this->m_pcHeader->translate[1];

				vec.y = (float)pcVertices[iIndex].v[2] * this->m_pcHeader->scale[2];
				vec.y += this->m_pcHeader->translate[2];

				// read the normal vector from the precalculated normal table
				vNormals[iCurrent] = *((const aiVector3D*)(&g_avNormals[std::min(
					int(pcVertices[iIndex].normalIndex),
					int(sizeof(g_avNormals) / sizeof(g_avNormals[0]))-1)]));

				std::swap ( vNormals[iCurrent].y,vNormals[iCurrent].z );

				// read texture coordinates
				iIndex = pcTriangles->index_uv[c];

				// validate UV indices
				if (iIndex >= (unsigned int) this->m_pcHeader->synctype)
				{
					iIndex = this->m_pcHeader->synctype-1;
					DefaultLogger::get()->warn("Index overflow in MDL3/4/5/6 UV coord list");
				}

				float s = (float)pcTexCoords[iIndex].u;
				float t = (float)pcTexCoords[iIndex].v;

				
				// Scale s and t to range from 0.0 to 1.0 
				if (5 != this->iGSFileVersion && 
					this->m_pcHeader->skinwidth && this->m_pcHeader->skinheight)
				{
					s = (s + 0.5f) / this->m_pcHeader->skinwidth;
					t = 1.0f-(t + 0.5f) / this->m_pcHeader->skinheight;
				}

				vTexCoords[iCurrent].x = s;
				vTexCoords[iCurrent].y = t;
			}
			pcMesh->mFaces[i].mIndices[0] = iTemp+2;
			pcMesh->mFaces[i].mIndices[1] = iTemp+1;
			pcMesh->mFaces[i].mIndices[2] = iTemp+0;
			pcTriangles++;
		}
	}

	// For MDL5 we will need to build valid texture coordinates
	// basing upon the file loaded (only support one file as skin)
	if (5 == this->iGSFileVersion)
	{
		if (0 != this->m_pcHeader->num_skins && 0 != this->pScene->mNumTextures)
		{
			aiTexture* pcTex = this->pScene->mTextures[0];

			// if the file is loaded in DDS format: get the size of the
			// texture from the header of the DDS file
			// skip three DWORDs and read first height, then the width
			unsigned int iWidth, iHeight;
			if (0 == pcTex->mHeight)
			{
				uint32_t* piPtr = (uint32_t*)pcTex->pcData;
				
				piPtr += 3;
				iHeight = (unsigned int)*piPtr++;
				iWidth = (unsigned int)*piPtr;
			}
			else
			{
				iWidth = pcTex->mWidth;
				iHeight = pcTex->mHeight;
			}

			for (std::vector<aiVector3D>::iterator
				i =  vTexCoords.begin();
				i != vTexCoords.end();++i)
			{
				(*i).x /= iWidth;
				(*i).y /= iHeight;
				(*i).y = 1.0f- (*i).y; // DX to OGL
			}
		}
	}

	// allocate output storage
	pScene->mMeshes[0]->mNumVertices = vPositions.size();
	pScene->mMeshes[0]->mVertices = new aiVector3D[vPositions.size()];
	pScene->mMeshes[0]->mNormals = new aiVector3D[vPositions.size()];
	pScene->mMeshes[0]->mTextureCoords[0] = new aiVector3D[vPositions.size()];

	// memcpy() the data to the c-syle arrays
	memcpy(pScene->mMeshes[0]->mVertices,	&vPositions[0],	
		vPositions.size() * sizeof(aiVector3D));
	memcpy(pScene->mMeshes[0]->mNormals,	&vNormals[0],	
		vPositions.size() * sizeof(aiVector3D));
	memcpy(pScene->mMeshes[0]->mTextureCoords[0],	&vTexCoords[0],	
		vPositions.size() * sizeof(aiVector3D));
	return;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::ParseSkinLump_GameStudioA7(
	const unsigned char* szCurrent,
	const unsigned char** szCurrentOut,
	std::vector<MaterialHelper*>& pcMats)
{
	ai_assert(NULL != szCurrent);
	ai_assert(NULL != szCurrentOut);

	*szCurrentOut = szCurrent;
	const MDL::Skin_MDL7* pcSkin = (const MDL::Skin_MDL7*)szCurrent;
	szCurrent += 12;

	// allocate an output material
	MaterialHelper* pcMatOut = new MaterialHelper();
	pcMats.push_back(pcMatOut);

	aiTexture* pcNew = NULL;

	// get the type of the skin
	unsigned int iMasked = (unsigned int)(pcSkin->typ & 0xF);

	// skip length of file name
	szCurrent += AI_MDL7_MAX_TEXNAMESIZE;

	if (0x1 ==  iMasked)
	{
		// ***** REFERENCE TO ANOTHER SKIN INDEX *****

		// NOTE: Documentation - if you can call it a documentation, I prefer
		// the expression "rubbish" - states it is currently unused. However,
		// I don't know what ideas the terrible developers of Conitec will
		// have tomorrow, so Im going to implement it.
		int referrer = pcSkin->width;
		pcMatOut->AddProperty<int>(&referrer,1,"quakquakquak");
	}
	else if (0x6 == iMasked)
	{
		// ***** EMBEDDED DDS FILE *****
		if (1 != pcSkin->height)
		{
			DefaultLogger::get()->warn("Found a reference to an embedded DDS texture, "
				"but texture height is not equal to 1, which is not supported by MED");
		}

		pcNew = new aiTexture();
		pcNew->mHeight = 0;
		pcNew->achFormatHint[0] = 'd';
		pcNew->achFormatHint[1] = 'd';
		pcNew->achFormatHint[2] = 's';
		pcNew->achFormatHint[3] = '\0';

		pcNew->pcData = (aiTexel*) new unsigned char[pcNew->mWidth];
		memcpy(pcNew->pcData,szCurrent,pcNew->mWidth);
		szCurrent += pcSkin->width;
	}
	if (0x7 == iMasked)
	{
		// ***** REFERENCE TO EXTERNAL FILE *****
		if (1 != pcSkin->height)
		{
			DefaultLogger::get()->warn("Found a reference to an external texture, "
				"but texture height is not equal to 1, which is not supported by MED");
		}

		aiString szFile;
		const size_t iLen = strlen((const char*)szCurrent);
		size_t iLen2 = iLen+1;
		iLen2 = iLen2 > MAXLEN ? MAXLEN : iLen2;
		memcpy(szFile.data,(const char*)szCurrent,iLen2);
		szFile.length = iLen;

		szCurrent += iLen2;

		// place this as diffuse texture
		pcMatOut->AddProperty(&szFile,AI_MATKEY_TEXTURE_DIFFUSE(0));
	}
	else if (0 != iMasked || 0 == pcSkin->typ)
	{
		// ***** STANDARD COLOR TEXTURE *****
		pcNew = new aiTexture();
		if (0 == pcSkin->height || 0 == pcSkin->width)
		{
			DefaultLogger::get()->warn("Found embedded texture, but its width "
				"an height are both 0. Is this a joke?");

			// generate an empty chess pattern
			pcNew->mWidth = pcNew->mHeight = 8;
			pcNew->pcData = new aiTexel[64];
			for (unsigned int x = 0; x < 8;++x)
			{
				for (unsigned int y = 0; y < 8;++y)
				{
					bool bSet = false;
					if (0 == x % 2 && 0 != y % 2 ||
						0 != x % 2 && 0 == y % 2)bSet = true;
				
					aiTexel* pc = &pcNew->pcData[y * 8 + x];
					if (bSet)pc->r = pc->b = pc->g = 0xFF;
					else pc->r = pc->b = pc->g = 0;
					pc->a = 0xFF;
				}
			}
		}
		else
		{
			// it is a standard color texture. Fill in width and height
			// and call the same function we used for loading MDL5 files

			pcNew->mWidth = pcSkin->width;
			pcNew->mHeight = pcSkin->height;

			unsigned int iSkip = 0;
			this->ParseTextureColorData(szCurrent,iMasked,&iSkip,pcNew);

			// skip length of texture data
			szCurrent += iSkip;
		}
	}
	
	// check whether a material definition is contained in the skin
	if (pcSkin->typ & AI_MDL7_SKINTYPE_MATERIAL)
	{
		const MDL::Material_MDL7* pcMatIn = (const MDL::Material_MDL7*)szCurrent;
		szCurrent = (unsigned char*)(pcMatIn+1);

		aiColor4D clrTemp;

		// read diffuse color
		clrTemp.a = 1.0f; //pcMatIn->Diffuse.a;
		clrTemp.r = pcMatIn->Diffuse.r;
		clrTemp.g = pcMatIn->Diffuse.g;
		clrTemp.b = pcMatIn->Diffuse.b;
		pcMatOut->AddProperty<aiColor4D>(&clrTemp,1,AI_MATKEY_COLOR_DIFFUSE);

		// read specular color
		clrTemp.a = 1.0f; //pcMatIn->Specular.a;
		clrTemp.r = pcMatIn->Specular.r;
		clrTemp.g = pcMatIn->Specular.g;
		clrTemp.b = pcMatIn->Specular.b;
		pcMatOut->AddProperty<aiColor4D>(&clrTemp,1,AI_MATKEY_COLOR_SPECULAR);

		// read ambient color
		clrTemp.a = 1.0f; //pcMatIn->Ambient.a;
		clrTemp.r = pcMatIn->Ambient.r;
		clrTemp.g = pcMatIn->Ambient.g;
		clrTemp.b = pcMatIn->Ambient.b;
		pcMatOut->AddProperty<aiColor4D>(&clrTemp,1,AI_MATKEY_COLOR_AMBIENT);

		// read emissive color
		clrTemp.a = 1.0f; //pcMatIn->Emissive.a;
		clrTemp.r = pcMatIn->Emissive.r;
		clrTemp.g = pcMatIn->Emissive.g;
		clrTemp.b = pcMatIn->Emissive.b;
		pcMatOut->AddProperty<aiColor4D>(&clrTemp,1,AI_MATKEY_COLOR_EMISSIVE);

		// FIX: Take the opacity from the ambient color
		// the doc says something else, but it is fact that MED exports the
		// opacity like this .... ARRRGGHH!
		clrTemp.a = pcMatIn->Ambient.a;
		pcMatOut->AddProperty<float>(&clrTemp.a,1,AI_MATKEY_OPACITY);

		// read phong power
		int iShadingMode = (int)aiShadingMode_Gouraud;
		if (0.0f != pcMatIn->Power)
		{
			iShadingMode = (int)aiShadingMode_Phong;
			pcMatOut->AddProperty<float>(&pcMatIn->Power,1,AI_MATKEY_SHININESS);
		}
		pcMatOut->AddProperty<int>(&iShadingMode,1,AI_MATKEY_SHADING_MODEL);
	}

	// if an ASCII effect description (HLSL?) is contained in the file,
	// we can simply ignore it ...
	if (pcSkin->typ & AI_MDL7_SKINTYPE_MATERIAL_ASCDEF)
	{
		int32_t iMe = *((int32_t*)szCurrent);
		szCurrent += sizeof(char) * iMe + sizeof(int32_t);
	}

	// if an embedded texture has been loaded setup the corresponding
	// data structures in the aiScene instance
	if (NULL != pcNew)
	{
		// place this as diffuse texture
		char szCurrent[5];
		sprintf(szCurrent,"*%i",this->pScene->mNumTextures);
	
		aiString szFile;
		const size_t iLen = strlen((const char*)szCurrent);
		size_t iLen2 = iLen+1;
		iLen2 = iLen2 > MAXLEN ? MAXLEN : iLen2;
		memcpy(szFile.data,(const char*)szCurrent,iLen2);
		szFile.length = iLen;

		pcMatOut->AddProperty(&szFile,AI_MATKEY_TEXTURE_DIFFUSE(0));

		// store the texture
		aiTexture** pc = this->pScene->mTextures;
		this->pScene->mTextures = new aiTexture*[this->pScene->mNumTextures+1];
		for (unsigned int i = 0; i < this->pScene->mNumTextures;++i)
			this->pScene->mTextures[i] = pc[i];

		this->pScene->mTextures[this->pScene->mNumTextures] = pcNew;
		this->pScene->mNumTextures++;
		delete[] pc;
	}

	// place the name of the skin in the material
	const size_t iLen = strlen(pcSkin->texture_name); 
	if (0 != iLen)
	{
		aiString szFile;
		memcpy(szFile.data,pcSkin->texture_name,sizeof(pcSkin->texture_name));
		szFile.length = iLen;

		pcMatOut->AddProperty(&szFile,AI_MATKEY_NAME);
	}

	*szCurrentOut = szCurrent;
	return;
}

#define _AI_MDL7_ACCESS(_data, _index, _limit, _type) \
	(*((const _type*)(((const char*)_data) + _index * _limit)))

#define _AI_MDL7_ACCESS_VERT(_data, _index, _limit) \
	_AI_MDL7_ACCESS(_data,_index,_limit,MDL::Vertex_MDL7)

// ------------------------------------------------------------------------------------------------
void MDLImporter::ValidateHeader_GameStudioA7(const MDL::Header_MDL7* pcHeader)
{
	ai_assert(NULL != pcHeader);

	if (sizeof(MDL::ColorValue_MDL7) != pcHeader->colorvalue_stc_size)
	{
		// LOG
		throw new ImportErrorException( 
			"[3DGS MDL7] sizeof(MDL::ColorValue_MDL7) != pcHeader->colorvalue_stc_size");
	}
	if (sizeof(MDL::TexCoord_MDL7) != pcHeader->skinpoint_stc_size)
	{
		// LOG
		throw new ImportErrorException( 
			"[3DGS MDL7] sizeof(MDL::TexCoord_MDL7) != pcHeader->skinpoint_stc_size");
	}
	if (sizeof(MDL::Skin_MDL7) != pcHeader->skin_stc_size)
	{
		// LOG
		throw new ImportErrorException( 
			"sizeof(MDL::Skin_MDL7) != pcHeader->skin_stc_size");
	}

	// if there are no groups ... how should we load such a file?
	if(0 == pcHeader->groups_num)
	{
		// LOG
		throw new ImportErrorException( "[3DGS MDL7] No frames found");
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::InternReadFile_GameStudioA7( )
{
	ai_assert(NULL != pScene);

	// current cursor position in the file
	const MDL::Header_MDL7* pcHeader = (const MDL::Header_MDL7*)this->m_pcHeader;
	const unsigned char* szCurrent = (const unsigned char*)(pcHeader+1);

	// validate the header of the file. There are some structure
	// sizes that are expected by the loader to be constant 
	this->ValidateHeader_GameStudioA7(pcHeader);

	// skip all bones
	szCurrent += sizeof(MDL::Bone_MDL7) * pcHeader->bones_num;

	// allocate a material list
	std::vector<MaterialHelper*> pcMats;

	// vector to hold all created meshes
	std::vector<aiMesh*> avOutList;
	avOutList.reserve(pcHeader->groups_num);

	// read all groups
	for (unsigned int iGroup = 0; iGroup < (unsigned int)pcHeader->groups_num;++iGroup)
	{
		const MDL::Group_MDL7* pcGroup = (const MDL::Group_MDL7*)szCurrent;
		szCurrent = (const unsigned char*)(pcGroup+1);

		if (1 != pcGroup->typ)
		{
			// Not a triangle-based mesh
			DefaultLogger::get()->warn("[3DGS MDL7] Mesh group is not basing on"
				"triangles. Continuing happily");
		}

		// read all skins
		pcMats.reserve(pcMats.size() + pcGroup->numskins);
		for (unsigned int iSkin = 0; iSkin < (unsigned int)pcGroup->numskins;++iSkin)
		{
			this->ParseSkinLump_GameStudioA7(szCurrent,&szCurrent,pcMats);
		}
		// if we have absolutely no skin loaded we need to generate a default material
		if (pcMats.empty())
		{
			const int iMode = (int)aiShadingMode_Gouraud;
			pcMats.push_back(new MaterialHelper());
			MaterialHelper* pcHelper = (MaterialHelper*)pcMats[0];
			pcHelper->AddProperty<int>(&iMode, 1, AI_MATKEY_SHADING_MODEL);

			aiColor3D clr;
			clr.b = clr.g = clr.r = 0.6f;
			pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_DIFFUSE);
			pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_SPECULAR);

			clr.b = clr.g = clr.r = 0.05f;
			pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_AMBIENT);
		}

		// now get a pointer to all texture coords in the group
		const MDL::TexCoord_MDL7* pcGroupUVs = (const MDL::TexCoord_MDL7*)szCurrent;
		szCurrent += pcHeader->skinpoint_stc_size * pcGroup->num_stpts;

		// now get a pointer to all triangle in the group
		const MDL::Triangle_MDL7* pcGroupTris = (const MDL::Triangle_MDL7*)szCurrent;
		szCurrent += pcHeader->triangle_stc_size * pcGroup->numtris;

		// now get a pointer to all vertices in the group
		const MDL::Vertex_MDL7* pcGroupVerts = (const MDL::Vertex_MDL7*)szCurrent;
		szCurrent += pcHeader->mainvertex_stc_size * pcGroup->numverts;

		// build output vectors
		std::vector<aiVector3D> vPositions;
		vPositions.resize(pcGroup->numtris * 3);

		std::vector<aiVector3D> vNormals;
		vNormals.resize(pcGroup->numtris * 3);

		std::vector<aiVector3D> vTextureCoords1;
		vTextureCoords1.resize(pcGroup->numtris * 3,
			aiVector3D(std::numeric_limits<float>::quiet_NaN(),0.0f,0.0f));

		std::vector<aiVector3D> vTextureCoords2;
		
		bool bNeed2UV = false;
		if (pcHeader->triangle_stc_size >= sizeof(MDL::Triangle_MDL7))
		{
			vTextureCoords2.resize(pcGroup->numtris * 3,
			aiVector3D(std::numeric_limits<float>::quiet_NaN(),0.0f,0.0f));
			bNeed2UV = true;
		}
		MDL::IntFace_MDL7* pcFaces = new MDL::IntFace_MDL7[pcGroup->numtris];

		// iterate through all triangles and build valid display lists
		for (unsigned int iTriangle = 0; iTriangle < (unsigned int)pcGroup->numtris; ++iTriangle)
		{
			// iterate through all indices of the current triangle
			for (unsigned int c = 0; c < 3;++c)
			{
				// validate the vertex index
				unsigned int iIndex = pcGroupTris->v_index[c];
				if(iIndex > (unsigned int)pcGroup->numverts)
				{
					// LOG
					iIndex = pcGroup->numverts-1;

					DefaultLogger::get()->warn("Index overflow in MDL7 vertex list");
				}
				unsigned int iOutIndex = iTriangle * 3 + c;

				// write the output face index
				pcFaces[iTriangle].mIndices[c] = iTriangle * 3 + (2-c);

				// swap z and y axis
				vPositions[iOutIndex].x = _AI_MDL7_ACCESS_VERT(pcGroupVerts,iIndex,pcHeader->mainvertex_stc_size) .x;
				vPositions[iOutIndex].z = _AI_MDL7_ACCESS_VERT(pcGroupVerts,iIndex,pcHeader->mainvertex_stc_size) .y;
				vPositions[iOutIndex].y = _AI_MDL7_ACCESS_VERT(pcGroupVerts,iIndex,pcHeader->mainvertex_stc_size) .z;

				// now read the normal vector
				if (AI_MDL7_FRAMEVERTEX030305_STCSIZE <= pcHeader->mainvertex_stc_size)
				{
					// read the full normal vector
					vNormals[iOutIndex].x = _AI_MDL7_ACCESS_VERT(pcGroupVerts,iIndex,pcHeader->mainvertex_stc_size) .norm[0];
					vNormals[iOutIndex].z = _AI_MDL7_ACCESS_VERT(pcGroupVerts,iIndex,pcHeader->mainvertex_stc_size) .norm[1];
					vNormals[iOutIndex].y = _AI_MDL7_ACCESS_VERT(pcGroupVerts,iIndex,pcHeader->mainvertex_stc_size) .norm[2];

					// FIX: It seems to be necessary to invert all normals
					vNormals[iOutIndex].x *= -1.0f;
					vNormals[iOutIndex].y *= -1.0f;
					vNormals[iOutIndex].z *= -1.0f;
				}
				else if (AI_MDL7_FRAMEVERTEX120503_STCSIZE <= pcHeader->mainvertex_stc_size)
				{
					// read the normal vector from Quake2's smart table
					vNormals[iOutIndex] = *((const aiVector3D*)(&g_avNormals[std::min(
						int(_AI_MDL7_ACCESS_VERT(pcGroupVerts,iIndex,pcHeader->mainvertex_stc_size) .norm162index),
						int(sizeof(g_avNormals) / sizeof(g_avNormals[0]))-1)]));

					std::swap(vNormals[iOutIndex].z,vNormals[iOutIndex].y);
				}

			
				// validate and process the first uv coordinate set
				// *************************************************************
				const unsigned int iMin =  sizeof(MDL::Triangle_MDL7)-
					sizeof(MDL::SkinSet_MDL7)-sizeof(uint32_t);

				const unsigned int iMin2 =  sizeof(MDL::Triangle_MDL7)-
					sizeof(MDL::SkinSet_MDL7);

				if (pcHeader->triangle_stc_size >= iMin)
				{
					iIndex = pcGroupTris->skinsets[0].st_index[c];
					if(iIndex > (unsigned int)pcGroup->num_stpts)
					{
						iIndex = pcGroup->num_stpts-1;
					}

					float u = pcGroupUVs[iIndex].u;
					float v = 1.0f-pcGroupUVs[iIndex].v;
				
					vTextureCoords1[iOutIndex].x = u;
					vTextureCoords1[iOutIndex].y = v;
					
					// assign the material index, but only if it is existing
					if (pcHeader->triangle_stc_size >= iMin2)
					{
						pcFaces[iTriangle].iMatIndex[0] = pcGroupTris->skinsets[0].material;
					}
				}
				// validate and process the second uv coordinate set
				// *************************************************************
				if (pcHeader->triangle_stc_size >= sizeof(MDL::Triangle_MDL7))
				{
					iIndex = pcGroupTris->skinsets[1].st_index[c];
					if(iIndex > (unsigned int)pcGroup->num_stpts)
					{
						iIndex = pcGroup->num_stpts-1;
					}

					float u = pcGroupUVs[iIndex].u;
					float v = 1.0f-pcGroupUVs[iIndex].v;

					vTextureCoords2[iOutIndex].x = u;
					vTextureCoords2[iOutIndex].y = v;
				
					// check whether we do really need the second texture
					// coordinate set ... wastes memory and loading time
					if (0 != iIndex && (u != vTextureCoords1[iOutIndex].x ||
						v != vTextureCoords1[iOutIndex].y))
					{
						bNeed2UV = true;
					}
					// if the material differs, we need a second skin, too
					if (pcGroupTris->skinsets[1].material != pcGroupTris->skinsets[0].material)
					{
						bNeed2UV = true;
					}

					// assign the material index
					pcFaces[iTriangle].iMatIndex[1] = pcGroupTris->skinsets[1].material;
				}
			}

			// get the next triangle in the list
			pcGroupTris = (const MDL::Triangle_MDL7*)((const char*)pcGroupTris + pcHeader->triangle_stc_size);
		}

		// if we don't need a second set of texture coordinates there is no reason to keep it in memory ...
		std::vector<unsigned int>** aiSplit;
		unsigned int iNumMaterials = 0;
		if (!bNeed2UV)
		{
			vTextureCoords2.clear();

			// allocate the array
			aiSplit = new std::vector<unsigned int>*[pcMats.size()];
			iNumMaterials = pcMats.size();

			for (unsigned int m = 0; m < pcMats.size();++m)
				aiSplit[m] = new std::vector<unsigned int>();

			// iterate through all faces and sort by material
			for (unsigned int iFace = 0; iFace < (unsigned int)pcGroup->numtris;++iFace)
			{
				// check range
				if (pcFaces[iFace].iMatIndex[0] >= iNumMaterials)
				{
					// use the last material instead
					aiSplit[iNumMaterials-1]->push_back(iFace);

					// sometimes MED writes -1, but normally only if there is only
					// one skin assigned. No warning in this case
					if(0xFFFFFFFF != pcFaces[iFace].iMatIndex[0])
						DefaultLogger::get()->warn("Index overflow in MDL7 material list [#0]");
				}
				else aiSplit[pcFaces[iFace].iMatIndex[0]]->push_back(iFace);
			}
		}
		else
		{
			// we need to build combined materials for each combination of
			std::vector<MDL::IntMaterial_MDL7> avMats;
			avMats.reserve(pcMats.size()*2);

			std::vector<std::vector<unsigned int>* > aiTempSplit;
			aiTempSplit.reserve(pcMats.size()*2);

			for (unsigned int m = 0; m < pcMats.size();++m)
				aiTempSplit[m] = new std::vector<unsigned int>();

			// iterate through all faces and sort by material
			for (unsigned int iFace = 0; iFace < (unsigned int)pcGroup->numtris;++iFace)
			{
				// check range
				unsigned int iMatIndex = pcFaces[iFace].iMatIndex[0];
				if (iMatIndex >= iNumMaterials)
				{
					iMatIndex = iNumMaterials-1;

					// sometimes MED writes -1, but normally only if there is only
					// one skin assigned. No warning in this case
					if(0xFFFFFFFF != iMatIndex)
						DefaultLogger::get()->warn("Index overflow in MDL7 material list [#1]");
				}
				unsigned int iMatIndex2 = pcFaces[iFace].iMatIndex[1];
				if (iMatIndex2 >= iNumMaterials)
				{
					// sometimes MED writes -1, but normally only if there is only
					// one skin assigned. No warning in this case
					if(0xFFFFFFFF != iMatIndex2)
						DefaultLogger::get()->warn("Index overflow in MDL7 material list [#2]");
				}

				// do a slow O(log(n)*n) seach in the list ...
				unsigned int iNum = 0;
				bool bFound = false;
				for (std::vector<MDL::IntMaterial_MDL7>::iterator
					i =  avMats.begin();
					i != avMats.end();++i,++iNum)
				{
					if ((*i).iOldMatIndices[0] == iMatIndex &&
						(*i).iOldMatIndices[1] == iMatIndex2)
					{
						// reuse this material
						bFound = true;
						break;
					}
				}
				if (!bFound)
				{
					//  build a new material ...
					MDL::IntMaterial_MDL7 sHelper;
					sHelper.pcMat = new MaterialHelper();
					sHelper.iOldMatIndices[0] = iMatIndex;
					sHelper.iOldMatIndices[1] = iMatIndex2;
					this->JoinSkins_GameStudioA7(pcMats[iMatIndex],pcMats[iMatIndex2],sHelper.pcMat);

					// and add it to the list
					avMats.push_back(sHelper);
					iNum = avMats.size()-1;
				}
				// adjust the size of the file array
				if (iNum == aiTempSplit.size())
				{
					aiTempSplit.push_back(new std::vector<unsigned int>());
				}
				aiTempSplit[iNum]->push_back(iFace);
			}

			// now add the newly created materials to the old list
			if (0 == iGroup)
			{
				pcMats.resize(avMats.size());
				for (unsigned int o = 0; o < avMats.size();++o)
					pcMats[o] = avMats[o].pcMat;
			}
			else
			{
				// TODO: This might result in redundant materials ...
				unsigned int iOld = pcMats.size();
				pcMats.resize(pcMats.size() + avMats.size());
				for (unsigned int o = iOld; o < avMats.size();++o)
					pcMats[o] = avMats[o].pcMat;
			}
			iNumMaterials = pcMats.size();

			// and build the final face-to-material array
			aiSplit = new std::vector<unsigned int>*[aiTempSplit.size()];
			for (unsigned int m = 0; m < iNumMaterials;++m)
				aiSplit[m] = aiTempSplit[m];

			// no need to delete the member of aiTempSplit
		}

		// now generate output meshes
		unsigned int iOldSize = avOutList.size();
		this->GenerateOutputMeshes_GameStudioA7(
			(const std::vector<unsigned int>**)aiSplit,pcMats,
			avOutList,pcFaces,vPositions,vNormals, vTextureCoords1,vTextureCoords2);

		// store the group index temporarily
		ai_assert(AI_MAX_NUMBER_OF_TEXTURECOORDS >= 3);
		for (unsigned int l = iOldSize;l < avOutList.size();++l)
		{
			avOutList[l]->mNumUVComponents[2] = iGroup;
		}

		// delete the face-to-material helper array
		for (unsigned int m = 0; m < iNumMaterials;++m)
			delete aiSplit[m];
		delete[] aiSplit;

		// now we need to skip all faces
		for(unsigned int iFrame = 0; iFrame < (unsigned int)pcGroup->numframes;++iFrame)
		{
			const MDL::Frame_MDL7* pcFrame = (const MDL::Frame_MDL7*)szCurrent;

			unsigned int iAdd = pcHeader->frame_stc_size + 
				pcFrame->vertices_count * pcHeader->framevertex_stc_size +
				pcFrame->transmatrix_count * pcHeader->bonetrans_stc_size;

			if (((unsigned int)szCurrent -  (unsigned int)pcHeader) + iAdd > (unsigned int)pcHeader->data_size)
			{
				DefaultLogger::get()->warn("Index overflow in frame area. Ignoring frames");
				// don't parse more groups if we can't even read one
				goto __BREAK_OUT;
			}

			szCurrent += iAdd;
		}
	}
__BREAK_OUT: // EVIL ;-)

	// now we need to build a final mesh list
	this->pScene->mNumMeshes = avOutList.size();
	this->pScene->mMeshes = new aiMesh*[avOutList.size()];

	for (unsigned int i = 0; i < avOutList.size();++i)
	{
		this->pScene->mMeshes[i] = avOutList[i];
	}

	// build a final material list. Offset all mesh material indices
	this->pScene->mNumMaterials = pcMats.size();
	this->pScene->mMaterials = new aiMaterial*[this->pScene->mNumMaterials];
	for (unsigned int i = 0; i < this->pScene->mNumMaterials;++i)
		this->pScene->mMaterials[i] = pcMats[i];
	
	// search for referrer materials
	for (unsigned int i = 0; i < this->pScene->mNumMaterials;++i)
	{
		int iIndex = 0;
		if (AI_SUCCESS == aiGetMaterialInteger(this->pScene->mMaterials[i],
			"quakquakquak", &iIndex) )
		{
			for (unsigned int a = 0; a < avOutList.size();++a)
			{
				if (i == avOutList[a]->mMaterialIndex)
				{
					avOutList[a]->mMaterialIndex = iIndex;
				}
			}
			// TODO: Remove the material from the list
		}
	}

	// now generate a nodegraph whose rootnode references all meshes
	this->pScene->mRootNode = new aiNode();
	this->pScene->mRootNode->mNumMeshes = this->pScene->mNumMeshes;
	this->pScene->mRootNode->mMeshes = new unsigned int[this->pScene->mRootNode->mNumMeshes];
	for (unsigned int i = 0; i < this->pScene->mRootNode->mNumMeshes;++i)
		this->pScene->mRootNode->mMeshes[i] = i;

	// seems we're finished now
	return;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::GenerateOutputMeshes_GameStudioA7(
	const std::vector<unsigned int>** aiSplit,
	const std::vector<MaterialHelper*>& pcMats,
	std::vector<aiMesh*>& avOutList,
	const MDL::IntFace_MDL7* pcFaces,
	const std::vector<aiVector3D>& vPositions,
	const std::vector<aiVector3D>& vNormals, 
	const std::vector<aiVector3D>& vTextureCoords1,
	const std::vector<aiVector3D>& vTextureCoords2)
{
	ai_assert(NULL != aiSplit);
	ai_assert(NULL != pcFaces);

	for (unsigned int i = 0; i < pcMats.size();++i)
	{
		if (!aiSplit[i]->empty())
		{
			// allocate the output mesh
			aiMesh* pcMesh = new aiMesh();
			pcMesh->mNumUVComponents[0] = 2;
			pcMesh->mMaterialIndex = i;

			// allocate output storage
			pcMesh->mNumFaces = aiSplit[i]->size();
			pcMesh->mFaces = new aiFace[pcMesh->mNumFaces];

			pcMesh->mNumVertices = pcMesh->mNumFaces*3;
			pcMesh->mVertices = new aiVector3D[pcMesh->mNumVertices];
			pcMesh->mNormals = new aiVector3D[pcMesh->mNumVertices];

			pcMesh->mTextureCoords[0] = new aiVector3D[pcMesh->mNumVertices];
			if (!vTextureCoords2.empty())
			{
				pcMesh->mNumUVComponents[1] = 2;
				pcMesh->mTextureCoords[1] = new aiVector3D[pcMesh->mNumVertices];
			}

			// iterate through all faces and build an unique set of vertices
			unsigned int iCurrent = 0;
			for (unsigned int iFace = 0; iFace < pcMesh->mNumFaces;++iFace)
			{
				pcMesh->mFaces[iFace].mNumIndices = 3;
				pcMesh->mFaces[iFace].mIndices = new unsigned int[3];

				unsigned int iSrcFace = aiSplit[i]->operator[](iFace);
				const MDL::IntFace_MDL7& oldFace = pcFaces[iSrcFace];

				// iterate through all face indices
				for (unsigned int c = 0; c < 3;++c)
				{
					pcMesh->mVertices[iCurrent] = vPositions[oldFace.mIndices[c]];
					pcMesh->mNormals[iCurrent] = vNormals[oldFace.mIndices[c]];
					pcMesh->mTextureCoords[0][iCurrent] = vTextureCoords1[oldFace.mIndices[c]];

					if (!vTextureCoords2.empty())
					{
						pcMesh->mTextureCoords[1][iCurrent] = vTextureCoords2[oldFace.mIndices[c]];
					}

					pcMesh->mFaces[iFace].mIndices[c] = iCurrent;
					++iCurrent;
				}
			}

			// add the mesh to the list of output meshes
			avOutList.push_back(pcMesh);
		}
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::JoinSkins_GameStudioA7(
	MaterialHelper* pcMat1,
	MaterialHelper* pcMat2,
	MaterialHelper* pcMatOut)
{
	ai_assert(NULL != pcMat1);
	ai_assert(NULL != pcMat2);
	ai_assert(NULL != pcMatOut);

	// first create a full copy of the first skin property set
	// and assign it to the output material
	MaterialHelper::CopyPropertyList(pcMatOut,pcMat1);

	int iVal = 0;
	pcMatOut->AddProperty<int>(&iVal,1,AI_MATKEY_UVWSRC_DIFFUSE(0));

	// then extract the diffuse texture from the second skin,
	// setup 1 as UV source and we have it

	aiString sString;
	if(AI_SUCCESS == aiGetMaterialString ( pcMat2, AI_MATKEY_TEXTURE_DIFFUSE(0),&sString ))
	{
		iVal = 1;
		pcMatOut->AddProperty<int>(&iVal,1,AI_MATKEY_UVWSRC_DIFFUSE(1));
		pcMatOut->AddProperty(&sString,AI_MATKEY_TEXTURE_DIFFUSE(1));
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::FlipNormals(aiMesh* pcMesh)
{
	ai_assert(NULL != pcMesh);

	// compute the bounding box of both the model vertices + normals and
	// the umodified model vertices. Then check whether the first BB
	// is smaller than the second. In this case we can assume that the
	// normals need to be flipped, although there are a few special cases ..
	// convex, concave, planar models ...

	aiVector3D vMin0(1e10f,1e10f,1e10f);
	aiVector3D vMin1(1e10f,1e10f,1e10f);
	aiVector3D vMax0(-1e10f,-1e10f,-1e10f);
	aiVector3D vMax1(-1e10f,-1e10f,-1e10f);

	for (unsigned int i = 0; i < pcMesh->mNumVertices;++i)
	{
		vMin1.x = std::min(vMin1.x,pcMesh->mVertices[i].x);
		vMin1.y = std::min(vMin1.y,pcMesh->mVertices[i].y);
		vMin1.z = std::min(vMin1.z,pcMesh->mVertices[i].z);

		vMax1.x = std::max(vMax1.x,pcMesh->mVertices[i].x);
		vMax1.y = std::max(vMax1.y,pcMesh->mVertices[i].y);
		vMax1.z = std::max(vMax1.z,pcMesh->mVertices[i].z);

		aiVector3D vWithNormal = pcMesh->mVertices[i] + pcMesh->mNormals[i];

		vMin0.x = std::min(vMin0.x,vWithNormal.x);
		vMin0.y = std::min(vMin0.y,vWithNormal.y);
		vMin0.z = std::min(vMin0.z,vWithNormal.z);

		vMax0.x = std::max(vMax0.x,vWithNormal.x);
		vMax0.y = std::max(vMax0.y,vWithNormal.y);
		vMax0.z = std::max(vMax0.z,vWithNormal.z);
	}

	if (fabsf((vMax0.x - vMin0.x) * (vMax0.y - vMin0.y) * (vMax0.z - vMin0.z)) <= 
		fabsf((vMax1.x - vMin1.x) * (vMax1.y - vMin1.y) * (vMax1.z - vMin1.z)))
	{
		DefaultLogger::get()->info("The models normals are facing inwards "
			"(or the model is too planar or concave). Flipping the normal set ...");

		for (unsigned int i = 0; i < pcMesh->mNumVertices;++i)
		{
			pcMesh->mNormals[i] *= -1.0f;
		}
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::InternReadFile_HL2( )
{
	const MDL::Header_HL2* pcHeader = (const MDL::Header_HL2*)this->mBuffer;
	return;
}