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

/** @file Implementation of the MD3 importer class */

#include "AssimpPCH.h"


#include "MD3Loader.h"
#include "MaterialSystem.h"
#include "StringComparison.h"
#include "ByteSwap.h"

using namespace Assimp;


// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
MD3Importer::MD3Importer()
{
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
MD3Importer::~MD3Importer()
{
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool MD3Importer::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
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
	if (extension[3] != '3')return false;

	return true;
}
// ------------------------------------------------------------------------------------------------
void MD3Importer::ValidateHeaderOffsets()
{
	// check magic number
	if (pcHeader->IDENT != AI_MD3_MAGIC_NUMBER_BE &&
		pcHeader->IDENT != AI_MD3_MAGIC_NUMBER_LE)
			throw new ImportErrorException( "Invalid MD3 file: Magic bytes not found");

	// check file format version
	if (pcHeader->VERSION > 15)
		DefaultLogger::get()->warn( "Unsupported MD3 file version. Continuing happily ...");

	// check some values whether they are valid
	if (!pcHeader->NUM_SURFACES)
		throw new ImportErrorException( "Invalid md3 file: NUM_SURFACES is 0");

	if (pcHeader->OFS_FRAMES	>= fileSize ||
		pcHeader->OFS_SURFACES	>= fileSize || 
		pcHeader->OFS_EOF		> fileSize)
	{
		throw new ImportErrorException("Invalid MD3 header: some offsets are outside the file");
	}

	if (pcHeader->NUM_FRAMES <= this->configFrameID )
		throw new ImportErrorException("The requested frame is not existing the file");
}
// ------------------------------------------------------------------------------------------------
void MD3Importer::ValidateSurfaceHeaderOffsets(const MD3::Surface* pcSurf)
{
	// calculate the relative offset of the surface
	int32_t ofs = int32_t((const unsigned char*)pcSurf-this->mBuffer);

	if (pcSurf->OFS_TRIANGLES	+ ofs + pcSurf->NUM_TRIANGLES * sizeof(MD3::Triangle)	> fileSize ||
		pcSurf->OFS_SHADERS		+ ofs + pcSurf->NUM_SHADER * sizeof(MD3::Shader)		> fileSize ||
		pcSurf->OFS_ST			+ ofs + pcSurf->NUM_VERTICES * sizeof(MD3::TexCoord)	> fileSize ||
		pcSurf->OFS_XYZNORMAL	+ ofs + pcSurf->NUM_VERTICES * sizeof(MD3::Vertex)		> fileSize)
	{
		throw new ImportErrorException("Invalid MD3 surface header: some offsets are outside the file");
	}

	if (pcSurf->NUM_TRIANGLES > AI_MD3_MAX_TRIANGLES)
		DefaultLogger::get()->warn("The model contains more triangles than Quake 3 supports");
	if (pcSurf->NUM_SHADER > AI_MD3_MAX_SHADERS)
		DefaultLogger::get()->warn("The model contains more shaders than Quake 3 supports");
	if (pcSurf->NUM_VERTICES > AI_MD3_MAX_VERTS)
		DefaultLogger::get()->warn("The model contains more vertices than Quake 3 supports");
	if (pcSurf->NUM_FRAMES > AI_MD3_MAX_FRAMES)
		DefaultLogger::get()->warn("The model contains more frames than Quake 3 supports");
}
// ------------------------------------------------------------------------------------------------
// Setup configuration properties
void MD3Importer::SetupProperties(const Importer* pImp)
{
	// The AI_CONFIG_IMPORT_MD3_KEYFRAME option overrides the
	// AI_CONFIG_IMPORT_GLOBAL_KEYFRAME option.
	if(0xffffffff == (this->configFrameID = pImp->GetPropertyInteger(
		AI_CONFIG_IMPORT_MD3_KEYFRAME,0xffffffff)))
	{
		this->configFrameID = pImp->GetPropertyInteger(AI_CONFIG_IMPORT_GLOBAL_KEYFRAME,0);
	}
}
// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void MD3Importer::InternReadFile( 
	const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile));

	// Check whether we can read from the file
	if( file.get() == NULL)
		throw new ImportErrorException( "Failed to open MD3 file " + pFile + ".");

	// check whether the md3 file is large enough to contain
	// at least the file header
	fileSize = (unsigned int)file->FileSize();
	if( fileSize < sizeof(MD3::Header))
		throw new ImportErrorException( "MD3 File is too small.");

	// allocate storage and copy the contents of the file to a memory buffer
	std::vector<unsigned char> mBuffer2 (fileSize);
	file->Read( &mBuffer2[0], 1, fileSize);
	mBuffer = &mBuffer2[0];

	pcHeader = (BE_NCONST MD3::Header*)mBuffer;

#ifdef AI_BUILD_BIG_ENDIAN

	AI_SWAP4(pcHeader->VERSION);
	AI_SWAP4(pcHeader->FLAGS);
	AI_SWAP4(pcHeader->IDENT);
	AI_SWAP4(pcHeader->NUM_FRAMES);
	AI_SWAP4(pcHeader->NUM_SKINS);
	AI_SWAP4(pcHeader->NUM_SURFACES);
	AI_SWAP4(pcHeader->NUM_TAGS);
	AI_SWAP4(pcHeader->OFS_EOF);
	AI_SWAP4(pcHeader->OFS_FRAMES);
	AI_SWAP4(pcHeader->OFS_SURFACES);
	AI_SWAP4(pcHeader->OFS_TAGS);

#endif

	// validate the header
	this->ValidateHeaderOffsets();

	// now navigate to the list of surfaces
	BE_NCONST MD3::Surface* pcSurfaces = (BE_NCONST MD3::Surface*)(mBuffer + pcHeader->OFS_SURFACES);

	// allocate output storage
	pScene->mNumMeshes = pcHeader->NUM_SURFACES;
	pScene->mMeshes = new aiMesh*[pScene->mNumMeshes];

	pScene->mNumMaterials = pcHeader->NUM_SURFACES;
	pScene->mMaterials = new aiMaterial*[pScene->mNumMeshes];

	// if an exception is thrown before the meshes are allocated ->
	// otherwise the pointer value would be invalid and delete would crash
	::memset(pScene->mMeshes,0,pScene->mNumMeshes*sizeof(aiMesh*));
	::memset(pScene->mMaterials,0,pScene->mNumMaterials*sizeof(aiMaterial*));

	unsigned int iNum = pcHeader->NUM_SURFACES;
	unsigned int iNumMaterials = 0;
	unsigned int iDefaultMatIndex = 0xFFFFFFFF;
	while (iNum-- > 0)
	{

#ifdef AI_BUILD_BIG_ENDIAN

		AI_SWAP4(pcSurfaces->FLAGS);
		AI_SWAP4(pcSurfaces->IDENT);
		AI_SWAP4(pcSurfaces->NUM_FRAMES);
		AI_SWAP4(pcSurfaces->NUM_SHADER);
		AI_SWAP4(pcSurfaces->NUM_TRIANGLES);
		AI_SWAP4(pcSurfaces->NUM_VERTICES);
		AI_SWAP4(pcSurfaces->OFS_END);
		AI_SWAP4(pcSurfaces->OFS_SHADERS);
		AI_SWAP4(pcSurfaces->OFS_ST);
		AI_SWAP4(pcSurfaces->OFS_TRIANGLES);
		AI_SWAP4(pcSurfaces->OFS_XYZNORMAL);

#endif

		// validate the surface
		this->ValidateSurfaceHeaderOffsets(pcSurfaces);

		// navigate to the vertex list of the surface
		BE_NCONST MD3::Vertex* pcVertices = (BE_NCONST MD3::Vertex*)
			(((uint8_t*)pcSurfaces) + pcSurfaces->OFS_XYZNORMAL);

		// navigate to the triangle list of the surface
		BE_NCONST MD3::Triangle* pcTriangles = (BE_NCONST MD3::Triangle*)
			(((uint8_t*)pcSurfaces) + pcSurfaces->OFS_TRIANGLES);

		// navigate to the texture coordinate list of the surface
		BE_NCONST MD3::TexCoord* pcUVs = (BE_NCONST MD3::TexCoord*)
			(((uint8_t*)pcSurfaces) + pcSurfaces->OFS_ST);

		// navigate to the shader list of the surface
		BE_NCONST MD3::Shader* pcShaders = (BE_NCONST MD3::Shader*)
			(((uint8_t*)pcSurfaces) + pcSurfaces->OFS_SHADERS);

		// if the submesh is empty ignore it
		if (0 == pcSurfaces->NUM_VERTICES || 0 == pcSurfaces->NUM_TRIANGLES)
		{
			pcSurfaces = (BE_NCONST MD3::Surface*)(((uint8_t*)pcSurfaces) + pcSurfaces->OFS_END);
			pScene->mNumMeshes--;
			continue;
		}

#ifdef AI_BUILD_BIG_ENDIAN

		for (uint32_t i = 0; i < pcSurfaces->NUM_VERTICES;++i)
		{
			AI_SWAP2( pcVertices[i].NORMAL );
			AI_SWAP2( pcVertices[i].X );
			AI_SWAP2( pcVertices[i].Y );
			AI_SWAP2( pcVertices[i].Z );

			AI_SWAP4( pcUVs[i].U );
			AI_SWAP4( pcUVs[i].U );
		}
		for (uint32_t i = 0; i < pcSurfaces->NUM_TRIANGLES;++i)
		{
			AI_SWAP4(pcTriangles[i].INDEXES[0]);
			AI_SWAP4(pcTriangles[i].INDEXES[1]);
			AI_SWAP4(pcTriangles[i].INDEXES[2]);
		}

#endif

		// allocate the output mesh
		pScene->mMeshes[iNum] = new aiMesh();
		aiMesh* pcMesh = pScene->mMeshes[iNum];
		pcMesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

		pcMesh->mNumVertices		= pcSurfaces->NUM_TRIANGLES*3;
		pcMesh->mNumFaces			= pcSurfaces->NUM_TRIANGLES;
		pcMesh->mFaces				= new aiFace[pcSurfaces->NUM_TRIANGLES];
		pcMesh->mNormals			= new aiVector3D[pcMesh->mNumVertices];
		pcMesh->mVertices			= new aiVector3D[pcMesh->mNumVertices];
		pcMesh->mTextureCoords[0]	= new aiVector3D[pcMesh->mNumVertices];
		pcMesh->mNumUVComponents[0] = 2;

		// fill in all triangles
		unsigned int iCurrent = 0;
		for (unsigned int i = 0; i < (unsigned int)pcSurfaces->NUM_TRIANGLES;++i)
		{
			pcMesh->mFaces[i].mIndices = new unsigned int[3];
			pcMesh->mFaces[i].mNumIndices = 3;

			unsigned int iTemp = iCurrent;
			for (unsigned int c = 0; c < 3;++c,++iCurrent)
			{
				// read vertices
				pcMesh->mVertices[iCurrent].x = pcVertices[ pcTriangles->INDEXES[c]].X*AI_MD3_XYZ_SCALE;
				pcMesh->mVertices[iCurrent].y = pcVertices[ pcTriangles->INDEXES[c]].Y*AI_MD3_XYZ_SCALE;
				pcMesh->mVertices[iCurrent].z = pcVertices[ pcTriangles->INDEXES[c]].Z*AI_MD3_XYZ_SCALE;

				// convert the normal vector to uncompressed float3 format
				LatLngNormalToVec3(pcVertices[pcTriangles->INDEXES[c]].NORMAL,
					(float*)&pcMesh->mNormals[iCurrent]);

				//pcMesh->mNormals[iCurrent].y *= -1.0f;

				// read texture coordinates
				pcMesh->mTextureCoords[0][iCurrent].x = pcUVs[ pcTriangles->INDEXES[c]].U;
				pcMesh->mTextureCoords[0][iCurrent].y = 1.0f-pcUVs[ pcTriangles->INDEXES[c]].V;
			}
			// FIX: flip the face ordering for use with OpenGL
			pcMesh->mFaces[i].mIndices[0] = iTemp+2;
			pcMesh->mFaces[i].mIndices[1] = iTemp+1;
			pcMesh->mFaces[i].mIndices[2] = iTemp+0;
			pcTriangles++;
		}

		// get the first shader (= texture?) assigned to the surface
		if (pcSurfaces->NUM_SHADER)
		{
			// make a relative path.
			// if the MD3's internal path itself and the given path are using
			// the same directory remove it
			const char* szEndDir1 = ::strrchr((const char*)pcHeader->NAME,'\\');
			if (!szEndDir1)szEndDir1 = ::strrchr((const char*)pcHeader->NAME,'/');

			const char* szEndDir2 = ::strrchr((const char*)pcShaders->NAME,'\\');
			if (!szEndDir2)szEndDir2 = ::strrchr((const char*)pcShaders->NAME,'/');

			if (szEndDir1 && szEndDir2)
			{
				// both of them are valid
				const unsigned int iLen1 = (unsigned int)(szEndDir1 - (const char*)pcHeader->NAME);
				const unsigned int iLen2 = std::min (iLen1, (unsigned int)(szEndDir2 - (const char*)pcShaders->NAME) );

				bool bSuccess = true;
				for (unsigned int a = 0; a  < iLen2;++a)
				{
					char sz = ::tolower ( pcShaders->NAME[a] );
					char sz2 = ::tolower ( pcHeader->NAME[a] );
					if (sz != sz2)
					{
						bSuccess = false;
						break;
					}
				}
				if (bSuccess)
				{
					// use the file name only
					szEndDir2++;
				}
				else
				{
					// use the full path
					szEndDir2 = (const char*)pcShaders->NAME;
				}
			}
			MaterialHelper* pcHelper = new MaterialHelper();

			if (szEndDir2)
			{
				if (szEndDir2[0])
				{
					aiString szString;
					const size_t iLen = ::strlen(szEndDir2);
					::memcpy(szString.data,szEndDir2,iLen);
					szString.data[iLen] = '\0';
					szString.length = iLen;

					pcHelper->AddProperty(&szString,AI_MATKEY_TEXTURE_DIFFUSE(0));
				}
				else 
				{
					DefaultLogger::get()->warn("Texture file name has zero length. "
						"It will be skipped.");
				}
			}

			int iMode = (int)aiShadingMode_Gouraud;
			pcHelper->AddProperty<int>(&iMode, 1, AI_MATKEY_SHADING_MODEL);

			// add a small ambient color value - Quake 3 seems to have one
			aiColor3D clr;
			clr.b = clr.g = clr.r = 0.05f;
			pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_AMBIENT);

			clr.b = clr.g = clr.r = 1.0f;
			pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_DIFFUSE);
			pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_SPECULAR);

			aiString szName;
			szName.Set(AI_DEFAULT_MATERIAL_NAME);
			pcHelper->AddProperty(&szName,AI_MATKEY_NAME);

			pScene->mMaterials[iNumMaterials] = (aiMaterial*)pcHelper;
			pcMesh->mMaterialIndex = iNumMaterials++;
		}
		else
		{
			if (0xFFFFFFFF != iDefaultMatIndex)
			{
				pcMesh->mMaterialIndex = iDefaultMatIndex;
			}
			else
			{
				MaterialHelper* pcHelper = new MaterialHelper();

				// fill in a default material
				int iMode = (int)aiShadingMode_Gouraud;
				pcHelper->AddProperty<int>(&iMode, 1, AI_MATKEY_SHADING_MODEL);

				aiColor3D clr;
				clr.b = clr.g = clr.r = 0.6f;
				pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_DIFFUSE);
				pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_SPECULAR);

				clr.b = clr.g = clr.r = 0.05f;
				pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_AMBIENT);

				pScene->mMaterials[iNumMaterials] = (aiMaterial*)pcHelper;
				iDefaultMatIndex = pcMesh->mMaterialIndex = iNumMaterials++;
			}
		}
		// go to the next surface
		pcSurfaces = (BE_NCONST MD3::Surface*)(((unsigned char*)pcSurfaces) + pcSurfaces->OFS_END);
	}

	if (!pScene->mNumMeshes)
		throw new ImportErrorException( "Invalid MD3 file: File contains no valid mesh");
	pScene->mNumMaterials = iNumMaterials;

	// now we need to generate an empty node graph
	pScene->mRootNode = new aiNode();
	pScene->mRootNode->mNumMeshes = pScene->mNumMeshes;
	pScene->mRootNode->mMeshes = new unsigned int[pScene->mNumMeshes];

	for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
		pScene->mRootNode->mMeshes[i] = i;
}
