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

/** @file Implementation of the CPP-API class #Importer */
#include <fstream>
#include <string>

#include "../include/assimp.hpp"
#include "../include/aiAssert.h"
#include "../include/aiScene.h"
#include "../include/aiPostProcess.h"
#include "../include/DefaultLogger.h"

#include "BaseImporter.h"
#include "BaseProcess.h"
#include "DefaultIOStream.h"
#include "DefaultIOSystem.h"

// Importers
#if (!defined AI_BUILD_NO_X_IMPORTER)
#	include "XFileImporter.h"
#endif
#if (!defined AI_BUILD_NO_3DS_IMPORTER)
#	include "3DSLoader.h"
#endif
#if (!defined AI_BUILD_NO_MD3_IMPORTER)
#	include "MD3Loader.h"
#endif
#if (!defined AI_BUILD_NO_MD4_IMPORTER)
#	include "MD4Loader.h"
#endif
#if (!defined AI_BUILD_NO_MDL_IMPORTER)
#	include "MDLLoader.h"
#endif
#if (!defined AI_BUILD_NO_MD2_IMPORTER)
#	include "MD2Loader.h"
#endif
#if (!defined AI_BUILD_NO_PLY_IMPORTER)
#	include "PlyLoader.h"
#endif
#if (!defined AI_BUILD_NO_ASE_IMPORTER)
#	include "ASELoader.h"
#endif
#if (!defined AI_BUILD_NO_OBJ_IMPORTER)
#	include "ObjFileImporter.h"
#endif
#if (!defined AI_BUILD_NO_HMP_IMPORTER)
#	include "HMPLoader.h"
#endif
#if (!defined AI_BUILD_NO_SMD_IMPORTER)
#	include "SMDLoader.h"
#endif

// PostProcess-Steps
#if (!defined AI_BUILD_NO_CALCTANGENTS_PROCESS)
#	include "CalcTangentsProcess.h"
#endif
#if (!defined AI_BUILD_NO_JOINVERTICES_PROCESS)
#	include "JoinVerticesProcess.h"
#endif
#if (!defined AI_BUILD_NO_CONVERTTOLH_PROCESS)
#	include "ConvertToLHProcess.h"
#endif
#if (!defined AI_BUILD_NO_TRIANGULATE_PROCESS)
#	include "TriangulateProcess.h"
#endif
#if (!defined AI_BUILD_NO_GENFACENORMALS_PROCESS)
#	include "GenFaceNormalsProcess.h"
#endif
#if (!defined AI_BUILD_NO_GENVERTEXNORMALS_PROCESS)
#	include "GenVertexNormalsProcess.h"
#endif
#if (!defined AI_BUILD_NO_KILLNORMALS_PROCESS)
#	include "KillNormalsProcess.h"
#endif
#if (!defined AI_BUILD_NO_SPLITLARGEMESHES_PROCESS)
#	include "SplitLargeMeshes.h"
#endif
#if (!defined AI_BUILD_NO_PRETRANSFORMVERTICES_PROCESS)
#	include "PretransformVertices.h"
#endif
#if (!defined AI_BUILD_NO_LIMITBONEWEIGHTS_PROCESS)
#	include "LimitBoneWeightsProcess.h"
#endif
#if (!defined AI_BUILD_NO_VALIDATEDS_PROCESS)
#	include "ValidateDataStructure.h"
#endif

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor. 
Importer::Importer() :
	mIOHandler(NULL),
	mScene(NULL),
	mErrorString("")	
{
	// allocate a default IO handler
	mIOHandler = new DefaultIOSystem;
	mIsDefaultHandler = true;

	// add an instance of each worker class here
#if (!defined AI_BUILD_NO_X_IMPORTER)
	mImporter.push_back( new XFileImporter());
#endif
#if (!defined AI_BUILD_NO_OBJ_IMPORTER)
	mImporter.push_back( new ObjFileImporter());
#endif
#if (!defined AI_BUILD_NO_3DS_IMPORTER)
	mImporter.push_back( new Dot3DSImporter());
#endif
#if (!defined AI_BUILD_NO_MD3_IMPORTER)
	mImporter.push_back( new MD3Importer());
#endif
#if (!defined AI_BUILD_NO_MD2_IMPORTER)
	mImporter.push_back( new MD2Importer());
#endif
#if (!defined AI_BUILD_NO_PLY_IMPORTER)
	mImporter.push_back( new PLYImporter());
#endif
#if (!defined AI_BUILD_NO_MDL_IMPORTER)
	mImporter.push_back( new MDLImporter());
#endif
#if (!defined AI_BUILD_NO_MD4_IMPORTER)
	mImporter.push_back( new MD4Importer());
#endif
#if (!defined AI_BUILD_NO_ASE_IMPORTER)
	mImporter.push_back( new ASEImporter());
#endif
	#if (!defined AI_BUILD_NO_HMP_IMPORTER)
	mImporter.push_back( new HMPImporter());
#endif
	#if (!defined AI_BUILD_NO_SMD_IMPORTER)
	mImporter.push_back( new SMDImporter());
#endif

	// add an instance of each post processing step here in the order 
	// of sequence it is executed
#if (!defined AI_BUILD_NO_VALIDATEDS_PROCESS)
	mPostProcessingSteps.push_back( new ValidateDSProcess());
#endif
#if (!defined AI_BUILD_NO_TRIANGULATE_PROCESS)
	mPostProcessingSteps.push_back( new TriangulateProcess());
#endif
#if (!defined AI_BUILD_NO_PRETRANSFORMVERTICES_PROCESS)
	mPostProcessingSteps.push_back( new PretransformVertices());
#endif
#if (!defined AI_BUILD_NO_SPLITLARGEMESHES_PROCESS)
	mPostProcessingSteps.push_back( new SplitLargeMeshesProcess_Triangle());
#endif
#if (!defined AI_BUILD_NO_KILLNORMALS_PROCESS)
	mPostProcessingSteps.push_back( new KillNormalsProcess());
#endif
#if (!defined AI_BUILD_NO_GENFACENORMALS_PROCESS)
	mPostProcessingSteps.push_back( new GenFaceNormalsProcess());
#endif
#if (!defined AI_BUILD_NO_GENVERTEXNORMALS_PROCESS)
	mPostProcessingSteps.push_back( new GenVertexNormalsProcess());
#endif
#if (!defined AI_BUILD_NO_CALCTANGENTS_PROCESS)
	mPostProcessingSteps.push_back( new CalcTangentsProcess());
#endif
#if (!defined AI_BUILD_NO_JOINVERTICES_PROCESS)
	mPostProcessingSteps.push_back( new JoinVerticesProcess());
#endif
#if (!defined AI_BUILD_NO_SPLITLARGEMESHES_PROCESS)
	mPostProcessingSteps.push_back( new SplitLargeMeshesProcess_Vertex());
#endif
#if (!defined AI_BUILD_NO_CONVERTTOLH_PROCESS)
	mPostProcessingSteps.push_back( new ConvertToLHProcess());
#endif
#if (!defined AI_BUILD_NO_LIMITBONEWEIGHTS_PROCESS)
	mPostProcessingSteps.push_back( new LimitBoneWeightsProcess());
#endif
}

// ------------------------------------------------------------------------------------------------
// Destructor. 
Importer::~Importer()
{
	for( unsigned int a = 0; a < mImporter.size(); a++)
		delete mImporter[a];
	for( unsigned int a = 0; a < mPostProcessingSteps.size(); a++)
		delete mPostProcessingSteps[a];

	// delete the assigned IO handler
	delete mIOHandler;

	// kill imported scene. Destructors should do that recursivly
	delete mScene;
}

// ------------------------------------------------------------------------------------------------
// Supplies a custom IO handler to the importer to open and access files.
void Importer::SetIOHandler( IOSystem* pIOHandler)
{
	if (!pIOHandler)
	{
		delete mIOHandler;
		mIOHandler = new DefaultIOSystem();
		mIsDefaultHandler = true;
	}
	else if (mIOHandler != pIOHandler)
	{
		delete mIOHandler;
		mIOHandler = pIOHandler;
		mIsDefaultHandler = false;
	}
	return;
}
// ------------------------------------------------------------------------------------------------
IOSystem* Importer::GetIOHandler()
{
	return mIOHandler;
}
// ------------------------------------------------------------------------------------------------
bool Importer::IsDefaultIOHandler()
{
	return mIsDefaultHandler;
}
// ------------------------------------------------------------------------------------------------
// Validate post process step flags 
bool ValidateFlags(unsigned int pFlags)
{
	if (pFlags & aiProcess_GenSmoothNormals &&
		pFlags & aiProcess_GenNormals)
	{
		DefaultLogger::get()->error("aiProcess_GenSmoothNormals and aiProcess_GenNormals "
			"may not be specified together");
		return false;
	}

	return true;
}
// ------------------------------------------------------------------------------------------------
// Reads the given file and returns its contents if successful. 
const aiScene* Importer::ReadFile( const std::string& pFile, unsigned int pFlags)
{
	// validate the flags
	ai_assert(ValidateFlags(pFlags));

	// check whether this Importer instance has already loaded
	// a scene. In this case we need to delete the old one
	if (this->mScene)
	{
		delete mScene;
		this->mScene = NULL;
	}

	// first check if the file is accessable at all
	if( !mIOHandler->Exists( pFile))
	{
		mErrorString = "Unable to open file \"" + pFile + "\".";
		DefaultLogger::get()->error(mErrorString);
		return NULL;
	}

	// find an worker class which can handle the file
	BaseImporter* imp = NULL;
	for( unsigned int a = 0; a < mImporter.size(); a++)
	{
		if( mImporter[a]->CanRead( pFile, mIOHandler))
		{
			imp = mImporter[a];
			break;
		}
	}

	// put a proper error message if no suitable importer was found
	if( !imp)
	{
		mErrorString = "No suitable reader found for the file format of file \"" + pFile + "\".";
		DefaultLogger::get()->error(mErrorString);
		return NULL;
	}

	// dispatch the reading to the worker class for this format
	mScene = imp->ReadFile( pFile, mIOHandler);
	
	// if successful, apply all active post processing steps to the imported data
	if( mScene)
	{
		for( unsigned int a = 0; a < mPostProcessingSteps.size(); a++)
		{
			BaseProcess* process = mPostProcessingSteps[a];
			if( process->IsActive( pFlags))
			{
				process->ExecuteOnScene( this );
			}
			if( !mScene)break; // error string has already been set ...
		}
	}

	// if failed, extract the error string
	else if( !mScene)
	{
		mErrorString = imp->GetErrorText();
	}

	// either successful or failure - the pointer expresses it anyways
	return mScene;
}

// ------------------------------------------------------------------------------------------------
//	Empty and private copy constructor
Importer::Importer(const Importer &other)
{
	// empty
}

// ------------------------------------------------------------------------------------------------
// Helper function to check whether an extension is supported by ASSIMP
bool Importer::IsExtensionSupported(const std::string& szExtension)
{
	for (std::vector<BaseImporter*>::const_iterator
		i =  this->mImporter.begin();
		i != this->mImporter.end();++i)
	{
		// pass the file extension to the CanRead(..,NULL)-method
		if ((*i)->CanRead(szExtension,NULL))return true;
	}
	return false;
}
// ------------------------------------------------------------------------------------------------
// Helper function to build a list of all file extensions supported by ASSIMP
void Importer::GetExtensionList(std::string& szOut)
{
	unsigned int iNum = 0;
	for (std::vector<BaseImporter*>::const_iterator
		i =  this->mImporter.begin();
		i != this->mImporter.end();++i,++iNum)
	{
		// insert a comma as delimiter character
		if (0 != iNum)
			szOut.append(";");

		(*i)->GetExtensionList(szOut);
	}
	return;
}

