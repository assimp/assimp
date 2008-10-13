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

#include "AssimpPCH.h"

// internal headers
#include "MaterialSystem.h"
#include "HMPLoader.h"
#include "MD2FileData.h"


using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
HMPImporter::HMPImporter()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
HMPImporter::~HMPImporter()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool HMPImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
{
	// simple check of file extension is enough for the moment
	std::string::size_type pos = pFile.find_last_of('.');
	// no file extension - can't read
	if( pos == std::string::npos)
		return false;
	std::string extension = pFile.substr( pos);

	if (extension.length() < 4)return false;
	if (extension[0] != '.')return false;
	if (extension[1] != 'h' && extension[1] != 'H')return false;
	if (extension[2] != 'm' && extension[2] != 'M')return false;
	if (extension[3] != 'p' && extension[3] != 'P')return false;

	return true;
}
// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void HMPImporter::InternReadFile( 
	const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile));

	// Check whether we can read from the file
	if( file.get() == NULL)
	{
		throw new ImportErrorException( "Failed to open HMP file " + pFile + ".");
	}

	// check whether the HMP file is large enough to contain
	// at least the file header
	size_t fileSize = file->FileSize();
	if( fileSize < 50)
	{
		throw new ImportErrorException( "HMP File is too small.");
	}

	// allocate storage and copy the contents of the file to a memory buffer
	this->pScene = pScene;
	this->pIOHandler = pIOHandler;
	this->mBuffer = new unsigned char[fileSize+1];
	file->Read( (void*)mBuffer, 1, fileSize);

	this->iFileSize = (unsigned int)fileSize;

	// determine the file subtype and call the appropriate member function
	uint32_t iMagic = *((uint32_t*)this->mBuffer);

	try {

	// HMP4 format
	if (AI_HMP_MAGIC_NUMBER_LE_4 == iMagic ||
		AI_HMP_MAGIC_NUMBER_BE_4 == iMagic)
	{
		DefaultLogger::get()->debug("HMP subtype: 3D GameStudio A4, magic word is HMP4");
		this->InternReadFile_HMP4();
	}
	// HMP5 format
	else if (AI_HMP_MAGIC_NUMBER_LE_5 == iMagic ||
			 AI_HMP_MAGIC_NUMBER_BE_5 == iMagic)
	{
		DefaultLogger::get()->debug("HMP subtype: 3D GameStudio A5, magic word is HMP5");
		this->InternReadFile_HMP5();
	}
	// HMP7 format
	else if (AI_HMP_MAGIC_NUMBER_LE_7 == iMagic ||
			 AI_HMP_MAGIC_NUMBER_BE_7 == iMagic)
	{
		DefaultLogger::get()->debug("HMP subtype: 3D GameStudio A7, magic word is HMP7");
		this->InternReadFile_HMP7();
	}
	else
	{
		// print the magic word to the logger
		char szBuffer[5];
		szBuffer[0] = ((char*)&iMagic)[0];
		szBuffer[1] = ((char*)&iMagic)[1];
		szBuffer[2] = ((char*)&iMagic)[2];
		szBuffer[3] = ((char*)&iMagic)[3];
		szBuffer[4] = '\0';

		// we're definitely unable to load this file
		throw new ImportErrorException( "Unknown HMP subformat " + pFile +
			". Magic word (" + szBuffer + ") is not known");
	}

	} catch (ImportErrorException* ex) {
		delete[] this->mBuffer;
		throw ex;
	}

	// delete the file buffer
	delete[] this->mBuffer;
	return;
}
// ------------------------------------------------------------------------------------------------ 
void HMPImporter::ValidateHeader_HMP457( )
{
	const HMP::Header_HMP5* const pcHeader = (const HMP::Header_HMP5*)this->mBuffer;

	if (120 > this->iFileSize)
	{
		throw new ImportErrorException("HMP file is too small (header size is "
			"120 bytes, this file is smaller)");
	}

	if (!pcHeader->ftrisize_x || !pcHeader->ftrisize_y)
	{
		throw new ImportErrorException("Size of triangles in either  x or y direction is zero");
	}
	if(pcHeader->fnumverts_x < 1.0f || (pcHeader->numverts/pcHeader->fnumverts_x) < 1.0f)
	{
		throw new ImportErrorException("Number of triangles in either x or y direction is zero");
	}
	if(!pcHeader->numframes)
	{
		throw new ImportErrorException("There are no frames. At least one should be there");
	}
}
// ------------------------------------------------------------------------------------------------ 
void HMPImporter::InternReadFile_HMP4( )
{
	throw new ImportErrorException("HMP4 is currently not supported");
}
// ------------------------------------------------------------------------------------------------ 
void HMPImporter::InternReadFile_HMP5( )
{
	// read the file header and skip everything to byte 84
	const HMP::Header_HMP5* pcHeader = (const HMP::Header_HMP5*)this->mBuffer;
	const unsigned char* szCurrent = (const unsigned char*)(this->mBuffer+84);
	this->ValidateHeader_HMP457();

	// generate an output mesh
	this->pScene->mNumMeshes = 1;
	this->pScene->mMeshes = new aiMesh*[1];
	aiMesh* pcMesh = this->pScene->mMeshes[0] = new aiMesh();

	pcMesh->mMaterialIndex = 0;
	pcMesh->mVertices = new aiVector3D[pcHeader->numverts];
	pcMesh->mNormals = new aiVector3D[pcHeader->numverts];

	const unsigned int height = (unsigned int)(pcHeader->numverts / pcHeader->fnumverts_x);
	const unsigned int width = (unsigned int)pcHeader->fnumverts_x;

	// generate/load a material for the terrain
	this->CreateMaterial(szCurrent,&szCurrent);

	// goto offset 120, I don't know why ...
	// (fixme) is this the frame header? I assume yes since it starts with 2. 
	szCurrent += 36;

	this->SizeCheck(szCurrent + sizeof(const HMP::Vertex_HMP7)*height*width);

	// now load all vertices from the file
	aiVector3D* pcVertOut = pcMesh->mVertices;
	aiVector3D* pcNorOut = pcMesh->mNormals;
	const HMP::Vertex_HMP5* src = (const HMP::Vertex_HMP5*) szCurrent;
	for (unsigned int y = 0; y < height;++y)
	{
		for (unsigned int x = 0; x < width;++x)
		{
			pcVertOut->x = x * pcHeader->ftrisize_x;
			pcVertOut->y = y * pcHeader->ftrisize_y;
			pcVertOut->z = (((float)src->z / 0xffff)-0.5f) * pcHeader->ftrisize_x * 8.0f; 
			MD2::LookupNormalIndex(src->normals162index, *pcNorOut );
			++pcVertOut;++pcNorOut;++src;
		}
	}

	// generate texture coordinates if necessary
	if (pcHeader->numskins)this->GenerateTextureCoords(width,height);

	// now build a list of faces
	this->CreateOutputFaceList(width,height);	

	// there is no nodegraph in HMP files. Simply assign the one mesh
	// (no, not the one ring) to the root node
	this->pScene->mRootNode = new aiNode();
	this->pScene->mRootNode->mName.Set("terrain_root");
	this->pScene->mRootNode->mNumMeshes = 1;
	this->pScene->mRootNode->mMeshes = new unsigned int[1];
	this->pScene->mRootNode->mMeshes[0] = 0;
}
// ------------------------------------------------------------------------------------------------ 
void HMPImporter::InternReadFile_HMP7( )
{
	// read the file header and skip everything to byte 84
	const HMP::Header_HMP5* const pcHeader = (const HMP::Header_HMP5*)this->mBuffer;
	const unsigned char* szCurrent = (const unsigned char*)(this->mBuffer+84);
	this->ValidateHeader_HMP457();

	// generate an output mesh
	this->pScene->mNumMeshes = 1;
	this->pScene->mMeshes = new aiMesh*[1];
	aiMesh* pcMesh = this->pScene->mMeshes[0] = new aiMesh();

	pcMesh->mMaterialIndex = 0;
	pcMesh->mVertices = new aiVector3D[pcHeader->numverts];
	pcMesh->mNormals = new aiVector3D[pcHeader->numverts];

	const unsigned int height = (unsigned int)(pcHeader->numverts / pcHeader->fnumverts_x);
	const unsigned int width = (unsigned int)pcHeader->fnumverts_x;

	// generate/load a material for the terrain
	this->CreateMaterial(szCurrent,&szCurrent);

	// goto offset 120, I don't know why ...
	// (fixme) is this the frame header? I assume yes since it starts with 2. 
	szCurrent += 36;

	this->SizeCheck(szCurrent + sizeof(const HMP::Vertex_HMP7)*height*width);

	// now load all vertices from the file
	aiVector3D* pcVertOut = pcMesh->mVertices;
	aiVector3D* pcNorOut = pcMesh->mNormals;
	const HMP::Vertex_HMP7* src = (const HMP::Vertex_HMP7*) szCurrent;
	for (unsigned int y = 0; y < height;++y)
	{
		for (unsigned int x = 0; x < width;++x)
		{
			pcVertOut->x = x * pcHeader->ftrisize_x;
			pcVertOut->y = y * pcHeader->ftrisize_y;

			// FIXME: What exctly is the correct scaling factor to use?
			// possibly pcHeader->scale_origin[2] in combination with a
			// signed interpretation of src->z?
			pcVertOut->z = (((float)src->z / 0xffff)-0.5f) * pcHeader->ftrisize_x * 8.0f; 

			pcNorOut->x = ((float)src->normal_x / 0x80 ); // * pcHeader->scale_origin[0];
			pcNorOut->y = ((float)src->normal_y / 0x80 ); // * pcHeader->scale_origin[1];
			pcNorOut->z = 1.0f;
			pcNorOut->Normalize();
			
			++pcVertOut;++pcNorOut;++src;
		}
	}

	// generate texture coordinates if necessary
	if (pcHeader->numskins)this->GenerateTextureCoords(width,height);

	// now build a list of faces
	this->CreateOutputFaceList(width,height);	

	// there is no nodegraph in HMP files. Simply assign the one mesh
	// (no, not the One Ring) to the root node
	this->pScene->mRootNode = new aiNode();
	this->pScene->mRootNode->mName.Set("terrain_root");
	this->pScene->mRootNode->mNumMeshes = 1;
	this->pScene->mRootNode->mMeshes = new unsigned int[1];
	this->pScene->mRootNode->mMeshes[0] = 0;
}
// ------------------------------------------------------------------------------------------------ 
void HMPImporter::CreateMaterial(const unsigned char* szCurrent,
	const unsigned char** szCurrentOut)
{
	aiMesh* const pcMesh = this->pScene->mMeshes[0];
	const HMP::Header_HMP5* const pcHeader = (const HMP::Header_HMP5*)this->mBuffer;

	// we don't need to generate texture coordinates if
	// we have no textures in the file ...
	if (pcHeader->numskins)
	{
		pcMesh->mTextureCoords[0] = new aiVector3D[pcHeader->numverts];
		pcMesh->mNumUVComponents[0] = 2;

		// now read the first skin and skip all others
		this->ReadFirstSkin(pcHeader->numskins,szCurrent,&szCurrent);
	}
	else
	{
		// generate a default material
		const int iMode = (int)aiShadingMode_Gouraud;
		MaterialHelper* pcHelper = new MaterialHelper();
		pcHelper->AddProperty<int>(&iMode, 1, AI_MATKEY_SHADING_MODEL);

		aiColor3D clr;
		clr.b = clr.g = clr.r = 0.6f;
		pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_DIFFUSE);
		pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_SPECULAR);

		clr.b = clr.g = clr.r = 0.05f;
		pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_AMBIENT);

		aiString szName;
		szName.Set(AI_DEFAULT_MATERIAL_NAME);
		pcHelper->AddProperty(&szName,AI_MATKEY_NAME);

		// add the material to the scene
		this->pScene->mNumMaterials = 1;
		this->pScene->mMaterials = new aiMaterial*[1];
		this->pScene->mMaterials[0] = pcHelper;
	}
	*szCurrentOut = szCurrent;
}
// ------------------------------------------------------------------------------------------------ 
void HMPImporter::CreateOutputFaceList(unsigned int width,unsigned int height)
{
	aiMesh* const pcMesh = this->pScene->mMeshes[0];

	// allocate enough storage
	const unsigned int iNumSquares = (width-1) * (height-1);
	pcMesh->mNumFaces = iNumSquares << 1;
	pcMesh->mFaces = new aiFace[pcMesh->mNumFaces];

	pcMesh->mNumVertices = pcMesh->mNumFaces*3;
	aiVector3D* pcVertices = new aiVector3D[pcMesh->mNumVertices];
	aiVector3D* pcNormals = new aiVector3D[pcMesh->mNumVertices];

	aiFace* pcFaceOut(pcMesh->mFaces);
	aiVector3D* pcVertOut = pcVertices;
	aiVector3D* pcNorOut = pcNormals;

	aiVector3D* pcUVs = pcMesh->mTextureCoords[0] ? new aiVector3D[pcMesh->mNumVertices] : NULL;
	aiVector3D* pcUVOut(pcUVs);

	// build the terrain square
	unsigned int iCurrent = 0;
	for (unsigned int y = 0; y < height-1;++y)
	{
		for (unsigned int x = 0; x < width-1;++x)
		{
			// first triangle of the square
			pcFaceOut->mNumIndices = 3;
			pcFaceOut->mIndices = new unsigned int[3];

			*pcVertOut++ = pcMesh->mVertices[y*width+x];
			*pcVertOut++ = pcMesh->mVertices[y*width+x+1];
			*pcVertOut++ = pcMesh->mVertices[(y+1)*width+x];

			*pcNorOut++ = pcMesh->mNormals[y*width+x];
			*pcNorOut++ = pcMesh->mNormals[y*width+x+1];
			*pcNorOut++ = pcMesh->mNormals[(y+1)*width+x];

			if (pcMesh->mTextureCoords[0])
			{
				*pcUVOut++ = pcMesh->mTextureCoords[0][y*width+x];
				*pcUVOut++ = pcMesh->mTextureCoords[0][y*width+x+1];
				*pcUVOut++ = pcMesh->mTextureCoords[0][(y+1)*width+x];
			}
			
			pcFaceOut->mIndices[2] = iCurrent++;
			pcFaceOut->mIndices[1] = iCurrent++;
			pcFaceOut->mIndices[0] = iCurrent++;
			++pcFaceOut;

			// second triangle of the square
			pcFaceOut->mNumIndices = 3;
			pcFaceOut->mIndices = new unsigned int[3];

			*pcVertOut++ = pcMesh->mVertices[(y+1)*width+x];
			*pcVertOut++ = pcMesh->mVertices[y*width+x+1];
			*pcVertOut++ = pcMesh->mVertices[(y+1)*width+x+1];

			*pcNorOut++ = pcMesh->mNormals[(y+1)*width+x];
			*pcNorOut++ = pcMesh->mNormals[y*width+x+1];
			*pcNorOut++ = pcMesh->mNormals[(y+1)*width+x+1];

			if (pcMesh->mTextureCoords[0])
			{
				*pcUVOut++ = pcMesh->mTextureCoords[0][(y+1)*width+x];
				*pcUVOut++ = pcMesh->mTextureCoords[0][y*width+x+1];
				*pcUVOut++ = pcMesh->mTextureCoords[0][(y+1)*width+x+1];
			}
			
			pcFaceOut->mIndices[2] = iCurrent++;
			pcFaceOut->mIndices[1] = iCurrent++;
			pcFaceOut->mIndices[0] = iCurrent++;
			++pcFaceOut;
		}
	}
	delete[] pcMesh->mVertices;
	pcMesh->mVertices = pcVertices;

	delete[] pcMesh->mNormals;
	pcMesh->mNormals = pcNormals;

	if (pcMesh->mTextureCoords[0])
	{
		delete[] pcMesh->mTextureCoords[0];
		pcMesh->mTextureCoords[0] = pcUVs;
	}
}
// ------------------------------------------------------------------------------------------------ 
void HMPImporter::ReadFirstSkin(unsigned int iNumSkins, const unsigned char* szCursor,
	const unsigned char** szCursorOut)
{
	ai_assert(0 != iNumSkins && NULL != szCursor);

	// read the type of the skin ...
	// sometimes we need to skip 12 bytes here, I don't know why ...
	uint32_t iType = *((uint32_t*)szCursor);szCursor += sizeof(uint32_t);
	if (0 == iType)
	{
		DefaultLogger::get()->warn("Skin type is 0. Skipping 12 bytes to "
			"the next valid value, which seems to be the real skin type. "
			"However, it is not known whether or not this is correct.");
		szCursor += sizeof(uint32_t) * 2;
		iType = *((uint32_t*)szCursor);szCursor += sizeof(uint32_t);
		if (0 == iType)
		{
			throw new ImportErrorException("Unable to read HMP7 skin chunk");
		}
	}
	// read width and height
	uint32_t iWidth = *((uint32_t*)szCursor);szCursor += sizeof(uint32_t);
	uint32_t iHeight = *((uint32_t*)szCursor);szCursor += sizeof(uint32_t);

	// allocate an output material
	MaterialHelper* pcMat = new MaterialHelper();

	// read the skin, this works exactly as for MDL7
	this->ParseSkinLump_3DGS_MDL7(szCursor,&szCursor,
		pcMat,iType,iWidth,iHeight);

	// now we need to skip any other skins ... 
	for (unsigned int i = 1; i< iNumSkins;++i)
	{
		iType = *((uint32_t*)szCursor);szCursor += sizeof(uint32_t);
		iWidth = *((uint32_t*)szCursor);szCursor += sizeof(uint32_t);
		iHeight = *((uint32_t*)szCursor);szCursor += sizeof(uint32_t);

		this->SkipSkinLump_3DGS_MDL7(szCursor,&szCursor,
			iType,iWidth,iHeight);

		this->SizeCheck(szCursor);
	}

	// setup the material ...
	this->pScene->mNumMaterials = 1;
	this->pScene->mMaterials = new aiMaterial*[1];
	this->pScene->mMaterials[0] = pcMat;

	*szCursorOut = szCursor;
	return;
}
// ------------------------------------------------------------------------------------------------ 
void HMPImporter::GenerateTextureCoords(
	const unsigned int width, const unsigned int height)
{
	ai_assert(NULL != this->pScene->mMeshes && NULL != this->pScene->mMeshes[0] &&
		NULL != this->pScene->mMeshes[0]->mTextureCoords[0]);

	aiVector3D* uv = this->pScene->mMeshes[0]->mTextureCoords[0];

	const float fY = (1.0f / height) + (1.0f / height) / (height-1);
	const float fX = (1.0f / width) + (1.0f / width) / (width-1);

	for (unsigned int y = 0; y < height;++y)
	{
		for (unsigned int x = 0; x < width;++x)
		{
			uv->y = 1.0f-fY*y;
			uv->x = fX*x;
			uv->z = 0.0f;
			++uv;
		}
	}
	return;
}
