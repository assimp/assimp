/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team

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

#include "AssimpPCH.h"

#if 0

// ------------------------------------------------------------------------------------------------
// Exporters
// ------------------------------------------------------------------------------------------------
#ifndef ASSIMP_BUILD_NO_COLLADA_EXPORTER
//#	include "ColladaExporter.h"
void ExportSceneCollada (aiExportDataBlob*, const aiScene*);
#endif
#ifndef ASSIMP_BUILD_NO_3DS_EXPORTER
void ExportScene3DS(aiExportDataBlob*, const aiScene*);
#endif


// ------------------------------------------------------------------------------------------------
// Table of export format descriptors along with the corresponding exporter functions
// ------------------------------------------------------------------------------------------------
namespace Assimp {

	typedef void (*fpExportFunc) (aiExportDataBlob*, const aiScene*) /* throw DeadlyExportError */;

	//
	aiExportFormatDesc g_aExportInfo[] = {
		{
			  "collada"
			, "COLLADA Open Standard for Painful Asset Interchange"
			, "dae"
		},
		{
			  "3ds"
			, "Autodesk(tm) 3DS file format"
			, "3ds"
		}
	};

	fpExportFunc g_aExportFunctions[] = {
		ExportSceneCollada,
		ExportScene3DS
	};
}



// ------------------------------------------------------------------------------------------------
ASSIMP_API size_t aiGetExportFormatCount(void)
{
	return 0;
}


// ------------------------------------------------------------------------------------------------
ASSIMP_API const C_STRUCT aiExportFormatDesc* aiGetExportFormatDescription( size_t pIndex)
{
	return NULL;
}


// ------------------------------------------------------------------------------------------------
ASSIMP_API const C_STRUCT aiExportDataBlob* aiExportScene( const aiScene* pScene, const char* pFormatId )
{
	return NULL;
}


// ------------------------------------------------------------------------------------------------
ASSIMP_API C_STRUCT aiReturn aiWriteBlobToFile( const C_STRUCT aiExportDataBlob* pBlob, const char* pPath, const aiFileIO* pIOSystem )
{
	return AI_FAILURE;
}


// ------------------------------------------------------------------------------------------------
ASSIMP_API C_STRUCT void aiReleaseExportData( const aiExportDataBlob* pData )
{
	delete pData;
}


#endif
