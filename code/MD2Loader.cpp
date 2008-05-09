/*
---------------------------------------------------------------------------
Free Asset Import Library (ASSIMP)
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

#include "MD2NormalTable.h"

#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"
#include "../include/aiAssert.h"

#include <boost/scoped_ptr.hpp>

using namespace Assimp;

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

	// not brilliant but working ;-)
	if( extension == ".md2" || extension == ".MD2" || 
		extension == ".mD2" || extension == ".Md2")
		return true;

	return false;
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
	size_t fileSize = file->FileSize();
	if( fileSize < sizeof(MD2::Header))
	{
		throw new ImportErrorException( ".md2 File is too small.");
	}

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
		throw new ImportErrorException( "Unsupported md3 file version");
	}

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
	pScene->mMeshes[0] = new aiMesh();

	// navigate to the begin of the frame data
	const MD2::Frame* pcFrame = (const MD2::Frame*) ((unsigned char*)this->m_pcHeader + 
		this->m_pcHeader->offsetFrames);

	// navigate to the begin of the triangle data
	MD2::Triangle* pcTriangles = (MD2::Triangle*) ((unsigned char*)this->m_pcHeader + 
		this->m_pcHeader->offsetTriangles);

	// navigate to the begin of the tex coords data
	const MD2::TexCoord* pcTexCoords = (const MD2::TexCoord*) ((unsigned char*)this->m_pcHeader + 
		this->m_pcHeader->offsetTexCoords);

	// navigate to the begin of the vertex data
	const MD2::Vertex* pcVerts = (const MD2::Vertex*) (pcFrame->vertices);

	pScene->mMeshes[0]->mNumFaces = this->m_pcHeader->numTriangles;
	pScene->mMeshes[0]->mFaces = new aiFace[this->m_pcHeader->numTriangles];

	// temporary vectors for position/texture coordinates/normals
	std::vector<aiVector3D> vPositions;
	std::vector<aiVector3D> vTexCoords;
	std::vector<aiVector3D> vNormals;

	vPositions.resize(pScene->mMeshes[0]->mNumFaces*3,aiVector3D());
	vTexCoords.resize(pScene->mMeshes[0]->mNumFaces*3,aiVector3D(
		std::numeric_limits<float>::quiet_NaN(),
		std::numeric_limits<float>::quiet_NaN(),0.0f));
	vNormals.resize(pScene->mMeshes[0]->mNumFaces*3,aiVector3D());

	// not sure whether there are MD2 files without texture coordinates
	if (0 != this->m_pcHeader->numTexCoords && 0 != this->m_pcHeader->numSkins)
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

		aiString szString;
		const size_t iLen = strlen(pcSkins->name);
		memcpy(szString.data,pcSkins->name,iLen+1);
		szString.length = iLen-1;

		pcHelper->AddProperty(&szString,AI_MATKEY_TEXTURE_DIFFUSE(0));
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
	}


	// now read all triangles of the first frame, apply scaling and translation
	unsigned int iCurrent = 0;
	if (0 != this->m_pcHeader->numTexCoords)
	{
		for (unsigned int i = 0; i < (unsigned int)this->m_pcHeader->numTriangles;++i)
		{
			// allocate the face
			pScene->mMeshes[0]->mFaces[i].mIndices = new unsigned int[3];
			pScene->mMeshes[0]->mFaces[i].mNumIndices = 3;

			// copy texture coordinates
			// check whether they are different from the previous value at this index.
			// In this case, create a full separate set of vertices/normals/texcoords
			for (unsigned int c = 0; c < 3;++c,++iCurrent)
			{
				pScene->mMeshes[0]->mFaces[i].mIndices[c] = iCurrent;

				// validate vertex indices
				if (pcTriangles[i].vertexIndices[c] >= this->m_pcHeader->numVertices)
					pcTriangles[i].vertexIndices[c] = this->m_pcHeader->numVertices-1;

				// copy face indices
				unsigned int iIndex = (unsigned int)pcTriangles[i].vertexIndices[c];

				// read x,y, and z component of the vertex
				aiVector3D& vec = vPositions[iCurrent];

				vec.x = (float)pcVerts[iIndex].vertex[0] * pcFrame->scale[0];
				vec.x += pcFrame->translate[0];

				// (flip z and y component)
				vec.z = (float)pcVerts[iIndex].vertex[1] * pcFrame->scale[1];
				vec.z += pcFrame->translate[1];

				vec.y = (float)pcVerts[iIndex].vertex[2] * pcFrame->scale[2];
				vec.y += pcFrame->translate[2];

				// read the normal vector from the precalculated normal table
				vNormals[iCurrent] = *((const aiVector3D*)(&g_avNormals[std::min(
					int(pcVerts[iIndex].lightNormalIndex),
					int(sizeof(g_avNormals) / sizeof(g_avNormals[0]))-1)]));

				std::swap ( vNormals[iCurrent].y,vNormals[iCurrent].z );

				// validate texture coordinates
				if (pcTriangles[iIndex].textureIndices[c] >= this->m_pcHeader->numTexCoords)
					pcTriangles[iIndex].textureIndices[c] = this->m_pcHeader->numTexCoords-1;

				aiVector3D* pcOut = &vTexCoords[iCurrent];
				float u,v;
				u = (float)pcTexCoords[pcTriangles[i].textureIndices[c]].s / this->m_pcHeader->skinWidth;
				v = (float)pcTexCoords[pcTriangles[i].textureIndices[c]].t / this->m_pcHeader->skinHeight;
				pcOut->x = u;
				pcOut->y = v;
			}
		}
	}
	else
	{
		for (unsigned int i = 0; i < (unsigned int)this->m_pcHeader->numTriangles;++i)
		{
			// allocate the face
			pScene->mMeshes[0]->mFaces[i].mIndices = new unsigned int[3];
			pScene->mMeshes[0]->mFaces[i].mNumIndices = 3;

			// copy texture coordinates
			// check whether they are different from the previous value at this index.
			// In this case, create a full separate set of vertices/normals/texcoords
			for (unsigned int c = 0; c < 3;++c,++iCurrent)
			{
				pScene->mMeshes[0]->mFaces[i].mIndices[c] = iCurrent;

				// validate vertex indices
				if (pcTriangles[i].vertexIndices[c] >= this->m_pcHeader->numVertices)
					pcTriangles[i].vertexIndices[c] = this->m_pcHeader->numVertices-1;

				// copy face indices
				unsigned int iIndex = (unsigned int)pcTriangles[i].vertexIndices[c];

				// read x,y, and z component of the vertex
				aiVector3D& vec = vPositions[iCurrent];

				vec.x = (float)pcVerts[iIndex].vertex[0] * pcFrame->scale[0];
				vec.x += pcFrame->translate[0];

				// (flip z and y component)
				vec.z = (float)pcVerts[iIndex].vertex[1] * pcFrame->scale[1];
				vec.z += pcFrame->translate[1];

				vec.y = (float)pcVerts[iIndex].vertex[2] * pcFrame->scale[2];
				vec.y += pcFrame->translate[2];

				// read the normal vector from the precalculated normal table
				vNormals[iCurrent] = *((const aiVector3D*)(&g_avNormals[std::min(
					int(pcVerts[iIndex].lightNormalIndex),
					int(sizeof(g_avNormals) / sizeof(g_avNormals[0]))-1)]));

				std::swap ( vNormals[iCurrent].y,vNormals[iCurrent].z );
			
				aiVector3D* pcOut = &vTexCoords[iCurrent];
				pcOut->x = (float)pcTexCoords[pcTriangles[i].textureIndices[c]].s / this->m_pcHeader->skinWidth;
				pcOut->y = (float)pcTexCoords[pcTriangles[i].textureIndices[c]].t / this->m_pcHeader->skinHeight;
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
	
	if (0 != this->m_pcHeader->numTexCoords)
	{
		memcpy(pScene->mMeshes[0]->mTextureCoords[0],	&vTexCoords[0],	
			vPositions.size() * sizeof(aiVector3D));
	}

	return;
}