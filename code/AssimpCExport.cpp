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

/** @file AssimpCExport.cpp
Assimp C export interface. See Exporter.cpp for some notes.
*/

#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_EXPORT
#include "CInterfaceIOWrapper.h" 

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
ASSIMP_API size_t aiGetExportFormatCount(void)
{
	return Exporter().GetExportFormatCount();
}


// ------------------------------------------------------------------------------------------------
ASSIMP_API const aiExportFormatDesc* aiGetExportFormatDescription( size_t pIndex)
{
	return Exporter().GetExportFormatDescription(pIndex);
}


// ------------------------------------------------------------------------------------------------
ASSIMP_API aiReturn aiExportScene( const aiScene* pScene, const char* pFormatId, const char* pFileName )
{
	return ::aiExportSceneEx(pScene,pFormatId,pFileName,NULL);
}


// ------------------------------------------------------------------------------------------------
ASSIMP_API aiReturn aiExportSceneEx( const aiScene* pScene, const char* pFormatId, const char* pFileName, aiFileIO* pIO)
{
	Exporter exp;

	if (pIO) {
		exp.SetIOHandler(new CIOSystemWrapper(pIO));
	}
	return exp.Export(pScene,pFormatId,pFileName);
}


// ------------------------------------------------------------------------------------------------
ASSIMP_API const C_STRUCT aiExportDataBlob* aiExportSceneToBlob( const aiScene* pScene, const char* pFormatId )
{
	Exporter exp;
	if (!exp.ExportToBlob(pScene,pFormatId)) {
		return NULL;
	}
	const aiExportDataBlob* blob = exp.GetOrphanedBlob();
	ai_assert(blob);

	return blob;
}

// ------------------------------------------------------------------------------------------------
ASSIMP_API C_STRUCT void aiReleaseExportData( const aiExportDataBlob* pData )
{
	delete pData;
}

#endif // !ASSIMP_BUILD_NO_EXPORT
