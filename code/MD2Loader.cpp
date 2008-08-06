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

/** @file Implementation of the MD2 importer class */
#include "MD2Loader.h"
#include "MaterialSystem.h"
#include "MD2NormalTable.h" // shouldn't be included by other units

#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"
#include "../include/aiAssert.h"
#include "../include/DefaultLogger.h"

#include <boost/scoped_ptr.hpp>

using namespace Assimp;
using namespace Assimp::MD2;


// helper macro to determine the size of an array
#if (!defined ARRAYSIZE)
#	define ARRAYSIZE(_array) (int(sizeof(_array) / sizeof(_array[0])))
#endif 

// ------------------------------------------------------------------------------------------------
// Helper function to lookup a normal in Quake 2's precalculated table
void MD2::LookupNormalIndex(uint8_t iNormalIndex,aiVector3D& vOut)
{
	// make sure the normal index has a valid value
	if (iNormalIndex >= ARRAYSIZE(g_avNormals))
	{
		DefaultLogger::get()->warn("Index overflow in MDL7 normal vector list (the "
			" LUT has only 162 entries). ");

		iNormalIndex = ARRAYSIZE(g_avNormals) - 1;
	}
	vOut = *((const aiVector3D*)(&g_avNormals[iNormalIndex]));
}


// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
MD2Importer::MD2Importer()
{
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
MD2Importer::~MD2Importer()
{
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool MD2Importer::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
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
	if (extension[3] != '2')return false;

	return true;
}
// ------------------------------------------------------------------------------------------------
// Validate the file header
void MD2Importer::ValidateHeader( )
{
	/* to be validated:
	int32_t offsetSkins; 
	int32_t offsetTexCoords; 
	int32_t offsetTriangles; 
	int32_t offsetFrames; 
	//int32_t offsetGlCommands; 
	int32_t offsetEnd; 
	*/

	if (this->m_pcHeader->offsetSkins	+ this->m_pcHeader->numSkins * sizeof (MD2::Skin)			>= this->fileSize ||
		this->m_pcHeader->offsetTexCoords + this->m_pcHeader->numTexCoords * sizeof (MD2::TexCoord) >= this->fileSize ||
		this->m_pcHeader->offsetTriangles + this->m_pcHeader->numTriangles * sizeof (MD2::Triangle) >= this->fileSize ||
		this->m_pcHeader->offsetFrames	  + this->m_pcHeader->numFrames * sizeof (MD2::Frame)		>= this->fileSize ||
		this->m_pcHeader->offsetEnd			> this->fileSize)
	{
		throw new ImportErrorException("Invalid MD2 header: some offsets are outside the file");
	}

	if (this->m_pcHeader->numSkins > AI_MD2_MAX_SKINS)
		DefaultLogger::get()->warn("The model contains more skins than Quake 2 supports");
	if ( this->m_pcHeader->numFrames > AI_MD2_MAX_FRAMES)
		DefaultLogger::get()->warn("The model contains more frames than Quake 2 supports");
	if (this->m_pcHeader->numVertices > AI_MD2_MAX_VERTS)
		DefaultLogger::get()->warn("The model contains more vertices than Quake 2 supports");
}
// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void MD2Importer::InternReadFile( 
								 const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile));

	// Check whether we can read from the file
	if( file.get() == NULL)
	{
		throw new ImportErrorException( "Failed to open md2 file " + pFile + ".");
	}

	// check whether the md3 file is large enough to contain
	// at least the file header
	fileSize = (unsigned int)file->FileSize();
	if( fileSize < sizeof(MD2::Header))
	{
		throw new ImportErrorException( "md2 File is too small.");
	}

	try
	{
		// allocate storage and copy the contents of the file to a memory buffer
		this->mBuffer = new unsigned char[fileSize];
		file->Read( (void*)mBuffer, 1, fileSize);

		this->m_pcHeader = (const MD2::Header*)this->mBuffer;

		// check magic number
		if (this->m_pcHeader->magic != AI_MD2_MAGIC_NUMBER_BE &&
			this->m_pcHeader->magic != AI_MD2_MAGIC_NUMBER_LE)
		{
			throw new ImportErrorException( "Invalid md2 file: Magic bytes not found");
		}

		// check file format version
		if (this->m_pcHeader->version != 8)
		{
			DefaultLogger::get()->warn( "Unsupported md2 file version. Continuing happily ...");
		}
		this->ValidateHeader();

		// check some values whether they are valid
		if (0 == this->m_pcHeader->numFrames)
		{
			throw new ImportErrorException( "Invalid md2 file: NUM_FRAMES is 0");
		}
		if (this->m_pcHeader->offsetEnd > (int32_t)fileSize)
		{
			throw new ImportErrorException( "Invalid md2 file: File is too small");
		}

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
		aiMesh* pcMesh = pScene->mMeshes[0] = new aiMesh();

		// navigate to the begin of the frame data
		const MD2::Frame* pcFrame = (const MD2::Frame*) (
			(unsigned char*)this->m_pcHeader + this->m_pcHeader->offsetFrames);

		// navigate to the begin of the triangle data
		MD2::Triangle* pcTriangles = (MD2::Triangle*) (
			(unsigned char*)this->m_pcHeader + this->m_pcHeader->offsetTriangles);

		// navigate to the begin of the tex coords data
		const MD2::TexCoord* pcTexCoords = (const MD2::TexCoord*) (
			(unsigned char*)this->m_pcHeader + this->m_pcHeader->offsetTexCoords);

		// navigate to the begin of the vertex data
		const MD2::Vertex* pcVerts = (const MD2::Vertex*) (pcFrame->vertices);

		pcMesh->mNumFaces = this->m_pcHeader->numTriangles;
		pcMesh->mFaces = new aiFace[this->m_pcHeader->numTriangles];

		// allocate output storage
		pcMesh->mNumVertices = (unsigned int)pcMesh->mNumFaces*3;
		pcMesh->mVertices = new aiVector3D[pcMesh->mNumVertices];
		pcMesh->mNormals = new aiVector3D[pcMesh->mNumVertices];

		// not sure whether there are MD2 files without texture coordinates
		// NOTE: texture coordinates can be there without a texture,
		// but a texture can't be there without a valid UV channel
		if (this->m_pcHeader->numTexCoords && this->m_pcHeader->numSkins)
		{
			// navigate to the first texture associated with the mesh
			const MD2::Skin* pcSkins = (const MD2::Skin*) ((unsigned char*)this->m_pcHeader + 
				this->m_pcHeader->offsetSkins);

			const int iMode = (int)aiShadingMode_Gouraud;
			MaterialHelper* pcHelper = (MaterialHelper*)pScene->mMaterials[0];
			pcHelper->AddProperty<int>(&iMode, 1, AI_MATKEY_SHADING_MODEL);

			aiColor3D clr;
			clr.b = clr.g = clr.r = 1.0f;
			pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_DIFFUSE);
			pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_SPECULAR);

			clr.b = clr.g = clr.r = 0.05f;
			pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_AMBIENT);

			if (pcSkins->name[0])
			{
				aiString szString;
				const size_t iLen = ::strlen(pcSkins->name);
				::memcpy(szString.data,pcSkins->name,iLen);
				szString.data[iLen] = '\0';
				szString.length = iLen;

				pcHelper->AddProperty(&szString,AI_MATKEY_TEXTURE_DIFFUSE(0));
			}
			else
			{
				DefaultLogger::get()->warn("Texture file name has zero length. It will be skipped.");
			}
		}
		else
		{
			// apply a default material
			const int iMode = (int)aiShadingMode_Gouraud;
			MaterialHelper* pcHelper = (MaterialHelper*)pScene->mMaterials[0];
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
		}


		// now read all triangles of the first frame, apply scaling and translation
		unsigned int iCurrent = 0;

		float fDivisorU,fDivisorV;
		if (this->m_pcHeader->numTexCoords)
		{
			// allocate storage for texture coordinates, too
			pcMesh->mTextureCoords[0] = new aiVector3D[pcMesh->mNumVertices];
			pcMesh->mNumUVComponents[0] = 2;

			// check whether the skin width or height are zero (this would
			// cause a division through zero)
			if (!this->m_pcHeader->skinWidth)
			{
				DefaultLogger::get()->error("Skin width is zero but there are "
					"valid absolute texture coordinates. Unable to compute "
					"relative texture coordinates ranging from 0 to 1");
				fDivisorU = 1.0f;
			}
			else fDivisorU = (float)this->m_pcHeader->skinWidth;
			if (!this->m_pcHeader->skinHeight)
			{
				DefaultLogger::get()->error("Skin height is zero but there are "
					"valid absolute texture coordinates. Unable to compute "
					"relative texture coordinates ranging from 0 to 1");
				fDivisorV = 1.0f;
			}
			else fDivisorV = (float)this->m_pcHeader->skinHeight;
		}

		for (unsigned int i = 0; i < (unsigned int)this->m_pcHeader->numTriangles;++i)
		{
			// allocate the face
			pScene->mMeshes[0]->mFaces[i].mIndices = new unsigned int[3];
			pScene->mMeshes[0]->mFaces[i].mNumIndices = 3;

			// copy texture coordinates
			// check whether they are different from the previous value at this index.
			// In this case, create a full separate set of vertices/normals/texcoords
			unsigned int iTemp = iCurrent;
			for (unsigned int c = 0; c < 3;++c,++iCurrent)
			{
				// validate vertex indices
				if (pcTriangles[i].vertexIndices[c] >= this->m_pcHeader->numVertices)
				{
					DefaultLogger::get()->error("Vertex index is outside the allowed range");
					pcTriangles[i].vertexIndices[c] = this->m_pcHeader->numVertices-1;
				}

				// copy face indices
				unsigned int iIndex = (unsigned int)pcTriangles[i].vertexIndices[c];

				// read x,y, and z component of the vertex
				aiVector3D& vec = pcMesh->mVertices[iCurrent];

				vec.x = (float)pcVerts[iIndex].vertex[0] * pcFrame->scale[0];
				vec.x += pcFrame->translate[0];

				// (flip z and y component)
				// FIX: no .... invert y instead
				vec.y = (float)pcVerts[iIndex].vertex[1] * pcFrame->scale[1];
				vec.y += pcFrame->translate[1];
				vec.y *= -1.0f;

				vec.z = (float)pcVerts[iIndex].vertex[2] * pcFrame->scale[2];
				vec.z += pcFrame->translate[2];

				// read the normal vector from the precalculated normal table
				aiVector3D& vNormal = pcMesh->mNormals[iCurrent];
				LookupNormalIndex(pcVerts[iIndex].lightNormalIndex,vNormal);
				vNormal.y *= -1.0f;

				if (this->m_pcHeader->numTexCoords)
				{
					// validate texture coordinates
					if (pcTriangles[iIndex].textureIndices[c] >= this->m_pcHeader->numTexCoords)
					{
						DefaultLogger::get()->error("UV index is outside the allowed range");
						pcTriangles[iIndex].textureIndices[c] = this->m_pcHeader->numTexCoords-1;
					}

					aiVector3D& pcOut = pcMesh->mTextureCoords[0][iCurrent];
					float u,v;

					// the texture coordinates are absolute values but we
					// need relative values between 0 and 1
					u = (float)pcTexCoords[pcTriangles[i].textureIndices[c]].s / fDivisorU;
					v = (float)pcTexCoords[pcTriangles[i].textureIndices[c]].t / fDivisorV;
					pcOut.x = u;
					pcOut.y = 1.0f - v; // FIXME: Is this correct for MD2?
				}
			}
			// FIX: flip the face order for use with OpenGL
			pScene->mMeshes[0]->mFaces[i].mIndices[0] = iTemp+2;
			pScene->mMeshes[0]->mFaces[i].mIndices[1] = iTemp+1;
			pScene->mMeshes[0]->mFaces[i].mIndices[2] = iTemp+0;
		}
	}
	catch (ImportErrorException* ex)
	{
		delete[] this->mBuffer; AI_DEBUG_INVALIDATE_PTR(this->mBuffer);
		throw ex;
	}
	delete[] this->mBuffer; AI_DEBUG_INVALIDATE_PTR(this->mBuffer);
}