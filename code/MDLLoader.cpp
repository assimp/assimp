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

/** @file Implementation of the main parts of the MDL importer class */

// internal headers
#include "AssimpPCH.h"

#include "MDLLoader.h"
#include "MDLDefaultColorMap.h"
#include "MD2FileData.h" 


using namespace Assimp;


// ------------------------------------------------------------------------------------------------
// macros used by the MDL7 loader

#if (!defined _AI_MDL7_ACCESS)
#	define _AI_MDL7_ACCESS(_data, _index, _limit, _type) \
	(*((const _type*)(((const char*)_data) + _index * _limit)))
#endif 
#if (!defined _AI_MDL7_ACCESS_PTR)
#	define _AI_MDL7_ACCESS_PTR(_data, _index, _limit, _type) \
	((BE_NCONST _type*)(((const char*)_data) + _index * _limit))
#endif
#if (!defined _AI_MDL7_ACCESS_VERT)
#	define _AI_MDL7_ACCESS_VERT(_data, _index, _limit) \
	_AI_MDL7_ACCESS(_data,_index,_limit,MDL::Vertex_MDL7)
#endif

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
  (void)pIOHandler; //this avoids the compiler warning of unused element
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
// Setup configuration properties
void MDLImporter::SetupProperties(const Importer* pImp)
{
	// The AI_CONFIG_IMPORT_MDL_KEYFRAME option overrides the
	// AI_CONFIG_IMPORT_GLOBAL_KEYFRAME option.
	if(0xffffffff == (this->configFrameID = pImp->GetPropertyInteger(
		AI_CONFIG_IMPORT_MDL_KEYFRAME,0xffffffff)))
	{
		this->configFrameID =  pImp->GetPropertyInteger(AI_CONFIG_IMPORT_GLOBAL_KEYFRAME,0);
	}
	this->configPalette =  pImp->GetPropertyString(AI_CONFIG_IMPORT_MDL_COLORMAP,"colormap.lmp");
}
// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void MDLImporter::InternReadFile( const std::string& pFile, 
								 aiScene* pScene, IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile));

	// Check whether we can read from the file
	if( file.get() == NULL)
		throw new ImportErrorException( "Failed to open MDL file " + pFile + ".");


	// this should work for all other types of MDL files, too ...
	// the quake header is one of the smallest, afaik
	this->iFileSize = (unsigned int)file->FileSize();
	if( this->iFileSize < sizeof(MDL::Header))
		throw new ImportErrorException( "MDL File is too small.");

	// allocate storage and copy the contents of the file to a memory buffer
	this->pScene = pScene;
	this->pIOHandler = pIOHandler;
	this->mBuffer = new unsigned char[this->iFileSize+1];
	file->Read( (void*)mBuffer, 1, this->iFileSize);

	// append a binary zero to the end of the buffer.
	// this is just for safety that string parsing routines
	// find the end of the buffer ...
	this->mBuffer[this->iFileSize] = '\0';
	uint32_t iMagicWord = *((uint32_t*)this->mBuffer);

	// determine the file subtype and call the appropriate member function
	try {

		// Original Quake1 format
		if (AI_MDL_MAGIC_NUMBER_BE == iMagicWord ||
			AI_MDL_MAGIC_NUMBER_LE == iMagicWord)
		{
			DefaultLogger::get()->debug("MDL subtype: Quake 1, magic word is IDPO");
			this->iGSFileVersion = 0;
			this->InternReadFile_Quake1();
		}
		// GameStudio A<old> MDL2 format - used by some test models that come with 3DGS
		else if (AI_MDL_MAGIC_NUMBER_BE_GS3 == iMagicWord ||
			AI_MDL_MAGIC_NUMBER_LE_GS3 == iMagicWord)
		{
			DefaultLogger::get()->debug("MDL subtype: 3D GameStudio A2, magic word is MDL2");
			this->iGSFileVersion = 2;
			this->InternReadFile_Quake1();
		}
		// GameStudio A4 MDL3 format
		else if (AI_MDL_MAGIC_NUMBER_BE_GS4 == iMagicWord ||
			AI_MDL_MAGIC_NUMBER_LE_GS4 == iMagicWord)
		{
			DefaultLogger::get()->debug("MDL subtype: 3D GameStudio A4, magic word is MDL3");
			this->iGSFileVersion = 3;
			this->InternReadFile_3DGS_MDL345();
		}
		// GameStudio A5+ MDL4 format
		else if (AI_MDL_MAGIC_NUMBER_BE_GS5a == iMagicWord ||
			AI_MDL_MAGIC_NUMBER_LE_GS5a == iMagicWord)
		{
			DefaultLogger::get()->debug("MDL subtype: 3D GameStudio A4, magic word is MDL4");
			this->iGSFileVersion = 4;
			this->InternReadFile_3DGS_MDL345();
		}
		// GameStudio A5+ MDL5 format
		else if (AI_MDL_MAGIC_NUMBER_BE_GS5b == iMagicWord ||
			AI_MDL_MAGIC_NUMBER_LE_GS5b == iMagicWord)
		{
			DefaultLogger::get()->debug("MDL subtype: 3D GameStudio A5, magic word is MDL5");
			this->iGSFileVersion = 5;
			this->InternReadFile_3DGS_MDL345();
		}
		// GameStudio A7 MDL7 format
		else if (AI_MDL_MAGIC_NUMBER_BE_GS7 == iMagicWord ||
			AI_MDL_MAGIC_NUMBER_LE_GS7 == iMagicWord)
		{
			DefaultLogger::get()->debug("MDL subtype: 3D GameStudio A7, magic word is MDL7");
			this->iGSFileVersion = 7;
			this->InternReadFile_3DGS_MDL7();
		}
		// IDST/IDSQ Format (CS:S/HL², etc ...)
		else if (AI_MDL_MAGIC_NUMBER_BE_HL2a == iMagicWord ||
			AI_MDL_MAGIC_NUMBER_LE_HL2a == iMagicWord ||
			AI_MDL_MAGIC_NUMBER_BE_HL2b == iMagicWord ||
			AI_MDL_MAGIC_NUMBER_LE_HL2b == iMagicWord)
		{
			DefaultLogger::get()->debug("MDL subtype: CS:S\\HL², magic word is IDST/IDSQ");
			this->iGSFileVersion = 0;
			this->InternReadFile_HL2();
		}
		else
		{
			// print the magic word to the logger
			char szBuffer[5];
			szBuffer[0] = ((char*)&iMagicWord)[0];
			szBuffer[1] = ((char*)&iMagicWord)[1];
			szBuffer[2] = ((char*)&iMagicWord)[2];
			szBuffer[3] = ((char*)&iMagicWord)[3];
			szBuffer[4] = '\0';

			// we're definitely unable to load this file
			throw new ImportErrorException( "Unknown MDL subformat " + pFile +
				". Magic word (" + szBuffer + ") is not known");
		}

	} 
	catch (ImportErrorException* ex) {
		delete[] this->mBuffer;
		AI_DEBUG_INVALIDATE_PTR(this->mBuffer);
		AI_DEBUG_INVALIDATE_PTR(this->pIOHandler);
		AI_DEBUG_INVALIDATE_PTR(this->pScene);
		throw ex;
	}

	// delete the file buffer and cleanup
	delete[] this->mBuffer;
	AI_DEBUG_INVALIDATE_PTR(this->mBuffer);
	AI_DEBUG_INVALIDATE_PTR(this->pIOHandler);
	AI_DEBUG_INVALIDATE_PTR(this->pScene);
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::SizeCheck(const void* szPos)
{
	if (!szPos || (const unsigned char*)szPos > this->mBuffer + this->iFileSize)
	{
		throw new ImportErrorException("Invalid MDL file. The file is too small "
			"or contains invalid data.");
	}
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::SizeCheck(const void* szPos, const char* szFile, unsigned int iLine)
{
	if (!szFile)return SizeCheck(szPos);
	if (!szPos || (const unsigned char*)szPos > this->mBuffer + this->iFileSize)
	{
		// remove a directory if there is one
		const char* szFilePtr = ::strrchr(szFile,'\\');
		if (!szFilePtr)
		{
			if(!(szFilePtr = ::strrchr(szFile,'/')))szFilePtr = szFile;
		}
		if (szFilePtr)++szFilePtr;

		char szBuffer[1024];
		::sprintf(szBuffer,"Invalid MDL file. The file is too small "
			"or contains invalid data (File: %s Line: %i)",szFilePtr,iLine);

		throw new ImportErrorException(szBuffer);
	}
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::ValidateHeader_Quake1(const MDL::Header* pcHeader)
{
	// some values may not be NULL
	if (!pcHeader->num_frames)
		throw new ImportErrorException( "[Quake 1 MDL] There are no frames in the file");

	if (!pcHeader->num_verts)
		throw new ImportErrorException( "[Quake 1 MDL] There are no vertices in the file");

	if (!pcHeader->num_tris)
		throw new ImportErrorException( "[Quake 1 MDL] There are no triangles in the file");

	// check whether the maxima are exceeded ...however, this applies for Quake 1 MDLs only
	if (!this->iGSFileVersion)
	{
		if (pcHeader->num_verts > AI_MDL_MAX_VERTS)
			DefaultLogger::get()->warn("Quake 1 MDL model has more than AI_MDL_MAX_VERTS vertices");

		if (pcHeader->num_tris > AI_MDL_MAX_TRIANGLES)
			DefaultLogger::get()->warn("Quake 1 MDL model has more than AI_MDL_MAX_TRIANGLES triangles");

		if (pcHeader->num_frames > AI_MDL_MAX_FRAMES)
			DefaultLogger::get()->warn("Quake 1 MDL model has more than AI_MDL_MAX_FRAMES frames");

		// (this does not apply for 3DGS MDLs)
		if (!this->iGSFileVersion && pcHeader->version != AI_MDL_VERSION)
			DefaultLogger::get()->warn("Quake 1 MDL model has an unknown version: AI_MDL_VERSION (=6) is "
				"the expected file format version");
		if(pcHeader->num_skins && (!pcHeader->skinwidth || !pcHeader->skinheight))
			DefaultLogger::get()->warn("Skin width or height are 0");
	}
}

#ifdef AI_BUILD_BIG_ENDIAN
// ------------------------------------------------------------------------------------------------
void FlipQuakeHeader(BE_NCONST MDL::Header* pcHeader)
{
	AI_SWAP4( pcHeader->ident);
	AI_SWAP4( pcHeader->version);
	AI_SWAP4( pcHeader->boundingradius);
	AI_SWAP4( pcHeader->flags);
	AI_SWAP4( pcHeader->num_frames);
	AI_SWAP4( pcHeader->num_skins);
	AI_SWAP4( pcHeader->num_tris);
	AI_SWAP4( pcHeader->num_verts);
	for (unsigned int i = 0; i < 3;++i)
	{
		AI_SWAP4( pcHeader->scale[i]);
		AI_SWAP4( pcHeader->translate[i]);
	}
	AI_SWAP4( pcHeader->size);
	AI_SWAP4( pcHeader->skinheight);
  AI_SWAP4( pcHeader->skinwidth); 
  AI_SWAP4( pcHeader->synctype); 
  
//	ByteSwap::Swap4(& pcHeader->skin);

}
#endif
// ------------------------------------------------------------------------------------------------
void MDLImporter::InternReadFile_Quake1( )
{
	ai_assert(NULL != pScene);

	BE_NCONST MDL::Header *pcHeader = (BE_NCONST MDL::Header*)this->mBuffer;

#ifdef AI_BUILD_BIG_ENDIAN
	FlipQuakeHeader(pcHeader);
#endif

	ValidateHeader_Quake1(pcHeader);

	// current cursor position in the file
	const unsigned char* szCurrent = (const unsigned char*)(pcHeader+1);

	// need to read all textures
	for (unsigned int i = 0; i < (unsigned int)pcHeader->num_skins;++i)
	{
		union{BE_NCONST MDL::Skin* pcSkin;BE_NCONST MDL::GroupSkin* pcGroupSkin;};
		pcSkin = (BE_NCONST MDL::Skin*)szCurrent;

		AI_SWAP4( pcSkin->group );

		// Quake 1 groupskins
		if (1 == pcSkin->group)
		{
			AI_SWAP4( pcGroupSkin->nb );

			// need to skip multiple images
			const unsigned int iNumImages = (unsigned int)pcGroupSkin->nb;
			szCurrent += sizeof(uint32_t) * 2;

			if (0 != iNumImages)	
			{
				if (!i) {
					// however, create only one output image (the first)
					this->CreateTextureARGB8_3DGS_MDL3(szCurrent + iNumImages * sizeof(float));
				}
				// go to the end of the skin section / the beginning of the next skin
				szCurrent += pcHeader->skinheight * pcHeader->skinwidth +
					sizeof(float) * iNumImages;
			}
		}
		// 3DGS has a few files that are using other 3DGS like texture formats here
		else
		{
			szCurrent += sizeof(uint32_t);
			unsigned int iSkip = i ? 0xffffffff : 0;
			this->CreateTexture_3DGS_MDL4(szCurrent,pcSkin->group,&iSkip);
			szCurrent += iSkip;
		}
	}
	// get a pointer to the texture coordinates
	BE_NCONST MDL::TexCoord* pcTexCoords = (BE_NCONST MDL::TexCoord*)szCurrent;
	szCurrent += sizeof(MDL::TexCoord) * pcHeader->num_verts;

	// get a pointer to the triangles
	BE_NCONST MDL::Triangle* pcTriangles = (BE_NCONST MDL::Triangle*)szCurrent;
	szCurrent += sizeof(MDL::Triangle) * pcHeader->num_tris;
	VALIDATE_FILE_SIZE(szCurrent);

	// now get a pointer to the first frame in the file
	BE_NCONST MDL::Frame* pcFrames = (BE_NCONST MDL::Frame*)szCurrent;
	BE_NCONST MDL::SimpleFrame* pcFirstFrame;

	if (0 == pcFrames->type)
	{
		// get address of single frame
		pcFirstFrame = &pcFrames->frame;
	}
	else
	{
		// get the first frame in the group
		BE_NCONST MDL::GroupFrame* pcFrames2 = (BE_NCONST MDL::GroupFrame*)pcFrames;
		pcFirstFrame = (BE_NCONST MDL::SimpleFrame*)(&pcFrames2->time + pcFrames->type);
	}
	BE_NCONST MDL::Vertex* pcVertices = (BE_NCONST MDL::Vertex*) ((pcFirstFrame->name) +
		sizeof(pcFirstFrame->name));

	VALIDATE_FILE_SIZE((const unsigned char*)(pcVertices + pcHeader->num_verts));

#ifdef AI_BUILD_BIG_ENDIAN

	for (int i = 0; i<pcHeader->num_verts;++i)
	{
		AI_SWAP4( pcTexCoords[i].onseam );
		AI_SWAP4( pcTexCoords[i].s );
		AI_SWAP4( pcTexCoords[i].t );
	}

	for (int i = 0; i<pcHeader->num_tris;++i)
	{
		AI_SWAP4( pcTriangles[i].facesfront);
		AI_SWAP4( pcTriangles[i].vertex[0]);
		AI_SWAP4( pcTriangles[i].vertex[1]);
		AI_SWAP4( pcTriangles[i].vertex[2]);
	}
  

#endif

	// setup materials
	this->SetupMaterialProperties_3DGS_MDL5_Quake1();

	// allocate enough storage to hold all vertices and triangles
	aiMesh* pcMesh = new aiMesh();
	
	pcMesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
	pcMesh->mNumVertices = pcHeader->num_tris * 3;
	pcMesh->mNumFaces = pcHeader->num_tris;
	pcMesh->mVertices = new aiVector3D[pcMesh->mNumVertices];
	pcMesh->mTextureCoords[0] = new aiVector3D[pcMesh->mNumVertices];
	pcMesh->mFaces = new aiFace[pcMesh->mNumFaces];
	pcMesh->mNormals = new aiVector3D[pcMesh->mNumVertices];
	pcMesh->mNumUVComponents[0] = 2;

	// there won't be more than one mesh inside the file
	pScene->mRootNode = new aiNode();
	pScene->mRootNode->mNumMeshes = 1;
	pScene->mRootNode->mMeshes = new unsigned int[1];
	pScene->mRootNode->mMeshes[0] = 0;
	pScene->mNumMeshes = 1;
	pScene->mMeshes = new aiMesh*[1];
	pScene->mMeshes[0] = pcMesh;

	// now iterate through all triangles
	unsigned int iCurrent = 0;
	for (unsigned int i = 0; i < (unsigned int) pcHeader->num_tris;++i)
	{
		pcMesh->mFaces[i].mIndices = new unsigned int[3];
		pcMesh->mFaces[i].mNumIndices = 3;

		unsigned int iTemp = iCurrent;
		for (unsigned int c = 0; c < 3;++c,++iCurrent)
		{
			pcMesh->mFaces[i].mIndices[c] = iCurrent;

			// read vertices
			unsigned int iIndex = pcTriangles->vertex[c];
			if (iIndex >= (unsigned int)pcHeader->num_verts)
			{
				iIndex = pcHeader->num_verts-1;
				DefaultLogger::get()->warn("Index overflow in Q1-MDL vertex list.");
			}

			aiVector3D& vec = pcMesh->mVertices[iCurrent];
			vec.x = (float)pcVertices[iIndex].v[0] * pcHeader->scale[0];
			vec.x += pcHeader->translate[0];

			vec.y = (float)pcVertices[iIndex].v[1] * pcHeader->scale[1];
			vec.y += pcHeader->translate[1];
			vec.y *= -1.0f;

			vec.z = (float)pcVertices[iIndex].v[2] * pcHeader->scale[2];
			vec.z += pcHeader->translate[2];

			// read the normal vector from the precalculated normal table
			MD2::LookupNormalIndex(pcVertices[iIndex].normalIndex,pcMesh->mNormals[iCurrent]);
			pcMesh->mNormals[iCurrent].y *= -1.0f;

			// read texture coordinates
			float s = (float)pcTexCoords[iIndex].s;
			float t = (float)pcTexCoords[iIndex].t;

			// translate texture coordinates
			if (0 == pcTriangles->facesfront &&
				0 != pcTexCoords[iIndex].onseam)
			{
				s += pcHeader->skinwidth * 0.5f; 
			}

			// Scale s and t to range from 0.0 to 1.0 
			pcMesh->mTextureCoords[0][iCurrent].x = (s + 0.5f) / pcHeader->skinwidth;
			pcMesh->mTextureCoords[0][iCurrent].y = 1.0f-(t + 0.5f) / pcHeader->skinheight;

		}
		pcMesh->mFaces[i].mIndices[0] = iTemp+2;
		pcMesh->mFaces[i].mIndices[1] = iTemp+1;
		pcMesh->mFaces[i].mIndices[2] = iTemp+0;
		pcTriangles++;
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::SetupMaterialProperties_3DGS_MDL5_Quake1( )
{
	// get a pointer to the file header
	const MDL::Header* const pcHeader = (const MDL::Header*)this->mBuffer;

	// allocate ONE material
	pScene->mMaterials = new aiMaterial*[1];
	pScene->mMaterials[0] = new MaterialHelper();
	pScene->mNumMaterials = 1;

	// setup the material properties
	const int iMode = (int)aiShadingMode_Gouraud;
	MaterialHelper* const pcHelper = (MaterialHelper*)pScene->mMaterials[0];
	pcHelper->AddProperty<int>(&iMode, 1, AI_MATKEY_SHADING_MODEL);

	aiColor4D clr;
	if (0 != pcHeader->num_skins && pScene->mNumTextures)
	{
		// can we replace the texture with a single color?
		clr = this->ReplaceTextureWithColor(pScene->mTextures[0]);
		if (is_not_qnan(clr.r))
		{
			delete pScene->mTextures[0];
			delete[] pScene->mTextures;
			pScene->mNumTextures = 0;
		}
		else
		{
			clr.b = clr.a = clr.g = clr.r = 1.0f;
			aiString szString;
			::memcpy(szString.data,AI_MAKE_EMBEDDED_TEXNAME(0),3);
			szString.length = 2;
			pcHelper->AddProperty(&szString,AI_MATKEY_TEXTURE_DIFFUSE(0));
		}
	}

	pcHelper->AddProperty<aiColor4D>(&clr, 1,AI_MATKEY_COLOR_DIFFUSE);
	pcHelper->AddProperty<aiColor4D>(&clr, 1,AI_MATKEY_COLOR_SPECULAR);

	clr.r *= 0.05f;clr.g *= 0.05f;
	clr.b *= 0.05f;clr.a  = 1.0f;
	pcHelper->AddProperty<aiColor4D>(&clr, 1,AI_MATKEY_COLOR_AMBIENT);
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::InternReadFile_3DGS_MDL345( )
{
	ai_assert(NULL != pScene);

	// the header of MDL 3/4/5 is nearly identical to the original Quake1 header
	BE_NCONST MDL::Header *pcHeader = (BE_NCONST MDL::Header*)this->mBuffer;
#ifdef AI_BUILD_BIG_ENDIAN
	FlipQuakeHeader(pcHeader);
#endif
	this->ValidateHeader_Quake1(pcHeader);

	// current cursor position in the file
	const unsigned char* szCurrent = (const unsigned char*)(pcHeader+1);

	// need to read all textures
	for (unsigned int i = 0; i < (unsigned int)pcHeader->num_skins;++i)
	{
		BE_NCONST MDL::Skin* pcSkin;
		pcSkin = (BE_NCONST  MDL::Skin*)szCurrent;
		AI_SWAP4( pcSkin->group);
		// create one output image
		unsigned int iSkip = i ? 0xffffffff : 0;
		if (5 <= this->iGSFileVersion)
		{
			// MDL5 format could contain MIPmaps
			this->CreateTexture_3DGS_MDL5((unsigned char*)pcSkin + sizeof(uint32_t),
				pcSkin->group,&iSkip);
		}
		else
		{
			this->CreateTexture_3DGS_MDL4((unsigned char*)pcSkin + sizeof(uint32_t),
				pcSkin->group,&iSkip);
		}
		// need to skip one image
		szCurrent += iSkip + sizeof(uint32_t);
		
	}
	// get a pointer to the texture coordinates
	BE_NCONST MDL::TexCoord_MDL3* pcTexCoords = (BE_NCONST MDL::TexCoord_MDL3*)szCurrent;
	szCurrent += sizeof(MDL::TexCoord_MDL3) * pcHeader->synctype;

	// NOTE: for MDLn formats "synctype" corresponds to the number of UV coords

	// get a pointer to the triangles
	BE_NCONST MDL::Triangle_MDL3* pcTriangles = (BE_NCONST MDL::Triangle_MDL3*)szCurrent;
	szCurrent += sizeof(MDL::Triangle_MDL3) * pcHeader->num_tris;

#ifdef AI_BUILD_BIG_ENDIAN

	for (int i = 0; i<pcHeader->synctype;++i)
	{
		AI_SWAP2( pcTexCoords[i].u );
		AI_SWAP2( pcTexCoords[i].v );
	}

	for (int i = 0; i<pcHeader->num_tris;++i)
	{
		AI_SWAP2( pcTriangles[i].index_xyz[0]);
		AI_SWAP2( pcTriangles[i].index_xyz[1]);
		AI_SWAP2( pcTriangles[i].index_xyz[2]);
		AI_SWAP2( pcTriangles[i].index_uv[0]);
		AI_SWAP2( pcTriangles[i].index_uv[1]);
		AI_SWAP2( pcTriangles[i].index_uv[2]);
	}

#endif

	VALIDATE_FILE_SIZE(szCurrent);

	// setup materials
	this->SetupMaterialProperties_3DGS_MDL5_Quake1();

	// allocate enough storage to hold all vertices and triangles
	aiMesh* pcMesh = new aiMesh();
	pcMesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

	pcMesh->mNumVertices = pcHeader->num_tris * 3;
	pcMesh->mNumFaces = pcHeader->num_tris;
	pcMesh->mFaces = new aiFace[pcMesh->mNumFaces];

	// there won't be more than one mesh inside the file
	pScene->mRootNode = new aiNode();
	pScene->mRootNode->mNumMeshes = 1;
	pScene->mRootNode->mMeshes = new unsigned int[1];
	pScene->mRootNode->mMeshes[0] = 0;
	pScene->mNumMeshes = 1;
	pScene->mMeshes = new aiMesh*[1];
	pScene->mMeshes[0] = pcMesh;

	// allocate output storage
	pcMesh->mNumVertices = (unsigned int)pcHeader->num_tris*3;
	pcMesh->mVertices = new aiVector3D[pcMesh->mNumVertices];
	pcMesh->mNormals = new aiVector3D[pcMesh->mNumVertices];

	if (pcHeader->synctype)
	{
		pcMesh->mTextureCoords[0] = new aiVector3D[pcMesh->mNumVertices];
		pcMesh->mNumUVComponents[0] = 2;
	}

	// now get a pointer to the first frame in the file
	BE_NCONST MDL::Frame* pcFrames = (BE_NCONST MDL::Frame*)szCurrent;
  AI_SWAP4(pcFrames->type);

	// byte packed vertices
	// BIG TODO: these two snippets are nearly totally identical ...
	// ***********************************************************************
	if (0 == pcFrames->type || 3 >= this->iGSFileVersion)
	{
		const MDL::SimpleFrame* pcFirstFrame = (const MDL::SimpleFrame*)(szCurrent + sizeof(uint32_t));

		// get a pointer to the vertices
		const MDL::Vertex* pcVertices = (const MDL::Vertex*) ((pcFirstFrame->name) 
			+ sizeof(pcFirstFrame->name));

		VALIDATE_FILE_SIZE(pcVertices + pcHeader->num_verts);

		// now iterate through all triangles
		unsigned int iCurrent = 0;
		for (unsigned int i = 0; i < (unsigned int) pcHeader->num_tris;++i)
		{
			pcMesh->mFaces[i].mIndices = new unsigned int[3];
			pcMesh->mFaces[i].mNumIndices = 3;

			unsigned int iTemp = iCurrent;
			for (unsigned int c = 0; c < 3;++c,++iCurrent)
			{
				// read vertices
				unsigned int iIndex = pcTriangles->index_xyz[c];
				if (iIndex >= (unsigned int)pcHeader->num_verts)
				{
					iIndex = pcHeader->num_verts-1;
					DefaultLogger::get()->warn("Index overflow in MDLn vertex list");
				}

				aiVector3D& vec = pcMesh->mVertices[iCurrent];
				vec.x = (float)pcVertices[iIndex].v[0] * pcHeader->scale[0];
				vec.x += pcHeader->translate[0];

				vec.y = (float)pcVertices[iIndex].v[1] * pcHeader->scale[1];
				vec.y += pcHeader->translate[1];
				vec.y *= -1.0f;

				vec.z = (float)pcVertices[iIndex].v[2] * pcHeader->scale[2];
				vec.z += pcHeader->translate[2];

				// read the normal vector from the precalculated normal table
				MD2::LookupNormalIndex(pcVertices[iIndex].normalIndex,pcMesh->mNormals[iCurrent]);
				pcMesh->mNormals[iCurrent].y *= -1.0f;

				// read texture coordinates
				if (pcHeader->synctype)
				{
					this->ImportUVCoordinate_3DGS_MDL345(pcMesh->mTextureCoords[0][iCurrent],
						pcTexCoords,pcTriangles->index_uv[c]);
				}
			}
			pcMesh->mFaces[i].mIndices[0] = iTemp+2;
			pcMesh->mFaces[i].mIndices[1] = iTemp+1;
			pcMesh->mFaces[i].mIndices[2] = iTemp+0;
			pcTriangles++;
		}

	}
	// short packed vertices 
	// ***********************************************************************
	else
	{
		// now get a pointer to the first frame in the file
		const MDL::SimpleFrame_MDLn_SP* pcFirstFrame = (const MDL::SimpleFrame_MDLn_SP*) (szCurrent + sizeof(uint32_t));

		// get a pointer to the vertices
		const MDL::Vertex_MDL4* pcVertices = (const MDL::Vertex_MDL4*) ((pcFirstFrame->name) +
			sizeof(pcFirstFrame->name));

		VALIDATE_FILE_SIZE(pcVertices + pcHeader->num_verts);

		// now iterate through all triangles
		unsigned int iCurrent = 0;
		for (unsigned int i = 0; i < (unsigned int) pcHeader->num_tris;++i)
		{
			pcMesh->mFaces[i].mIndices = new unsigned int[3];
			pcMesh->mFaces[i].mNumIndices = 3;

			unsigned int iTemp = iCurrent;
			for (unsigned int c = 0; c < 3;++c,++iCurrent)
			{
				// read vertices
				unsigned int iIndex = pcTriangles->index_xyz[c];
				if (iIndex >= (unsigned int)pcHeader->num_verts)
				{
					iIndex = pcHeader->num_verts-1;
					DefaultLogger::get()->warn("Index overflow in MDLn vertex list");
				}

				aiVector3D& vec = pcMesh->mVertices[iCurrent];
				vec.x = (float)pcVertices[iIndex].v[0] * pcHeader->scale[0];
				vec.x += pcHeader->translate[0];

				vec.y = (float)pcVertices[iIndex].v[1] * pcHeader->scale[1];
				vec.y += pcHeader->translate[1];
				vec.y *= -1.0f;

				vec.z = (float)pcVertices[iIndex].v[2] * pcHeader->scale[2];
				vec.z += pcHeader->translate[2];

				// read the normal vector from the precalculated normal table
				MD2::LookupNormalIndex(pcVertices[iIndex].normalIndex,pcMesh->mNormals[iCurrent]);
				pcMesh->mNormals[iCurrent].y *= -1.0f;

				// read texture coordinates
				if (pcHeader->synctype)
				{
					this->ImportUVCoordinate_3DGS_MDL345(pcMesh->mTextureCoords[0][iCurrent],
						pcTexCoords,pcTriangles->index_uv[c]);
				}
			}
			pcMesh->mFaces[i].mIndices[0] = iTemp+2;
			pcMesh->mFaces[i].mIndices[1] = iTemp+1;
			pcMesh->mFaces[i].mIndices[2] = iTemp+0;
			pcTriangles++;
		}
	}

	// For MDL5 we will need to build valid texture coordinates
	// basing upon the file loaded (only support one file as skin)
	if (0x5 == this->iGSFileVersion)
		this->CalculateUVCoordinates_MDL5();
	return;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::ImportUVCoordinate_3DGS_MDL345( 
	aiVector3D& vOut,
	const MDL::TexCoord_MDL3* pcSrc, 
	unsigned int iIndex)
{
	ai_assert(NULL != pcSrc);

	const MDL::Header* const pcHeader = (const MDL::Header*)this->mBuffer;

	// validate UV indices
	if (iIndex >= (unsigned int) pcHeader->synctype)
	{
		iIndex = pcHeader->synctype-1;
		DefaultLogger::get()->warn("Index overflow in MDLn UV coord list");
	}

	float s = (float)pcSrc[iIndex].u;
	float t = (float)pcSrc[iIndex].v;

	// Scale s and t to range from 0.0 to 1.0 
	if (0x5 != this->iGSFileVersion)
	{
		s = (s + 0.5f) / pcHeader->skinwidth;
		t = 1.0f-(t + 0.5f) / pcHeader->skinheight;
	}

	vOut.x = s;
	vOut.y = t;
	vOut.z = 0.0f;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::CalculateUVCoordinates_MDL5()
{
	const MDL::Header* const pcHeader = (const MDL::Header*)this->mBuffer;
	if (pcHeader->num_skins && this->pScene->mNumTextures)
	{
		const aiTexture* pcTex = this->pScene->mTextures[0];

		// if the file is loaded in DDS format: get the size of the
		// texture from the header of the DDS file
		// skip three DWORDs and read first height, then the width
		unsigned int iWidth, iHeight;
		if (!pcTex->mHeight)
		{
			const uint32_t* piPtr = (uint32_t*)pcTex->pcData;

			piPtr += 3;
			iHeight = (unsigned int)*piPtr++;
			iWidth  = (unsigned int)*piPtr;
			if (!iHeight || !iWidth)
			{
				DefaultLogger::get()->warn("Either the width or the height of the "
					"embedded DDS texture is zero. Unable to compute final texture "
					"coordinates. The texture coordinates remain in their original "
					"0-x/0-y (x,y = texture size) range.");
				iWidth = 1;
				iHeight = 1;
			}
		}
		else
		{
			iWidth = pcTex->mWidth;
			iHeight = pcTex->mHeight;
		}

		if (1 != iWidth || 1 != iHeight)
		{
			const float fWidth = (float)iWidth;
			const float fHeight = (float)iHeight;
			aiMesh* pcMesh = this->pScene->mMeshes[0];
			for (unsigned int i = 0; i < pcMesh->mNumVertices;++i)
			{
				// width and height can't be 0 here
				pcMesh->mTextureCoords[0][i].x /= fWidth;
				pcMesh->mTextureCoords[0][i].y /= fHeight;
				pcMesh->mTextureCoords[0][i].y = 1.0f - pcMesh->mTextureCoords[0][i].y; // DX to OGL
			}
		}
	}
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::ValidateHeader_3DGS_MDL7(const MDL::Header_MDL7* pcHeader)
{
	ai_assert(NULL != pcHeader);

	if (sizeof(MDL::ColorValue_MDL7) != pcHeader->colorvalue_stc_size)
	{
		throw new ImportErrorException( 
			"[3DGS MDL7] sizeof(MDL::ColorValue_MDL7) != pcHeader->colorvalue_stc_size"
			);
	}
	if (sizeof(MDL::TexCoord_MDL7) != pcHeader->skinpoint_stc_size)
	{
		throw new ImportErrorException( 
			"[3DGS MDL7] sizeof(MDL::TexCoord_MDL7) != pcHeader->skinpoint_stc_size"
			);
	}
	if (sizeof(MDL::Skin_MDL7) != pcHeader->skin_stc_size)
	{
		throw new ImportErrorException( 
			"sizeof(MDL::Skin_MDL7) != pcHeader->skin_stc_size"
			);
	}

	// if there are no groups ... how should we load such a file?
	if(!pcHeader->groups_num)
	{
		// LOG
		throw new ImportErrorException( "[3DGS MDL7] No frames found");
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::CalcAbsBoneMatrices_3DGS_MDL7(MDL::IntBone_MDL7** apcOutBones)
{
	const MDL::Header_MDL7 *pcHeader = (const MDL::Header_MDL7*)this->mBuffer;
  const MDL::Bone_MDL7* pcBones = (const MDL::Bone_MDL7*)(pcHeader+1);
	ai_assert(NULL != apcOutBones);

	// first find the bone that has NO parent, calculate the
	// animation matrix for it, then go on and search for the next parent
	// index (0) and so on until we can't find a new node.
	uint16_t iParent = 0xffff;
	uint32_t iIterations = 0;
	while (iIterations++ < pcHeader->bones_num)
	{
		for (uint32_t iBone = 0; iBone < pcHeader->bones_num;++iBone)
		{
			BE_NCONST MDL::Bone_MDL7* pcBone = _AI_MDL7_ACCESS_PTR(pcBones,iBone,
				pcHeader->bone_stc_size,MDL::Bone_MDL7);
        
      AI_SWAP2(pcBone->parent_index);
      AI_SWAP4(pcBone->x);
      AI_SWAP4(pcBone->y);
      AI_SWAP4(pcBone->z);
	
			if (iParent == pcBone->parent_index)
			{
				// extract from MDL7 readme ...
				/************************************************************
				The animation matrix is then calculated the following way:

				vector3 bPos = <absolute bone position>
				matrix44 laM;   // local animation matrix
				sphrvector key_rotate = <bone rotation>
		
				matrix44 m1,m2;
				create_trans_matrix(m1, -bPos.x, -bPos.y, -bPos.z);
				create_trans_matrix(m2, -bPos.x, -bPos.y, -bPos.z);

				create_rotation_matrix(laM,key_rotate);

				laM = sm1 * laM;
				laM = laM * sm2;
				*************************************************************/

				MDL::IntBone_MDL7* const pcOutBone = apcOutBones[iBone];

				// store the parent index of the bone
				pcOutBone->iParent = pcBone->parent_index;
				if (0xffff != iParent)
				{
					const MDL::IntBone_MDL7* pcParentBone = apcOutBones[iParent];
					pcOutBone->mOffsetMatrix.a4 = -pcParentBone->vPosition.x;
					pcOutBone->mOffsetMatrix.b4 = -pcParentBone->vPosition.y;
					pcOutBone->mOffsetMatrix.c4 = -pcParentBone->vPosition.z;
				}
				pcOutBone->vPosition.x = pcBone->x; 
				pcOutBone->vPosition.y = pcBone->y;
				pcOutBone->vPosition.z = pcBone->z;
				pcOutBone->mOffsetMatrix.a4 -= pcBone->x;
				pcOutBone->mOffsetMatrix.b4 -= pcBone->y;
				pcOutBone->mOffsetMatrix.c4 -= pcBone->z;

				if (AI_MDL7_BONE_STRUCT_SIZE__NAME_IS_NOT_THERE == pcHeader->bone_stc_size)
				{
					// no real name for our poor bone is specified :-(	
					pcOutBone->mName.length = ::sprintf(pcOutBone->mName.data,
						"UnnamedBone_%i",iBone);
				}
				else
				{
					// make sure we won't run over the buffer's end if there is no
					// terminal 0 character (however the documentation says there
					// should be one)
					uint32_t iMaxLen = pcHeader->bone_stc_size-16;
					for (uint32_t qq = 0; qq < iMaxLen;++qq)
					{
						if (!pcBone->name[qq])
						{
							iMaxLen = qq;
							break;
						}
					}

					// store the name of the bone
					pcOutBone->mName.length = (size_t)iMaxLen;
					::memcpy(pcOutBone->mName.data,pcBone->name,pcOutBone->mName.length);
					pcOutBone->mName.data[pcOutBone->mName.length] = '\0';
				}
			}
		}
		++iParent;
	}
}
// ------------------------------------------------------------------------------------------------
MDL::IntBone_MDL7** MDLImporter::LoadBones_3DGS_MDL7()
{
  const MDL::Header_MDL7 *pcHeader = (const MDL::Header_MDL7*)this->mBuffer;
	if (pcHeader->bones_num)
	{
		// validate the size of the bone data structure in the file
		if (AI_MDL7_BONE_STRUCT_SIZE__NAME_IS_20_CHARS != pcHeader->bone_stc_size &&
			AI_MDL7_BONE_STRUCT_SIZE__NAME_IS_32_CHARS != pcHeader->bone_stc_size &&
			AI_MDL7_BONE_STRUCT_SIZE__NAME_IS_NOT_THERE != pcHeader->bone_stc_size)
		{
			DefaultLogger::get()->warn("Unknown size of bone data structure");
			return NULL;
		}

		MDL::IntBone_MDL7** apcBonesOut = new MDL::IntBone_MDL7*[pcHeader->bones_num];
		for (uint32_t crank = 0; crank < pcHeader->bones_num;++crank)
			apcBonesOut[crank] = new MDL::IntBone_MDL7();

		// and calculate absolute bone offset matrices ...
		this->CalcAbsBoneMatrices_3DGS_MDL7(apcBonesOut);
		return apcBonesOut;
	}
	return NULL;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::ReadFaces_3DGS_MDL7(
	const MDL::IntGroupInfo_MDL7& groupInfo,
	MDL::IntGroupData_MDL7& groupData)
{
	const MDL::Header_MDL7 *pcHeader = (const MDL::Header_MDL7*)this->mBuffer; 
  BE_NCONST MDL::Triangle_MDL7* pcGroupTris = groupInfo.pcGroupTris;

	// iterate through all triangles and build valid display lists
	unsigned int iOutIndex = 0;
	for (unsigned int iTriangle = 0; iTriangle < (unsigned int)groupInfo.pcGroup->numtris; ++iTriangle)
	{  
    AI_SWAP2(pcGroupTris->v_index[0]);
    AI_SWAP2(pcGroupTris->v_index[1]);
    AI_SWAP2(pcGroupTris->v_index[2]);
  
		// iterate through all indices of the current triangle
		for (unsigned int c = 0; c < 3;++c,++iOutIndex)
		{
			// validate the vertex index
			unsigned int iIndex = pcGroupTris->v_index[c];
			if(iIndex > (unsigned int)groupInfo.pcGroup->numverts)
			{
				// (we might need to read this section a second time - to process
				//  frame vertices correctly)
				const_cast<MDL::Triangle_MDL7*>(pcGroupTris)->v_index[c] = iIndex = groupInfo.pcGroup->numverts-1;
				DefaultLogger::get()->warn("Index overflow in MDL7 vertex list");
			}

			// write the output face index
			groupData.pcFaces[iTriangle].mIndices[2-c] = iOutIndex;

			aiVector3D& vPosition = groupData.vPositions[ iOutIndex ];
			vPosition.x = _AI_MDL7_ACCESS_VERT(groupInfo.pcGroupVerts,iIndex, pcHeader->mainvertex_stc_size) .x;
			vPosition.y = -1.0f*_AI_MDL7_ACCESS_VERT(groupInfo.pcGroupVerts,iIndex,pcHeader->mainvertex_stc_size) .y;
			vPosition.z = _AI_MDL7_ACCESS_VERT(groupInfo.pcGroupVerts,iIndex,pcHeader->mainvertex_stc_size) .z;

			// if we have bones, save the index
			if (!groupData.aiBones.empty())
				groupData.aiBones[iOutIndex] = _AI_MDL7_ACCESS_VERT(groupInfo.pcGroupVerts,
				iIndex,pcHeader->mainvertex_stc_size).vertindex;

			// now read the normal vector
			if (AI_MDL7_FRAMEVERTEX030305_STCSIZE <= pcHeader->mainvertex_stc_size)
			{
				// read the full normal vector
				aiVector3D& vNormal = groupData.vNormals[ iOutIndex ];
				vNormal.x = _AI_MDL7_ACCESS_VERT(groupInfo.pcGroupVerts,iIndex,pcHeader->mainvertex_stc_size) .norm[0];
        AI_SWAP4(vNormal.x);    
				vNormal.y = -1.0f*_AI_MDL7_ACCESS_VERT(groupInfo.pcGroupVerts,iIndex,pcHeader->mainvertex_stc_size) .norm[1];
        AI_SWAP4(vNormal.y);    
				vNormal.z = _AI_MDL7_ACCESS_VERT(groupInfo.pcGroupVerts,iIndex,pcHeader->mainvertex_stc_size) .norm[2];
        AI_SWAP4(vNormal.z);    
			}
			else if (AI_MDL7_FRAMEVERTEX120503_STCSIZE <= pcHeader->mainvertex_stc_size)
			{
				// read the normal vector from Quake2's smart table
				aiVector3D& vNormal = groupData.vNormals[ iOutIndex ];
				MD2::LookupNormalIndex(_AI_MDL7_ACCESS_VERT(groupInfo.pcGroupVerts,iIndex,
					pcHeader->mainvertex_stc_size) .norm162index,vNormal);
				vNormal.y *= -1.0f;
			}
			// validate and process the first uv coordinate set
			// *************************************************************
			if (pcHeader->triangle_stc_size >= AI_MDL7_TRIANGLE_STD_SIZE_ONE_UV)
			{
				if (groupInfo.pcGroup->num_stpts)
				{
					AI_SWAP2(pcGroupTris->skinsets[0].st_index[0]); 
          AI_SWAP2(pcGroupTris->skinsets[0].st_index[1]);
          AI_SWAP2(pcGroupTris->skinsets[0].st_index[2]);
      
          
          iIndex = pcGroupTris->skinsets[0].st_index[c];
					if(iIndex > (unsigned int)groupInfo.pcGroup->num_stpts)
					{
						iIndex = groupInfo.pcGroup->num_stpts-1;
						DefaultLogger::get()->warn("Index overflow in MDL7 UV coordinate list (#1)");
					}

					float u = groupInfo.pcGroupUVs[iIndex].u;
					float v = 1.0f-groupInfo.pcGroupUVs[iIndex].v; // DX to OGL

					groupData.vTextureCoords1[iOutIndex].x = u;
					groupData.vTextureCoords1[iOutIndex].y = v;
				}
				// assign the material index, but only if it is existing
				if (pcHeader->triangle_stc_size >= AI_MDL7_TRIANGLE_STD_SIZE_ONE_UV_WITH_MATINDEX){
			    AI_SWAP4(pcGroupTris->skinsets[0].material);		
          groupData.pcFaces[iTriangle].iMatIndex[0] = pcGroupTris->skinsets[0].material;
        }     
			}
			// validate and process the second uv coordinate set
			// *************************************************************
			if (pcHeader->triangle_stc_size >= AI_MDL7_TRIANGLE_STD_SIZE_TWO_UV)
			{
				if (groupInfo.pcGroup->num_stpts)
				{
			    AI_SWAP2(pcGroupTris->skinsets[1].st_index[0]); 
          AI_SWAP2(pcGroupTris->skinsets[1].st_index[1]);
          AI_SWAP2(pcGroupTris->skinsets[1].st_index[2]);
          AI_SWAP4(pcGroupTris->skinsets[1].material);		
          
          iIndex = pcGroupTris->skinsets[1].st_index[c];
					if(iIndex > (unsigned int)groupInfo.pcGroup->num_stpts)
					{
						iIndex = groupInfo.pcGroup->num_stpts-1;
						DefaultLogger::get()->warn("Index overflow in MDL7 UV coordinate list (#2)");
					}

					float u = groupInfo.pcGroupUVs[ iIndex ].u;
					float v = 1.0f-groupInfo.pcGroupUVs[ iIndex ].v;

					groupData.vTextureCoords2[ iOutIndex ].x = u;
					groupData.vTextureCoords2[ iOutIndex ].y = v; // DX to OGL

					// check whether we do really need the second texture
					// coordinate set ... wastes memory and loading time
					if (0 != iIndex && (u != groupData.vTextureCoords1[ iOutIndex ].x ||
						v != groupData.vTextureCoords1[ iOutIndex ].y ) )
						groupData.bNeed2UV = true;
				
					// if the material differs, we need a second skin, too
					if (pcGroupTris->skinsets[ 1 ].material != pcGroupTris->skinsets[ 0 ].material)
						groupData.bNeed2UV = true;
				}
				// assign the material index
				groupData.pcFaces[ iTriangle ].iMatIndex[ 1 ] = pcGroupTris->skinsets[ 1 ].material;
			}
		}
		// get the next triangle in the list
		pcGroupTris = (BE_NCONST MDL::Triangle_MDL7*)((const char*)pcGroupTris + 
			pcHeader->triangle_stc_size);
	}
}
// ------------------------------------------------------------------------------------------------
bool MDLImporter::ProcessFrames_3DGS_MDL7(const MDL::IntGroupInfo_MDL7& groupInfo,
	MDL::IntGroupData_MDL7& groupData,
	MDL::IntSharedData_MDL7& shared,
	const unsigned char* szCurrent,
	const unsigned char** szCurrentOut)
{
	ai_assert(NULL != szCurrent && NULL != szCurrentOut);
  
  const MDL::Header_MDL7 *pcHeader = (const MDL::Header_MDL7*)this->mBuffer;

	// if we have no bones we can simply skip all frames,
	// otherwise we'll need to process them.
	// FIX: If we need another frame than the first we must apply frame vertex replacements ...
	for(unsigned int iFrame = 0; iFrame < (unsigned int)groupInfo.pcGroup->numframes;++iFrame)
	{
		MDL::IntFrameInfo_MDL7 frame ((BE_NCONST MDL::Frame_MDL7*)szCurrent,iFrame);
    
    AI_SWAP4(frame.pcFrame->vertices_count);     
    AI_SWAP4(frame.pcFrame->transmatrix_count);

		const unsigned int iAdd = pcHeader->frame_stc_size + 
			frame.pcFrame->vertices_count * pcHeader->framevertex_stc_size +
			frame.pcFrame->transmatrix_count * pcHeader->bonetrans_stc_size;

		if (((const char*)szCurrent - (const char*)pcHeader) + iAdd > (unsigned int)pcHeader->data_size)
		{
			DefaultLogger::get()->warn("Index overflow in frame area. "
				"Ignoring all frames and all further mesh groups, too.");

			// don't parse more groups if we can't even read one
			// FIXME: sometimes this seems to occur even for valid files ...
			*szCurrentOut = szCurrent;
			return false;
		}
		// our output frame?
		if (configFrameID == iFrame)
		{
			BE_NCONST MDL::Vertex_MDL7* pcFrameVertices = (BE_NCONST MDL::Vertex_MDL7*)(szCurrent+pcHeader->frame_stc_size);
      
      for (unsigned int qq = 0; qq < frame.pcFrame->vertices_count;++qq)
			{
				// I assume this are simple replacements for normal
				// vertices, the bone index serving as the index of the
				// vertex to be replaced.
				uint16_t iIndex = _AI_MDL7_ACCESS(pcFrameVertices,qq,
					pcHeader->framevertex_stc_size,MDL::Vertex_MDL7).vertindex;
        AI_SWAP2(iIndex);
				if (iIndex >= groupInfo.pcGroup->numverts)
				{
					DefaultLogger::get()->warn("Invalid vertex index in frame vertex section");
					continue;
				}

				aiVector3D vPosition,vNormal;
					
				vPosition.x = _AI_MDL7_ACCESS_VERT(pcFrameVertices,qq,pcHeader->framevertex_stc_size) .x;
        AI_SWAP4(vPosition.x);    
				vPosition.y = -1.0f*_AI_MDL7_ACCESS_VERT(pcFrameVertices,qq,pcHeader->framevertex_stc_size) .y;
		    AI_SWAP4(vPosition.y);		
        vPosition.z = _AI_MDL7_ACCESS_VERT(pcFrameVertices,qq,pcHeader->framevertex_stc_size) .z;
        AI_SWAP4(vPosition.z);

				// now read the normal vector
				if (AI_MDL7_FRAMEVERTEX030305_STCSIZE <= pcHeader->mainvertex_stc_size)
				{
					// read the full normal vector
					vNormal.x = _AI_MDL7_ACCESS_VERT(pcFrameVertices,qq,pcHeader->framevertex_stc_size) .norm[0];
          AI_SWAP4(vNormal.x);     
					vNormal.y = -1.0f* _AI_MDL7_ACCESS_VERT(pcFrameVertices,qq,pcHeader->framevertex_stc_size) .norm[1];
			    AI_SWAP4(vNormal.y);		
          vNormal.z = _AI_MDL7_ACCESS_VERT(pcFrameVertices,qq,pcHeader->framevertex_stc_size) .norm[2];
          AI_SWAP4(vNormal.z);
				}
				else if (AI_MDL7_FRAMEVERTEX120503_STCSIZE <= pcHeader->mainvertex_stc_size)
				{
					// read the normal vector from Quake2's smart table
					MD2::LookupNormalIndex(_AI_MDL7_ACCESS_VERT(pcFrameVertices,qq,
						pcHeader->framevertex_stc_size) .norm162index,vNormal);
					vNormal.y *= -1.0f;
				}

				// FIXME: O(n^2) at the moment ...
				BE_NCONST MDL::Triangle_MDL7* pcGroupTris = groupInfo.pcGroupTris;
				unsigned int iOutIndex = 0;
				for (unsigned int iTriangle = 0; iTriangle < (unsigned int)groupInfo.pcGroup->numtris; ++iTriangle)
				{
					// iterate through all indices of the current triangle
					for (unsigned int c = 0; c < 3;++c,++iOutIndex)
					{
						// replace the vertex with the new data
						const unsigned int iCurIndex = pcGroupTris->v_index[c];
						if (iCurIndex == iIndex)
						{
							groupData.vPositions[iOutIndex] = vPosition;
							groupData.vNormals[iOutIndex] = vNormal;
						}
					}
					// get the next triangle in the list
					pcGroupTris = (BE_NCONST MDL::Triangle_MDL7*)((const char*)
						pcGroupTris + pcHeader->triangle_stc_size);
				}
			}
		}
		// parse bone trafo matrix keys (only if there are bones ...)
		if (shared.apcOutBones)this->ParseBoneTrafoKeys_3DGS_MDL7(groupInfo,frame,shared);
		szCurrent += iAdd;
	}
	*szCurrentOut = szCurrent;
	return true;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::SortByMaterials_3DGS_MDL7(
	const MDL::IntGroupInfo_MDL7& groupInfo,
	MDL::IntGroupData_MDL7& groupData,
	MDL::IntSplittedGroupData_MDL7& splittedGroupData)
{
	const unsigned int iNumMaterials = (unsigned int)splittedGroupData.shared.pcMats.size();

	// if we don't need a second set of texture coordinates there is no reason to keep it in memory ...
	if (!groupData.bNeed2UV)
	{
		groupData.vTextureCoords2.clear();

		// allocate the array
		splittedGroupData.aiSplit = new std::vector<unsigned int>*[iNumMaterials];

		for (unsigned int m = 0; m < iNumMaterials;++m)
			splittedGroupData.aiSplit[m] = new std::vector<unsigned int>();

		// iterate through all faces and sort by material
		for (unsigned int iFace = 0; iFace < (unsigned int)groupInfo.pcGroup->numtris;++iFace)
		{
			// check range
			if (groupData.pcFaces[iFace].iMatIndex[0] >= iNumMaterials)
			{
				// use the last material instead
				splittedGroupData.aiSplit[iNumMaterials-1]->push_back(iFace);

				// sometimes MED writes -1, but normally only if there is only
				// one skin assigned. No warning in this case
				if(0xFFFFFFFF != groupData.pcFaces[iFace].iMatIndex[0])
					DefaultLogger::get()->warn("Index overflow in MDL7 material list [#0]");
			}
			else splittedGroupData.aiSplit[groupData.pcFaces[iFace].
				iMatIndex[0]]->push_back(iFace);
		}
	}
	else
	{
		// we need to build combined materials for each combination of
		std::vector<MDL::IntMaterial_MDL7> avMats;
		avMats.reserve(iNumMaterials*2);

		std::vector<std::vector<unsigned int>* > aiTempSplit;
		aiTempSplit.reserve(iNumMaterials*2);

		for (unsigned int m = 0; m < iNumMaterials;++m)
			aiTempSplit[m] = new std::vector<unsigned int>();

		// iterate through all faces and sort by material
		for (unsigned int iFace = 0; iFace < (unsigned int)groupInfo.pcGroup->numtris;++iFace)
		{
			// check range
			unsigned int iMatIndex = groupData.pcFaces[iFace].iMatIndex[0];
			if (iMatIndex >= iNumMaterials)
			{
				// sometimes MED writes -1, but normally only if there is only
				// one skin assigned. No warning in this case
				if(0xffffffff != iMatIndex)
					DefaultLogger::get()->warn("Index overflow in MDL7 material list [#1]");
				iMatIndex = iNumMaterials-1;
			}
			unsigned int iMatIndex2 = groupData.pcFaces[iFace].iMatIndex[1];

			unsigned int iNum = iMatIndex;
			if (0xffffffff != iMatIndex2 && iMatIndex != iMatIndex2)
			{
				if (iMatIndex2 >= iNumMaterials)
				{
					// sometimes MED writes -1, but normally only if there is only
					// one skin assigned. No warning in this case
					DefaultLogger::get()->warn("Index overflow in MDL7 material list [#2]");
					iMatIndex2 = iNumMaterials-1;
				}

				// do a slow seach in the list ...
				iNum = 0;
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
					this->JoinSkins_3DGS_MDL7(splittedGroupData.shared.pcMats[iMatIndex],
						splittedGroupData.shared.pcMats[iMatIndex2],sHelper.pcMat);

					// and add it to the list
					avMats.push_back(sHelper);
					iNum = (unsigned int)avMats.size()-1;
				}
				// adjust the size of the file array
				if (iNum == aiTempSplit.size())
				{
					aiTempSplit.push_back(new std::vector<unsigned int>());
				}
			}
			aiTempSplit[iNum]->push_back(iFace);
		}

		// now add the newly created materials to the old list
		if (0 == groupInfo.iIndex)
		{
			splittedGroupData.shared.pcMats.resize(avMats.size());
			for (unsigned int o = 0; o < avMats.size();++o)
				splittedGroupData.shared.pcMats[o] = avMats[o].pcMat;
		}
		else
		{
			// TODO: This might result in redundant materials ...
			splittedGroupData.shared.pcMats.resize(iNumMaterials + avMats.size());
			for (unsigned int o = iNumMaterials; o < avMats.size();++o)
				splittedGroupData.shared.pcMats[o] = avMats[o].pcMat;
		}

		// and build the final face-to-material array
		splittedGroupData.aiSplit = new std::vector<unsigned int>*[aiTempSplit.size()];
		for (unsigned int m = 0; m < iNumMaterials;++m)
			splittedGroupData.aiSplit[m] = aiTempSplit[m];
	}
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::InternReadFile_3DGS_MDL7( )
{
	ai_assert(NULL != pScene);

	MDL::IntSharedData_MDL7 sharedData;

	// current cursor position in the file
	BE_NCONST MDL::Header_MDL7 *pcHeader = (BE_NCONST MDL::Header_MDL7*)this->mBuffer; 
	const unsigned char* szCurrent = (const unsigned char*)(pcHeader+1);
  
  AI_SWAP4(pcHeader->version);
  AI_SWAP4(pcHeader->bones_num);
  AI_SWAP4(pcHeader->groups_num);
  AI_SWAP4(pcHeader->data_size);
  AI_SWAP4(pcHeader->entlump_size);
  AI_SWAP4(pcHeader->medlump_size);
  AI_SWAP2(pcHeader->bone_stc_size);
  AI_SWAP2(pcHeader->skin_stc_size);
  AI_SWAP2(pcHeader->colorvalue_stc_size);
  AI_SWAP2(pcHeader->material_stc_size);
  AI_SWAP2(pcHeader->skinpoint_stc_size);
  AI_SWAP2(pcHeader->triangle_stc_size);
  AI_SWAP2(pcHeader->mainvertex_stc_size);
  AI_SWAP2(pcHeader->framevertex_stc_size);
  AI_SWAP2(pcHeader->bonetrans_stc_size);
  AI_SWAP2(pcHeader->frame_stc_size);

	// validate the header of the file. There are some structure
	// sizes that are expected by the loader to be constant 
	this->ValidateHeader_3DGS_MDL7(pcHeader);

	// load all bones (they are shared by all groups, so
	// we'll need to add them to all groups/meshes later)
	// apcBonesOut is a list of all bones or NULL if they could not been loaded 
	szCurrent += pcHeader->bones_num * pcHeader->bone_stc_size;
	sharedData.apcOutBones = this->LoadBones_3DGS_MDL7();

	// vector to held all created meshes
	std::vector<aiMesh*>* avOutList;

	// 3 meshes per group - that should be OK for most models
	avOutList = new std::vector<aiMesh*>[pcHeader->groups_num];
	for (uint32_t i = 0; i < pcHeader->groups_num;++i)
		avOutList[i].reserve(3);

	// buffer to held the names of all groups in the file
	char* aszGroupNameBuffer = new char[AI_MDL7_MAX_GROUPNAMESIZE*pcHeader->groups_num];

	// read all groups
	for (unsigned int iGroup = 0; iGroup < (unsigned int)pcHeader->groups_num;++iGroup)
	{
		MDL::IntGroupInfo_MDL7 groupInfo((BE_NCONST MDL::Group_MDL7*)szCurrent,iGroup);
		szCurrent = (const unsigned char*)(groupInfo.pcGroup+1);

		VALIDATE_FILE_SIZE(szCurrent);
    
    AI_SWAP4(groupInfo.pcGroup->groupdata_size);
    AI_SWAP4(groupInfo.pcGroup->numskins);
    AI_SWAP4(groupInfo.pcGroup->num_stpts);
    AI_SWAP4(groupInfo.pcGroup->numtris);
    AI_SWAP4(groupInfo.pcGroup->numverts);
    AI_SWAP4(groupInfo.pcGroup->numframes);

		if (1 != groupInfo.pcGroup->typ)
		{
			// Not a triangle-based mesh
			DefaultLogger::get()->warn("[3DGS MDL7] Mesh group is not basing on"
				"triangles. Continuing happily");
		}

		// store the name of the group
		const unsigned int ofs = iGroup*AI_MDL7_MAX_GROUPNAMESIZE;
		::memcpy(&aszGroupNameBuffer[ofs],
			groupInfo.pcGroup->name,AI_MDL7_MAX_GROUPNAMESIZE);

		// make sure '\0' is at the end
		aszGroupNameBuffer[ofs+AI_MDL7_MAX_GROUPNAMESIZE-1] = '\0';

		// read all skins
		sharedData.pcMats.reserve(sharedData.pcMats.size() + groupInfo.pcGroup->numskins);
		sharedData.abNeedMaterials.resize(sharedData.abNeedMaterials.size() +
			groupInfo.pcGroup->numskins,false);

		for (unsigned int iSkin = 0; iSkin < (unsigned int)groupInfo.pcGroup->numskins;++iSkin)
		{
			this->ParseSkinLump_3DGS_MDL7(szCurrent,&szCurrent,sharedData.pcMats);
		}
		// if we have absolutely no skin loaded we need to generate a default material
		if (sharedData.pcMats.empty())
		{
			const int iMode = (int)aiShadingMode_Gouraud;
			sharedData.pcMats.push_back(new MaterialHelper());
			MaterialHelper* pcHelper = (MaterialHelper*)sharedData.pcMats[0];
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

			sharedData.abNeedMaterials.resize(1,false);
		}

		// now get a pointer to all texture coords in the group
		groupInfo.pcGroupUVs = (BE_NCONST MDL::TexCoord_MDL7*)szCurrent;      
    for(int i = 0; i < groupInfo.pcGroup->num_stpts; ++i){  
      AI_SWAP4(groupInfo.pcGroupUVs[i].u);
      AI_SWAP4(groupInfo.pcGroupUVs[i].v);
    }
    szCurrent += pcHeader->skinpoint_stc_size * groupInfo.pcGroup->num_stpts;

		// now get a pointer to all triangle in the group
		groupInfo.pcGroupTris = (BE_NCONST MDL::Triangle_MDL7*)szCurrent;
	  szCurrent += pcHeader->triangle_stc_size * groupInfo.pcGroup->numtris;

		// now get a pointer to all vertices in the group
		groupInfo.pcGroupVerts = (BE_NCONST MDL::Vertex_MDL7*)szCurrent;
    for(int i = 0; i < groupInfo.pcGroup->numverts; ++i){  
      AI_SWAP4(groupInfo.pcGroupVerts[i].x);
      AI_SWAP4(groupInfo.pcGroupVerts[i].y);
      AI_SWAP4(groupInfo.pcGroupVerts[i].z);
      
      AI_SWAP2(groupInfo.pcGroupVerts[i].vertindex);
      //We can not swap the normal information now as we don't know which of the two kinds it is
    }
		szCurrent += pcHeader->mainvertex_stc_size * groupInfo.pcGroup->numverts;

		VALIDATE_FILE_SIZE(szCurrent);
    
    MDL::IntSplittedGroupData_MDL7 splittedGroupData(sharedData,avOutList[iGroup]);
		MDL::IntGroupData_MDL7 groupData;
		if (groupInfo.pcGroup->numtris && groupInfo.pcGroup->numverts)
		{
			// build output vectors
			const unsigned int iNumVertices = groupInfo.pcGroup->numtris*3;
			groupData.vPositions.resize(iNumVertices);
			groupData.vNormals.resize(iNumVertices);

			if (sharedData.apcOutBones)groupData.aiBones.resize(iNumVertices,0xffffffff);

			// it is also possible that there are 0 UV coordinate sets
			if (groupInfo.pcGroup->num_stpts)
			{
				groupData.vTextureCoords1.resize(iNumVertices,aiVector3D());

				// check whether the triangle data structure is large enough
				// to contain a second UV coodinate set
				if (pcHeader->triangle_stc_size >= AI_MDL7_TRIANGLE_STD_SIZE_TWO_UV)
				{
					groupData.vTextureCoords2.resize(iNumVertices,aiVector3D());
					groupData.bNeed2UV = true;
				}
			}
			groupData.pcFaces = new MDL::IntFace_MDL7[groupInfo.pcGroup->numtris];

			// read all faces into the preallocated arrays
			this->ReadFaces_3DGS_MDL7(groupInfo, groupData);

			// sort by materials
			this->SortByMaterials_3DGS_MDL7(groupInfo, groupData,
				splittedGroupData);

			for (unsigned int qq = 0; qq < sharedData.pcMats.size();++qq)
			{
				if (!splittedGroupData.aiSplit[qq]->empty())
					sharedData.abNeedMaterials[qq] = true;
			}
		}
		else DefaultLogger::get()->warn("[3DGS MDL7] Mesh group consists of 0 "
			"vertices or faces. It will be skipped.");

		// process all frames and generate output meshes
		this->ProcessFrames_3DGS_MDL7(groupInfo,groupData, sharedData,szCurrent,&szCurrent);
		this->GenerateOutputMeshes_3DGS_MDL7(groupData,splittedGroupData);
	}

	// generate a nodegraph and subnodes for each group
	this->pScene->mRootNode = new aiNode();

	// now we need to build a final mesh list
	for (uint32_t i = 0; i < pcHeader->groups_num;++i)
	{
		this->pScene->mNumMeshes += (unsigned int)avOutList[i].size();
	}
	this->pScene->mMeshes = new aiMesh*[this->pScene->mNumMeshes];
	{
		unsigned int p = 0,q = 0;
		for (uint32_t i = 0; i < pcHeader->groups_num;++i)
		{
			for (unsigned int a = 0; a < avOutList[i].size();++a)
			{
				this->pScene->mMeshes[p++] = avOutList[i][a];
			}
			if (!avOutList[i].empty())++this->pScene->mRootNode->mNumChildren;
		}
		// we will later need an extra node to serve as parent for all bones
		if (sharedData.apcOutBones)++this->pScene->mRootNode->mNumChildren;
		this->pScene->mRootNode->mChildren = new aiNode*[this->pScene->mRootNode->mNumChildren];
		p = 0;
		for (uint32_t i = 0; i < pcHeader->groups_num;++i)
		{
			if (avOutList[i].empty())continue;
			
			aiNode* const pcNode = this->pScene->mRootNode->mChildren[p] = new aiNode();
			pcNode->mNumMeshes = (unsigned int)avOutList[i].size();
			pcNode->mMeshes = new unsigned int[pcNode->mNumMeshes];
			pcNode->mParent = this->pScene->mRootNode;
			for (unsigned int a = 0; a < pcNode->mNumMeshes;++a)
				pcNode->mMeshes[a] = q + a;
			q += (unsigned int)avOutList[i].size();

			// setup the name of the node
			char* const szBuffer = &aszGroupNameBuffer[i*AI_MDL7_MAX_GROUPNAMESIZE];
			if ('\0' == *szBuffer)pcNode->mName.length = ::sprintf(szBuffer,"Group_%i",p);
			else pcNode->mName.length = ::strlen(szBuffer);
			::strcpy(pcNode->mName.data,szBuffer);
			++p;
		}
	}

	// if there is only one root node with a single child we can optimize it a bit ...
	if (1 == this->pScene->mRootNode->mNumChildren && !sharedData.apcOutBones)
	{
		aiNode* pcOldRoot = this->pScene->mRootNode;
		this->pScene->mRootNode = pcOldRoot->mChildren[0];
		pcOldRoot->mChildren[0] = NULL;
		delete pcOldRoot;
		this->pScene->mRootNode->mParent = NULL;
	}
	else this->pScene->mRootNode->mName.Set("<mesh_root>");

	delete[] avOutList;
	delete[] aszGroupNameBuffer; 
	AI_DEBUG_INVALIDATE_PTR(avOutList);
	AI_DEBUG_INVALIDATE_PTR(aszGroupNameBuffer);

	// build a final material list. 
	this->CopyMaterials_3DGS_MDL7(sharedData);
	this->HandleMaterialReferences_3DGS_MDL7();

	// generate output bone animations and add all bones to the scenegraph
	if (sharedData.apcOutBones)
	{
		// this step adds empty dummy bones to the nodegraph
		// insert another dummy node to avoid name conflicts
		aiNode* const pc = this->pScene->mRootNode->mChildren[
			this->pScene->mRootNode->mNumChildren-1] = new aiNode();

		pc->mName.Set("<skeleton_root>");

		// add bones to the nodegraph
		this->AddBonesToNodeGraph_3DGS_MDL7((const Assimp::MDL::IntBone_MDL7 **)
			sharedData.apcOutBones,pc,0xffff);

		// this steps build a valid output animation
		this->BuildOutputAnims_3DGS_MDL7((const Assimp::MDL::IntBone_MDL7 **)
			sharedData.apcOutBones);
	}
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::CopyMaterials_3DGS_MDL7(MDL::IntSharedData_MDL7 &shared)
{
	this->pScene->mNumMaterials = (unsigned int)shared.pcMats.size();
	this->pScene->mMaterials = new aiMaterial*[this->pScene->mNumMaterials];
	for (unsigned int i = 0; i < this->pScene->mNumMaterials;++i)
		this->pScene->mMaterials[i] = shared.pcMats[i];
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::HandleMaterialReferences_3DGS_MDL7()
{
	// search for referrer materials
	for (unsigned int i = 0; i < this->pScene->mNumMaterials;++i)
	{
		int iIndex = 0;
		if (AI_SUCCESS == aiGetMaterialInteger(this->pScene->mMaterials[i],
			AI_MDL7_REFERRER_MATERIAL, &iIndex) )
		{
			for (unsigned int a = 0; a < this->pScene->mNumMeshes;++a)
			{
				aiMesh* const pcMesh = this->pScene->mMeshes[a];
				if (i == pcMesh->mMaterialIndex)
					pcMesh->mMaterialIndex = iIndex;
			}
			// collapse the rest of the array
			delete this->pScene->mMaterials[i];
			for (unsigned int pp = i; pp < this->pScene->mNumMaterials-1;++pp)
			{
				this->pScene->mMaterials[pp] = this->pScene->mMaterials[pp+1];
				for (unsigned int a = 0; a < this->pScene->mNumMeshes;++a)
				{
					aiMesh* const pcMesh = this->pScene->mMeshes[a];
					if (pcMesh->mMaterialIndex > i)--pcMesh->mMaterialIndex;
				}
			}
			--this->pScene->mNumMaterials;
		}
	}
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::ParseBoneTrafoKeys_3DGS_MDL7(
	const MDL::IntGroupInfo_MDL7& groupInfo,
	IntFrameInfo_MDL7& frame,
	MDL::IntSharedData_MDL7& shared)
{

	// get a pointer to the header ...
	const MDL::Header_MDL7* const pcHeader = (const MDL::Header_MDL7*)this->mBuffer;

	// only the first group contains bone animation keys
	if (frame.pcFrame->transmatrix_count)
	{
		if (!groupInfo.iIndex)
		{
			// skip all frames vertices. We can't support them
			const MDL::BoneTransform_MDL7* pcBoneTransforms = (const MDL::BoneTransform_MDL7*)
				(((const char*)frame.pcFrame) + pcHeader->frame_stc_size + 
				frame.pcFrame->vertices_count * pcHeader->framevertex_stc_size);

			// read all transformation matrices
			for (unsigned int iTrafo = 0; iTrafo < frame.pcFrame->transmatrix_count;++iTrafo)
			{
				if(pcBoneTransforms->bone_index >= pcHeader->bones_num)
				{
					DefaultLogger::get()->warn("Index overflow in frame area. "
						"Unable to parse this bone transformation");
				}
				else
				{
					this->AddAnimationBoneTrafoKey_3DGS_MDL7(frame.iIndex,
						pcBoneTransforms,shared.apcOutBones);
				}
				pcBoneTransforms = (const MDL::BoneTransform_MDL7*)(
					(const char*)pcBoneTransforms + pcHeader->bonetrans_stc_size);
			}
		}
		else
		{
			DefaultLogger::get()->warn("Found animation keyframes "
				"in a group that is not the first. They will be igored");
		}
	}
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::AddBonesToNodeGraph_3DGS_MDL7(const MDL::IntBone_MDL7** apcBones,
	aiNode* pcParent,uint16_t iParentIndex)
{
	ai_assert(NULL != apcBones && NULL != pcParent);

	// get a pointer to the header ...
	const MDL::Header_MDL7* const pcHeader = (const MDL::Header_MDL7*)this->mBuffer;

	const MDL::IntBone_MDL7** apcBones2 = apcBones;
	for (uint32_t i = 0; i <  pcHeader->bones_num;++i)
	{
		const MDL::IntBone_MDL7* const pcBone = *apcBones2++;
		if (pcBone->iParent == iParentIndex)++pcParent->mNumChildren;
	}
	pcParent->mChildren = new aiNode*[pcParent->mNumChildren];
	unsigned int qq = 0;
	for (uint32_t i = 0; i <  pcHeader->bones_num;++i)
	{
		const MDL::IntBone_MDL7* const pcBone = *apcBones++;
		if (pcBone->iParent != iParentIndex)continue;

		aiNode* pcNode = pcParent->mChildren[qq++] = new aiNode();
		pcNode->mName = aiString( pcBone->mName );

		this->AddBonesToNodeGraph_3DGS_MDL7(apcBones,pcNode,(uint16_t)i);
	}
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::BuildOutputAnims_3DGS_MDL7(
	const MDL::IntBone_MDL7** apcBonesOut)
{
	ai_assert(NULL != apcBonesOut);

	// get a pointer to the header ...
	const MDL::Header_MDL7* const pcHeader = (const MDL::Header_MDL7*)this->mBuffer;

	// one animation ...
	aiAnimation* pcAnim = new aiAnimation();
	for (uint32_t i = 0; i < pcHeader->bones_num;++i)
	{
		if (!apcBonesOut[i]->pkeyPositions.empty())
		{
			// get the last frame ... (needn't be equal to pcHeader->frames_num)
			for (size_t qq = 0; qq < apcBonesOut[i]->pkeyPositions.size();++qq)
			{
				pcAnim->mDuration = std::max(pcAnim->mDuration, (double)
					apcBonesOut[i]->pkeyPositions[qq].mTime);
			}
			++pcAnim->mNumChannels;
		}
	}
	if (pcAnim->mDuration)
	{
		pcAnim->mChannels = new aiNodeAnim*[pcAnim->mNumChannels];

		unsigned int iCnt = 0;
		for (uint32_t i = 0; i < pcHeader->bones_num;++i)
		{
			if (!apcBonesOut[i]->pkeyPositions.empty())
			{
				const MDL::IntBone_MDL7* const intBone = apcBonesOut[i];

				aiNodeAnim* const pcNodeAnim = pcAnim->mChannels[iCnt++] = new aiNodeAnim();
				pcNodeAnim->mNodeName = aiString( intBone->mName );

				// allocate enough storage for all keys
				pcNodeAnim->mNumPositionKeys = (unsigned int)intBone->pkeyPositions.size();
				pcNodeAnim->mNumScalingKeys  = (unsigned int)intBone->pkeyPositions.size();
				pcNodeAnim->mNumRotationKeys = (unsigned int)intBone->pkeyPositions.size();

				pcNodeAnim->mPositionKeys = new aiVectorKey[pcNodeAnim->mNumPositionKeys];
				pcNodeAnim->mScalingKeys = new aiVectorKey[pcNodeAnim->mNumPositionKeys];
				pcNodeAnim->mRotationKeys = new aiQuatKey[pcNodeAnim->mNumPositionKeys];

				// copy all keys
				for (unsigned int qq = 0; qq < pcNodeAnim->mNumPositionKeys;++qq)
				{
					pcNodeAnim->mPositionKeys[qq] = intBone->pkeyPositions[qq];
					pcNodeAnim->mScalingKeys[qq] = intBone->pkeyScalings[qq];
					pcNodeAnim->mRotationKeys[qq] = intBone->pkeyRotations[qq];
				}
			}
		}

		// store the output animation
		this->pScene->mNumAnimations = 1;
		this->pScene->mAnimations = new aiAnimation*[1];
		this->pScene->mAnimations[0] = pcAnim;
	}
	else delete pcAnim;
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::AddAnimationBoneTrafoKey_3DGS_MDL7(unsigned int iTrafo,
	const MDL::BoneTransform_MDL7* pcBoneTransforms,
	MDL::IntBone_MDL7** apcBonesOut)
{
	ai_assert(NULL != pcBoneTransforms);
	ai_assert(NULL != apcBonesOut);

	// first .. get the transformation matrix
	aiMatrix4x4 mTransform;
	mTransform.a1 = pcBoneTransforms->m[0];
	mTransform.b1 = pcBoneTransforms->m[1];
	mTransform.c1 = pcBoneTransforms->m[2];
	mTransform.d1 = pcBoneTransforms->m[3];

	mTransform.a2 = pcBoneTransforms->m[4];
	mTransform.b2 = pcBoneTransforms->m[5];
	mTransform.c2 = pcBoneTransforms->m[6];
	mTransform.d2 = pcBoneTransforms->m[7];

	mTransform.a3 = pcBoneTransforms->m[8];
	mTransform.b3 = pcBoneTransforms->m[9];
	mTransform.c3 = pcBoneTransforms->m[10];
	mTransform.d3 = pcBoneTransforms->m[11];

	// now decompose the transformation matrix into separate
	// scaling, rotation and translation
	aiVectorKey vScaling,vPosition;
	aiQuatKey qRotation;

	// FIXME: Decompose will assert in debug builds if the
	// matrix is invalid ...
	mTransform.Decompose(vScaling.mValue,qRotation.mValue,vPosition.mValue);

	// now generate keys
	vScaling.mTime = qRotation.mTime = vPosition.mTime = (double)iTrafo;

	// add the keys to the bone
	MDL::IntBone_MDL7* const pcBoneOut = apcBonesOut[pcBoneTransforms->bone_index];
	pcBoneOut->pkeyPositions.push_back	( vPosition );
	pcBoneOut->pkeyScalings.push_back	( vScaling  );
	pcBoneOut->pkeyRotations.push_back	( qRotation );
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::GenerateOutputMeshes_3DGS_MDL7(
	MDL::IntGroupData_MDL7& groupData,
	MDL::IntSplittedGroupData_MDL7& splittedGroupData)
{
	const MDL::IntSharedData_MDL7& shared = splittedGroupData.shared;

	// get a pointer to the header ...
	const MDL::Header_MDL7* const pcHeader = (const MDL::Header_MDL7*)this->mBuffer;
	const unsigned int iNumOutBones = pcHeader->bones_num;

	for (std::vector<MaterialHelper*>::size_type i = 0; i < shared.pcMats.size();++i)
	{
		if (!splittedGroupData.aiSplit[i]->empty())
		{
			// allocate the output mesh
			aiMesh* pcMesh = new aiMesh();

			pcMesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
			pcMesh->mMaterialIndex = (unsigned int)i;

			// allocate output storage
			pcMesh->mNumFaces = (unsigned int)splittedGroupData.aiSplit[i]->size();
			pcMesh->mFaces = new aiFace[pcMesh->mNumFaces];

			pcMesh->mNumVertices = pcMesh->mNumFaces*3;
			pcMesh->mVertices = new aiVector3D[pcMesh->mNumVertices];
			pcMesh->mNormals = new aiVector3D[pcMesh->mNumVertices];

			if (!groupData.vTextureCoords1.empty())
			{
				pcMesh->mNumUVComponents[0] = 2;
				pcMesh->mTextureCoords[0] = new aiVector3D[pcMesh->mNumVertices];
				if (!groupData.vTextureCoords2.empty())
				{
					pcMesh->mNumUVComponents[1] = 2;
					pcMesh->mTextureCoords[1] = new aiVector3D[pcMesh->mNumVertices];
				}
			}

			// iterate through all faces and build an unique set of vertices
			unsigned int iCurrent = 0;
			for (unsigned int iFace = 0; iFace < pcMesh->mNumFaces;++iFace)
			{
				pcMesh->mFaces[iFace].mNumIndices = 3;
				pcMesh->mFaces[iFace].mIndices = new unsigned int[3];

				unsigned int iSrcFace = splittedGroupData.aiSplit[i]->operator[](iFace);
				const MDL::IntFace_MDL7& oldFace = groupData.pcFaces[iSrcFace];

				// iterate through all face indices
				for (unsigned int c = 0; c < 3;++c)
				{
					const uint32_t iIndex = oldFace.mIndices[c];
					pcMesh->mVertices[iCurrent] = groupData.vPositions[iIndex];
					pcMesh->mNormals[iCurrent] = groupData.vNormals[iIndex];

					if (!groupData.vTextureCoords1.empty())
					{
						pcMesh->mTextureCoords[0][iCurrent] = groupData.vTextureCoords1[iIndex];
						if (!groupData.vTextureCoords2.empty())
						{
							pcMesh->mTextureCoords[1][iCurrent] = groupData.vTextureCoords2[iIndex];
						}
					}
					pcMesh->mFaces[iFace].mIndices[c] = iCurrent++;
				}
			}

			// if we have bones in the mesh we'll need to generate
			// proper vertex weights for them
			if (!groupData.aiBones.empty())
			{
				std::vector<std::vector<unsigned int> > aaiVWeightList;
				aaiVWeightList.resize(iNumOutBones);

				int iCurrent = 0;
				for (unsigned int iFace = 0; iFace < pcMesh->mNumFaces;++iFace)
				{
					unsigned int iSrcFace = splittedGroupData.aiSplit[i]->operator[](iFace);
					const MDL::IntFace_MDL7& oldFace = groupData.pcFaces[iSrcFace];

					// iterate through all face indices
					for (unsigned int c = 0; c < 3;++c)
					{
						unsigned int iBone = groupData.aiBones[ oldFace.mIndices[c] ];
						if (0xffffffff != iBone)
						{
							if (iBone >= iNumOutBones)
							{
								DefaultLogger::get()->error("Bone index overflow. "
									"The bone index of a vertex exceeds the allowed range. ");
								iBone = iNumOutBones-1;
							}
							aaiVWeightList[ iBone ].push_back ( iCurrent );
						}
						++iCurrent;
					}
				}
				// now check which bones are required ...
				for (std::vector<std::vector<unsigned int> >::const_iterator
					kimmi =  aaiVWeightList.begin();
					kimmi != aaiVWeightList.end();++kimmi)
				{
					if (!(*kimmi).empty())++pcMesh->mNumBones;
				}
				pcMesh->mBones = new aiBone*[pcMesh->mNumBones];
				iCurrent = 0;
				for (std::vector<std::vector<unsigned int> >::const_iterator
					kimmi =  aaiVWeightList.begin();
					kimmi != aaiVWeightList.end();++kimmi,++iCurrent)
				{
					if ((*kimmi).empty())continue;

					// seems we'll need this node
					aiBone* pcBone = pcMesh->mBones[ iCurrent ] = new aiBone();
					pcBone->mName = aiString(shared.apcOutBones[ iCurrent ]->mName);
					pcBone->mOffsetMatrix = shared.apcOutBones[ iCurrent ]->mOffsetMatrix;

					// setup vertex weights
					pcBone->mNumWeights = (unsigned int)(*kimmi).size();
					pcBone->mWeights = new aiVertexWeight[pcBone->mNumWeights];

					for (unsigned int weight = 0; weight < pcBone->mNumWeights;++weight)
					{
						pcBone->mWeights[weight].mVertexId = (*kimmi)[weight]; 
						pcBone->mWeights[weight].mWeight = 1.0f;
					}
				}
			}
			// add the mesh to the list of output meshes
			splittedGroupData.avOutList.push_back(pcMesh);
		}
	}
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::JoinSkins_3DGS_MDL7(
	MaterialHelper* pcMat1,
	MaterialHelper* pcMat2,
	MaterialHelper* pcMatOut)
{
	ai_assert(NULL != pcMat1 && NULL != pcMat2 && NULL != pcMatOut);

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
}
// ------------------------------------------------------------------------------------------------
void MDLImporter::InternReadFile_HL2( )
{
	//const MDL::Header_HL2* pcHeader = (const MDL::Header_HL2*)this->mBuffer;
}
