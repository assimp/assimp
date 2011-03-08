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

/** @file Exporter.cpp

Assimp export interface. While it's public interface bears many similarities
to the import interface (in fact, it is largely symmetric), the internal
implementations differs a lot. Exporters are considered stateless and are
simple callbacks which we maintain in a global list along with their
description strings.

Here we implement only the C++ interface (Assimp::Exporter).
*/

#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_EXPORT

#include "DefaultIOSystem.h"
#include "BlobIOSystem.h" 

namespace Assimp {

// ------------------------------------------------------------------------------------------------
// Exporter worker function prototypes. Should not be necessary to #ifndef them, it's just a prototype
void ExportSceneCollada(const char*,IOSystem*, const aiScene*);
void ExportScene3DS(const char*, IOSystem*, const aiScene*) {}

/// Function pointer type of a Export worker function
typedef void (*fpExportFunc)(const char*,IOSystem*,const aiScene*);

// ------------------------------------------------------------------------------------------------
/// Internal description of an Assimp export format option
struct ExportFormatEntry
{
	/// Public description structure to be returned by aiGetExportFormatDescription()
	aiExportFormatDesc mDescription;

	// Worker function to do the actual exporting
	fpExportFunc mExportFunction;

	// Constructor to fill all entries
	ExportFormatEntry( const char* pId, const char* pDesc, const char* pExtension, fpExportFunc pFunction)
	{
		mDescription.id = pId;
		mDescription.description = pDesc;
		mDescription.fileExtension = pExtension;
		mExportFunction = pFunction;
	}
};

// ------------------------------------------------------------------------------------------------
// global array of all export formats which Assimp supports in its current build
ExportFormatEntry gExporters[] = 
{
#ifndef ASSIMP_BUILD_NO_COLLADA_EXPORTER
	ExportFormatEntry( "collada", "COLLADA - Digital Asset Exchange Schema", "dae", &ExportSceneCollada),
#endif

#ifndef ASSIMP_BUILD_NO_3DS_EXPORTER
	ExportFormatEntry( "3ds", "Autodesk 3DS (legacy format)", "3ds" , &ExportScene3DS),
#endif
};


class ExporterPimpl {
public:

	ExporterPimpl()
		: blob()
		, mIOSystem(new Assimp::DefaultIOSystem())
		, mIsDefaultIOHandler(true)
	{
		
	}

	~ExporterPimpl() 
	{
		delete blob;
	}

public:
		
	aiExportDataBlob* blob;
	boost::shared_ptr< Assimp::IOSystem > mIOSystem;
	bool mIsDefaultIOHandler;
};

#define ASSIMP_NUM_EXPORTERS (sizeof(gExporters)/sizeof(gExporters[0]))

} // end of namespace Assimp





using namespace Assimp;


// ------------------------------------------------------------------------------------------------
Exporter :: Exporter() 
: pimpl(new ExporterPimpl())
{
}


// ------------------------------------------------------------------------------------------------
Exporter :: ~Exporter()
{
	delete pimpl;
}


// ------------------------------------------------------------------------------------------------
void Exporter :: SetIOHandler( IOSystem* pIOHandler)
{
	pimpl->mIsDefaultIOHandler = !pIOHandler;
	pimpl->mIOSystem.reset(pIOHandler);
}


// ------------------------------------------------------------------------------------------------
IOSystem* Exporter :: GetIOHandler() const
{
	return pimpl->mIOSystem.get();
}


// ------------------------------------------------------------------------------------------------
bool Exporter :: IsDefaultIOHandler() const
{
	return pimpl->mIsDefaultIOHandler;
}


// ------------------------------------------------------------------------------------------------
const aiExportDataBlob* Exporter :: ExportToBlob(  const aiScene* pScene, const char* pFormatId )
{
	if (pimpl->blob) {
		delete pimpl->blob;
		pimpl->blob = NULL;
	}


	boost::shared_ptr<IOSystem> old = pimpl->mIOSystem;

	BlobIOSystem* blobio = new BlobIOSystem();
	pimpl->mIOSystem = boost::shared_ptr<IOSystem>( blobio );

	if (AI_SUCCESS != Export(pScene,pFormatId,blobio->GetMagicFileName())) {
		pimpl->mIOSystem = old;
		return NULL;
	}

	pimpl->blob = blobio->GetBlobChain();
	pimpl->mIOSystem = old;

	return pimpl->blob;
}


// ------------------------------------------------------------------------------------------------
aiReturn Exporter :: Export( const aiScene* pScene, const char* pFormatId, const char* pPath )
{
	ASSIMP_BEGIN_EXCEPTION_REGION();
	for (size_t i = 0; i < ASSIMP_NUM_EXPORTERS; ++i) {
		if (!strcmp(gExporters[i].mDescription.id,pFormatId)) {

			try {
				gExporters[i].mExportFunction(pPath,pimpl->mIOSystem.get(),pScene);
			}
			catch (DeadlyExportError& err) {
				// XXX what to do with the error message? Maybe introduce extra member to hold it, similar to Assimp.Importer
				DefaultLogger::get()->error(err.what());
				return AI_FAILURE;
			}
			return AI_SUCCESS;
		}
	}
	ASSIMP_END_EXCEPTION_REGION(aiReturn);
	return AI_FAILURE;
}


// ------------------------------------------------------------------------------------------------
const aiExportDataBlob* Exporter :: GetBlob() const 
{
	return pimpl->blob;
}


// ------------------------------------------------------------------------------------------------
const aiExportDataBlob* Exporter :: GetOrphanedBlob() const 
{
	const aiExportDataBlob* tmp = pimpl->blob;
	pimpl->blob = NULL;
	return tmp;
}


// ------------------------------------------------------------------------------------------------
size_t Exporter :: GetExportFormatCount() const 
{
	return ASSIMP_NUM_EXPORTERS;
}

// ------------------------------------------------------------------------------------------------
const aiExportFormatDesc* Exporter :: GetExportFormatDescription( size_t pIndex ) const 
{
	if (pIndex >= ASSIMP_NUM_EXPORTERS) {
		return NULL;
	}

	return &gExporters[pIndex].mDescription;
}


#endif // !ASSIMP_BUILD_NO_EXPORT
