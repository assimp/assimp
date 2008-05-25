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
#include "MD3Loader.h"
#include "MaterialSystem.h"

#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"
#include "../include/aiAssert.h"

#include <boost/scoped_ptr.hpp>

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

	// not brilliant but working ;-)
	if( extension == ".md3" || extension == ".MD3" || 
		extension == ".mD3" || extension == ".Md3")
		return true;

	return false;
}
// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void MD3Importer::InternReadFile( 
	const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile));

	// Check whether we can read from the file
	if( file.get() == NULL)
	{
		throw new ImportErrorException( "Failed to open md3 file " + pFile + ".");
	}

	// check whether the md3 file is large enough to contain
	// at least the file header
	size_t fileSize = file->FileSize();
	if( fileSize < sizeof(MD3::Header))
	{
		throw new ImportErrorException( ".md3 File is too small.");
	}

	// allocate storage and copy the contents of the file to a memory buffer
	this->mBuffer = new unsigned char[fileSize];
	file->Read( (void*)mBuffer, 1, fileSize);

	this->m_pcHeader = (const MD3::Header*)this->mBuffer;

	// check magic number
	if (this->m_pcHeader->IDENT != AI_MD3_MAGIC_NUMBER_BE &&
		this->m_pcHeader->IDENT != AI_MD3_MAGIC_NUMBER_LE)
	{
		throw new ImportErrorException( "Invalid md3 file: Magic bytes not found");
	}

	// check file format version
	if (this->m_pcHeader->VERSION > 15)
	{
		throw new ImportErrorException( "Unsupported md3 file version");
	}

	// check some values whether they are valid
	if (0 == this->m_pcHeader->NUM_FRAMES)
	{
		throw new ImportErrorException( "Invalid md3 file: NUM_FRAMES is 0");
	}
	if (0 == this->m_pcHeader->NUM_SURFACES)
	{
		throw new ImportErrorException( "Invalid md3 file: NUM_SURFACES is 0");
	}
	if (this->m_pcHeader->OFS_EOF > (int32_t)fileSize)
	{
		throw new ImportErrorException( "Invalid md3 file: File is too small");
	}

	// now navigate to the list of surfaces
	const MD3::Surface* pcSurfaces = (const MD3::Surface*)
		(this->mBuffer + this->m_pcHeader->OFS_SURFACES);

	// allocate output storage
	pScene->mNumMeshes = this->m_pcHeader->NUM_SURFACES;
	pScene->mMeshes = new aiMesh*[pScene->mNumMeshes];

	pScene->mNumMaterials = this->m_pcHeader->NUM_SURFACES;
	pScene->mMaterials = new aiMaterial*[pScene->mNumMeshes];

	unsigned int iNum = this->m_pcHeader->NUM_SURFACES;
	unsigned int iNumMaterials = 0;
	unsigned int iDefaultMatIndex = 0xFFFFFFFF;
	while (iNum-- > 0)
	{
		// navigate to the vertex list of the surface
		const MD3::Vertex* pcVertices = (const MD3::Vertex*)
			(((unsigned char*)pcSurfaces) + pcSurfaces->OFS_XYZNORMAL);

		// navigate to the triangle list of the surface
		const MD3::Triangle* pcTriangles = (const MD3::Triangle*)
			(((unsigned char*)pcSurfaces) + pcSurfaces->OFS_TRIANGLES);

		// navigate to the texture coordinate list of the surface
		const MD3::TexCoord* pcUVs = (const MD3::TexCoord*)
			(((unsigned char*)pcSurfaces) + pcSurfaces->OFS_ST);

		// navigate to the shader list of the surface
		const MD3::Shader* pcShaders = (const MD3::Shader*)
			(((unsigned char*)pcSurfaces) + pcSurfaces->OFS_SHADERS);


		// if the submesh is empty ignore it
		if (0 == pcSurfaces->NUM_VERTICES || 0 == pcSurfaces->NUM_TRIANGLES)
		{
			pcSurfaces = (const MD3::Surface*)(((unsigned char*)pcSurfaces) + pcSurfaces->OFS_END);
			pScene->mNumMeshes--;
			continue;
		}

		// allocate the output mesh
		pScene->mMeshes[iNum] = new aiMesh();
		aiMesh* pcMesh = pScene->mMeshes[iNum];

		pcMesh->mNumVertices = pcSurfaces->NUM_TRIANGLES*3;
		pcMesh->mNumBones = 0;
		pcMesh->mColors[0] = pcMesh->mColors[1] = pcMesh->mColors[2] = pcMesh->mColors[3] = NULL;
		pcMesh->mNumFaces = pcSurfaces->NUM_TRIANGLES;
		pcMesh->mFaces = new aiFace[pcSurfaces->NUM_TRIANGLES];
		pcMesh->mNormals = new aiVector3D[pcMesh->mNumVertices];
		pcMesh->mVertices = new aiVector3D[pcMesh->mNumVertices];
		pcMesh->mTextureCoords[0] = new aiVector3D[pcMesh->mNumVertices];
		pcMesh->mTextureCoords[1] = pcMesh->mTextureCoords[2] = pcMesh->mTextureCoords[3] = NULL;
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
				pcMesh->mVertices[iCurrent].x = pcVertices[ pcTriangles->INDEXES[c]].X;
				pcMesh->mVertices[iCurrent].y = pcVertices[ pcTriangles->INDEXES[c]].Y;
				pcMesh->mVertices[iCurrent].z = pcVertices[ pcTriangles->INDEXES[c]].Z*-1.0f;

				// convert the normal vector to uncompressed float3 format
				LatLngNormalToVec3(pcVertices[pcTriangles->INDEXES[c]].NORMAL,
					(float*)&pcMesh->mNormals[iCurrent]);

				//std::swap(pcMesh->mNormals[iCurrent].z,pcMesh->mNormals[iCurrent].y);
				pcMesh->mNormals[iCurrent].z *= -1.0f;

				// read texture coordinates
				pcMesh->mTextureCoords[0][iCurrent].x = pcUVs[ pcTriangles->INDEXES[c]].U;
				pcMesh->mTextureCoords[0][iCurrent].y = 1.0f - pcUVs[ pcTriangles->INDEXES[c]].V;
			}
			pcMesh->mFaces[i].mIndices[0] = iTemp+2;
			pcMesh->mFaces[i].mIndices[1] = iTemp+1;
			pcMesh->mFaces[i].mIndices[2] = iTemp+0;
			pcTriangles++;
		}

		// get the first shader (= texture?) assigned to the surface
		if (0 != pcSurfaces->NUM_SHADER)
		{
			// make a relative path.
			// if the MD3's internal path itself and the given path are using
			// the same directory remove it
			const char* szEndDir1 = strrchr((const char*)this->m_pcHeader->NAME,'\\');
			if (!szEndDir1)szEndDir1 = strrchr((const char*)this->m_pcHeader->NAME,'/');

			const char* szEndDir2 = strrchr((const char*)pcShaders->NAME,'\\');
			if (!szEndDir2)szEndDir2 = strrchr((const char*)pcShaders->NAME,'/');

			if (szEndDir1 && szEndDir2)
			{
				// both of them are valid
				const unsigned int iLen1 = (unsigned int)(szEndDir1 - (const char*)this->m_pcHeader->NAME);
				const unsigned int iLen2 = std::min (iLen1, (unsigned int)(szEndDir2 - (const char*)pcShaders->NAME) );

				bool bSuccess = true;
				for (unsigned int a = 0; a  < iLen2;++a)
				{
					char sz = tolower ( pcShaders->NAME[a] );
					char sz2 = tolower ( this->m_pcHeader->NAME[a] );
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

			// now try to find out whether we have this shader already
			bool bHave = false;
			for (unsigned int p = 0; p < iNumMaterials;++p)
			{
				if (iDefaultMatIndex == p)continue;

				aiString szOut;
				if(AI_SUCCESS == aiGetMaterialString ( (aiMaterial*)pScene->mMaterials[p],
					AI_MATKEY_TEXBLEND_DIFFUSE(0),&szOut))
				{
					if (0 == ASSIMP_stricmp(szOut.data,szEndDir2))
					{
						// equal. reuse this material (texture)
						bHave = true;
						pcMesh->mMaterialIndex = p;
						break;
					}
				}
			}

			if (!bHave)
			{
				MaterialHelper* pcHelper = new MaterialHelper();

				if (szEndDir2)
				{
					aiString szString;
					const size_t iLen = strlen(szEndDir2);
					memcpy(szString.data,szEndDir2,iLen+1);
					szString.length = iLen-1;

					pcHelper->AddProperty(&szString,AI_MATKEY_TEXTURE_DIFFUSE(0));
				}

				int iMode = (int)aiShadingMode_Gouraud;
				pcHelper->AddProperty<int>(&iMode, 1, AI_MATKEY_SHADING_MODEL);

				aiColor3D clr;
				clr.b = clr.g = clr.r = 1.0f;
				pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_DIFFUSE);
				pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_SPECULAR);

				clr.b = clr.g = clr.r = 0.05f;
				pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_AMBIENT);

				pScene->mMaterials[iNumMaterials] = (aiMaterial*)pcHelper;
				pcMesh->mMaterialIndex = iNumMaterials++;
			}
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
				pcMesh->mMaterialIndex = iNumMaterials++;
			}
		}
		pcSurfaces = (const MD3::Surface*)(((unsigned char*)pcSurfaces) + pcSurfaces->OFS_END);
	}

	if (0 == pScene->mNumMeshes)
	{
		// cleanup before returning
		delete pScene;
		throw new ImportErrorException( "Invalid md3 file: File contains no valid mesh");
	}
	pScene->mNumMaterials = iNumMaterials;

	// now we need to generate an empty node graph
	pScene->mRootNode = new aiNode();
	pScene->mRootNode->mNumChildren = pScene->mNumMeshes;
	pScene->mRootNode->mChildren = new aiNode*[pScene->mNumMeshes];

	for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
	{
		pScene->mRootNode->mChildren[i] = new aiNode();
		pScene->mRootNode->mChildren[i]->mParent = pScene->mRootNode;
		pScene->mRootNode->mChildren[i]->mNumMeshes = 1;
		pScene->mRootNode->mChildren[i]->mMeshes = new unsigned int[1];
		pScene->mRootNode->mChildren[i]->mMeshes[0] = i;
	}

	delete[] this->mBuffer;
	return;
}