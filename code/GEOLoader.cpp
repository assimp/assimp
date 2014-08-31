/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team

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

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

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

/** @file  GEOLoader.cpp
 *  @brief Implementation of the GEO importer class 
 */

#include "AssimpPCH.h"
#ifndef ASSIMP_BUILD_NO_GEO_IMPORTER

// internal headers
#include "GEOLoader.h"
#include "ParsingUtils.h"
#include "fast_atof.h"


using namespace Assimp;

#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
#	define snprintf _snprintf
#	define vsnprintf _vsnprintf
#	define strcasecmp _stricmp
#	define strncasecmp _strnicmp
#endif

static const aiImporterDesc desc = {
	"Videoscape GEO Importer",
	"",
	"",
	"",
	aiImporterFlags_SupportBinaryFlavour,
	0,
	0,
	0,
	0,
	"3DG GEO GOUR" 
};

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
GEOImporter::GEOImporter()
{}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
GEOImporter::~GEOImporter()
{}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool GEOImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler, bool checkSig) const
{
	const std::string extension = GetExtension(pFile);

	if (strcasestr(desc.mFileExtensions, extension.c_str()))
		return true;
	else if (!extension.length() || checkSig)
	{
		if (!pIOHandler)return true;
		const char* tokens[] = {"gour", "3dg"}; /* ref: 3dg1 3dg2 3dg3 gour */
		return SearchFileHeaderForToken(pIOHandler, pFile, tokens, sizeof(tokens));
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
const aiImporterDesc* GEOImporter::GetInfo () const
{
	return &desc;
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void GEOImporter::InternReadFile( const std::string& pFile, 
	aiScene* pScene, IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile, "rb"));

	// Check whether we can read from the file
	if( file.get() == NULL) {
		throw DeadlyImportError( "Failed to open GEO file " + pFile + ".");
	}
	
	// allocate storage and copy the contents of the file to a memory buffer
	std::vector<char> mBuffer2;
	TextFileToBuffer(file.get(),mBuffer2);
	const char* buffer = &mBuffer2[0];

	char line[4096];
	GetNextLine(buffer,line);
	while ('G' == line[0] || 'D' == line[1] || '#' == line[0]) {
		GetNextLine(buffer,line); // skip the signature line and comment lines (#...)
	}

	const char* sz = line; SkipSpaces(&sz);
	const unsigned int numVertices = strtoul10(sz,&sz);

	pScene->mMeshes = new aiMesh*[ pScene->mNumMeshes = 1 ];
	aiMesh* mesh = pScene->mMeshes[0] = new aiMesh();

	/* TODO: the implementation */

	// generate the output node graph
	pScene->mRootNode = new aiNode();
	pScene->mRootNode->mName.Set("<GEORoot>");
	pScene->mRootNode->mMeshes = new unsigned int [pScene->mRootNode->mNumMeshes = 1];
	pScene->mRootNode->mMeshes[0] = 0;
}

#endif // !! ASSIMP_BUILD_NO_GEO_IMPORTER
