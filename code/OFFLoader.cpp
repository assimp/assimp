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

/** @file Implementation of the OFF importer class */

#include "AssimpPCH.h"

// internal headers
#include "OFFLoader.h"
#include "ParsingUtils.h"
#include "fast_atof.h"


using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
OFFImporter::OFFImporter()
{
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
OFFImporter::~OFFImporter()
{
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool OFFImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
{
	// simple check of file extension is enough for the moment
	std::string::size_type pos = pFile.find_last_of('.');
	// no file extension - can't read
	if( pos == std::string::npos)return false;
	std::string extension = pFile.substr( pos);


	return !(extension.length() != 4 || extension[0] != '.' ||
			 extension[1] != 'o' && extension[1] != 'O' ||
			 extension[2] != 'f' && extension[2] != 'F' ||
			 extension[3] != 'f' && extension[3] != 'F');
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void OFFImporter::InternReadFile( const std::string& pFile, 
	aiScene* pScene, IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile, "rb"));

	// Check whether we can read from the file
	if( file.get() == NULL)
		throw new ImportErrorException( "Failed to open OFF file " + pFile + ".");

	unsigned int fileSize = (unsigned int)file->FileSize();

	// allocate storage and copy the contents of the file to a memory buffer
	std::vector<char> mBuffer2(fileSize+1);
	file->Read(&mBuffer2[0], 1, fileSize);
	mBuffer2[fileSize] = '\0';
	const char* buffer = &mBuffer2[0];

	char line[4096];
	GetNextLine(buffer,line);
	if ('O' == line[0])GetNextLine(buffer,line); // skip the 'OFF' line

	const char* sz = line; SkipSpaces(&sz);
	const unsigned int numVertices = strtol10(sz,&sz);SkipSpaces(&sz);
	const unsigned int numFaces = strtol10(sz,&sz);

	pScene->mMeshes = new aiMesh*[ pScene->mNumMeshes = 1 ];
	aiMesh* mesh = pScene->mMeshes[0] = new aiMesh();
	aiFace* faces = mesh->mFaces = new aiFace [mesh->mNumFaces = numFaces];

	std::vector<aiVector3D> tempPositions(numVertices);

	// now read all vertex lines
	for (unsigned int i = 0; i< numVertices;++i)
	{
		if(!GetNextLine(buffer,line))
		{
			DefaultLogger::get()->error("OFF: The number of verts in the header is incorrect");
			break;
		}
		aiVector3D& v = tempPositions[i];

		sz = line; SkipSpaces(&sz);
		sz = fast_atof_move(sz,(float&)v.x); SkipSpaces(&sz);
		sz = fast_atof_move(sz,(float&)v.y); SkipSpaces(&sz);
		fast_atof_move(sz,(float&)v.z);
	}

	
	// First find out how many vertices we'll need
	const char* old = buffer;
	for (unsigned int i = 0; i< mesh->mNumFaces;++i)
	{
		if(!GetNextLine(buffer,line))
		{
			DefaultLogger::get()->error("OFF: The number of faces in the header is incorrect");
			break;
		}
		sz = line;SkipSpaces(&sz);
		if(!(faces->mNumIndices = strtol10(sz,&sz)) || faces->mNumIndices > 9)
		{
			DefaultLogger::get()->error("OFF: Faces with zero indices aren't allowed");
			--mesh->mNumFaces;
			continue;
		}
		mesh->mNumVertices += faces->mNumIndices;
		++faces;
	}

	if (!mesh->mNumVertices)
		throw new ImportErrorException("OFF: There are no valid faces");

	// allocate storage for the output vertices
	aiVector3D* verts = mesh->mVertices = new aiVector3D[mesh->mNumVertices];

	// second: now parse all face indices
	buffer = old;faces = mesh->mFaces;
	for (unsigned int i = 0, p = 0; i< mesh->mNumFaces;)
	{
		if(!GetNextLine(buffer,line))break;

		unsigned int idx;
		sz = line;SkipSpaces(&sz);
		if(!(idx = strtol10(sz,&sz)) || idx > 9)
			continue;

		faces->mIndices = new unsigned int [faces->mNumIndices];
		for (unsigned int m = 0; m < faces->mNumIndices;++m)
		{
			SkipSpaces(&sz);
			if ((idx = strtol10(sz,&sz)) >= numVertices)
			{
				DefaultLogger::get()->error("OFF: Vertex index is out of range");
				idx = numVertices-1;
			}
			faces->mIndices[m] = p++;
			*verts++ = tempPositions[idx];
		}
		++i;
		++faces;
	}
	
	// generate the output node graph
	pScene->mRootNode = new aiNode();
	pScene->mRootNode->mName.Set("<OFFRoot>");
	pScene->mRootNode->mMeshes = new unsigned int [pScene->mRootNode->mNumMeshes = 1];
	pScene->mRootNode->mMeshes[0] = 0;

	// generate a default material
	pScene->mMaterials = new aiMaterial*[pScene->mNumMaterials = 1];
	MaterialHelper* pcMat = new MaterialHelper();

	aiColor4D clr(0.6f,0.6f,0.6f,1.0f);
	pcMat->AddProperty(&clr,1,AI_MATKEY_COLOR_DIFFUSE);
	pScene->mMaterials[0] = pcMat;
}
