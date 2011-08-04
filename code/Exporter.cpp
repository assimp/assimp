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
#include "SceneCombiner.h" 
#include "BaseProcess.h" 
#include "Importer.h" // need this for GetPostProcessingStepInstanceList()

#include "MakeVerboseFormat.h"
#include "ConvertToLHProcess.h"

namespace Assimp {

// PostStepRegistry.cpp
void GetPostProcessingStepInstanceList(std::vector< BaseProcess* >& out);

// ------------------------------------------------------------------------------------------------
// Exporter worker function prototypes. Should not be necessary to #ifndef them, it's just a prototype
void ExportSceneCollada(const char*,IOSystem*, const aiScene*);
void ExportSceneObj(const char*,IOSystem*, const aiScene*);
void ExportSceneSTL(const char*,IOSystem*, const aiScene*);
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

	// Postprocessing steps to be executed PRIOR to calling mExportFunction
	unsigned int mEnforcePP;

	// Constructor to fill all entries
	ExportFormatEntry( const char* pId, const char* pDesc, const char* pExtension, fpExportFunc pFunction, unsigned int pEnforcePP = 0u)
	{
		mDescription.id = pId;
		mDescription.description = pDesc;
		mDescription.fileExtension = pExtension;
		mExportFunction = pFunction;
		mEnforcePP = pEnforcePP;
	}
};

// ------------------------------------------------------------------------------------------------
// global array of all export formats which Assimp supports in its current build
ExportFormatEntry gExporters[] = 
{
#ifndef ASSIMP_BUILD_NO_COLLADA_EXPORTER
	ExportFormatEntry( "collada", "COLLADA - Digital Asset Exchange Schema", "dae", &ExportSceneCollada),
#endif

#ifndef ASSIMP_BUILD_NO_OBJ_EXPORTER
	ExportFormatEntry( "obj", "Wavefront OBJ format", "obj", &ExportSceneObj),
#endif

#ifndef ASSIMP_BUILD_NO_STL_EXPORTER
	ExportFormatEntry( "stl", "Stereolithography", "stl" , &ExportSceneSTL, 
		aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_PreTransformVertices
	),
#endif

//#ifndef ASSIMP_BUILD_NO_3DS_EXPORTER
//	ExportFormatEntry( "3ds", "Autodesk 3DS (legacy format)", "3ds" , &ExportScene3DS),
//#endif
};


class ExporterPimpl {
public:

	ExporterPimpl()
		: blob()
		, mIOSystem(new Assimp::DefaultIOSystem())
		, mIsDefaultIOHandler(true)
	{
		GetPostProcessingStepInstanceList(mPostProcessingSteps);
	}

	~ExporterPimpl() 
	{
		delete blob;

		// Delete all post-processing plug-ins
		for( unsigned int a = 0; a < mPostProcessingSteps.size(); a++) {
			delete mPostProcessingSteps[a];
		}
	}

public:
		
	aiExportDataBlob* blob;
	boost::shared_ptr< Assimp::IOSystem > mIOSystem;
	bool mIsDefaultIOHandler;

	/** Post processing steps we can apply at the imported data. */
	std::vector< BaseProcess* > mPostProcessingSteps;

	/** Last fatal export error */
	std::string mError;
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
	FreeBlob();
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
const aiExportDataBlob* Exporter :: ExportToBlob(  const aiScene* pScene, const char* pFormatId, unsigned int pPreprocessing )
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
aiReturn Exporter :: Export( const aiScene* pScene, const char* pFormatId, const char* pPath, unsigned int pPreprocessing )
{
	ASSIMP_BEGIN_EXCEPTION_REGION();

	pimpl->mError = "";
	for (size_t i = 0; i < ASSIMP_NUM_EXPORTERS; ++i) {
		if (!strcmp(gExporters[i].mDescription.id,pFormatId)) {

			try {

				// Always create a full copy of the scene. We might optimize this one day, 
				// but for now it is the most pragmatic way.
				aiScene* scenecopy_tmp;
				SceneCombiner::CopyScene(&scenecopy_tmp,pScene);

				std::auto_ptr<aiScene> scenecopy(scenecopy_tmp);
				const ScenePrivateData* const priv = ScenePriv(pScene);

				// steps that are not idempotent, i.e. we might need to run them again, usually to get back to the
				// original state before the step was applied first. When checking which steps we don't need
				// to run, those are excluded.
				const unsigned int nonIdempotentSteps = aiProcess_FlipWindingOrder | aiProcess_FlipUVs | aiProcess_MakeLeftHanded;

				// Erase all pp steps that were already applied to this scene
				unsigned int pp = (gExporters[i].mEnforcePP | pPreprocessing) & ~(priv 
					? (priv->mPPStepsApplied & ~nonIdempotentSteps)
					: 0u);

				// If no extra postprocessing was specified, and we obtained this scene from an
				// Assimp importer, apply the reverse steps automatically.
				if (!pPreprocessing && priv) {
					pp |= (nonIdempotentSteps & priv->mPPStepsApplied);
				}

				// If the input scene is not in verbose format, but there is at least postprocessing step that relies on it,
				// we need to run the MakeVerboseFormat step first.
				if (scenecopy->mFlags & AI_SCENE_FLAGS_NON_VERBOSE_FORMAT) {
					
					bool verbosify = false;
					for( unsigned int a = 0; a < pimpl->mPostProcessingSteps.size(); a++) {
						BaseProcess* const p = pimpl->mPostProcessingSteps[a];

						if (p->IsActive(pp) && p->RequireVerboseFormat()) {
							verbosify = true;
							break;
						}
					}

					if (verbosify || (gExporters[i].mEnforcePP & aiProcess_JoinIdenticalVertices)) {
						DefaultLogger::get()->debug("export: Scene data not in verbose format, applying MakeVerboseFormat step first");

						MakeVerboseFormatProcess proc;
						proc.Execute(scenecopy.get());
					}
				}

				if (pp) {
					// the three 'conversion' steps need to be executed first because all other steps rely on the standard data layout
					{
						FlipWindingOrderProcess step;
						if (step.IsActive(pp)) {
							step.Execute(scenecopy.get());
						}
					}
					
					{
						FlipUVsProcess step;
						if (step.IsActive(pp)) {
							step.Execute(scenecopy.get());
						}
					}

					{
						MakeLeftHandedProcess step;
						if (step.IsActive(pp)) {
							step.Execute(scenecopy.get());
						}
					}

					// dispatch other processes
					for( unsigned int a = 0; a < pimpl->mPostProcessingSteps.size(); a++) {
						BaseProcess* const p = pimpl->mPostProcessingSteps[a];

						if (p->IsActive(pp) 
							&& !dynamic_cast<FlipUVsProcess*>(p) 
							&& !dynamic_cast<FlipWindingOrderProcess*>(p) 
							&& !dynamic_cast<MakeLeftHandedProcess*>(p)) {

							p->Execute(scenecopy.get());
						}
					}
					ScenePrivateData* const privOut = ScenePriv(scenecopy.get());
					ai_assert(privOut);

					privOut->mPPStepsApplied |= pp;
				}

				gExporters[i].mExportFunction(pPath,pimpl->mIOSystem.get(),scenecopy.get());
			}
			catch (DeadlyExportError& err) {
				pimpl->mError = err.what();
				return AI_FAILURE;
			}
			return AI_SUCCESS;
		}
	}

	pimpl->mError = std::string("Found no exporter to handle this file format: ") + pFormatId;
	ASSIMP_END_EXCEPTION_REGION(aiReturn);
	return AI_FAILURE;
}


// ------------------------------------------------------------------------------------------------
const char* Exporter :: GetErrorString() const
{
	return pimpl->mError.c_str();
}


// ------------------------------------------------------------------------------------------------
void Exporter :: FreeBlob( )
{
	delete pimpl->blob;
	pimpl->blob = NULL;

	pimpl->mError = "";
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
