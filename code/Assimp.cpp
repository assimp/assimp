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
/** @file Implementation of the Plain-C API */

// CRT headers
#include <map>

// public ASSIMP headers
#include "../include/assimp.h"
#include "../include/assimp.hpp"
#include "../include/DefaultLogger.h"
#include "../include/aiAssert.h"

// boost headers
#define AI_C_THREADSAFE
#if (defined AI_C_THREADSAFE)
#	include <boost/thread/thread.hpp>
#	include <boost/thread/mutex.hpp>
#endif

using namespace Assimp;

/** Stores the importer objects for all active import processes */
typedef std::map< const aiScene*, Assimp::Importer* > ImporterMap;

/** Local storage of all active import processes */
static ImporterMap gActiveImports;

/** Error message of the last failed import process */
static std::string gLastErrorString;

#if (defined AI_C_THREADSAFE)
/** Global mutex to manage the access to the importer map */
static boost::mutex gMutex;
#endif

// ------------------------------------------------------------------------------------------------
// Reads the given file and returns its content. 
const aiScene* aiImportFile( const char* pFile, unsigned int pFlags)
{
	ai_assert(NULL != pFile);

	// create an Importer for this file
	Assimp::Importer* imp = new Assimp::Importer;
	// and have it read the file
	const aiScene* scene = imp->ReadFile( pFile, pFlags);

	// if succeeded, place it in the collection of active processes
	if( scene)
	{
#if (defined AI_C_THREADSAFE)
		boost::mutex::scoped_lock lock(gMutex);
#endif
		gActiveImports[scene] = imp;
	} 
	else
	{
		// if failed, extract error code and destroy the import
		gLastErrorString = imp->GetErrorString();
		delete imp;
	}

	// return imported data. If the import failed the pointer is NULL anyways
	return scene;
}

// ------------------------------------------------------------------------------------------------
// Releases all resources associated with the given import process. 
void aiReleaseImport( const aiScene* pScene)
{
	if (!pScene)return;

	// lock the mutex
#if (defined AI_C_THREADSAFE)
	boost::mutex::scoped_lock lock(gMutex);
#endif

	// find the importer associated with this data
	ImporterMap::iterator it = gActiveImports.find( pScene);
	// it should be there... else the user is playing fools with us
	if( it == gActiveImports.end())
	{
		DefaultLogger::get()->error("Unable to find the Importer instance for this scene. "
			"Are you sure it has been created by aiImportFile(ex)(...)?");
		return;
	}

	// kill the importer, the data dies with it
	delete it->second;
	gActiveImports.erase( it);
}

// ------------------------------------------------------------------------------------------------
// Returns the error text of the last failed import process. 
const char* aiGetErrorString()
{
	return gLastErrorString.c_str();
}
// ------------------------------------------------------------------------------------------------
// Returns the error text of the last failed import process. 
int aiIsExtensionSupported(const char* szExtension)
{
	ai_assert(NULL != szExtension);

	// lock the mutex
#if (defined AI_C_THREADSAFE)
	boost::mutex::scoped_lock lock(gMutex);
#endif

	if (!gActiveImports.empty())
	{
		return (int)((*(gActiveImports.begin())).second->IsExtensionSupported(
			std::string ( szExtension )));
	}
	// need to create a temporary Importer instance.
	// TODO: Find a better solution ...
	Assimp::Importer* pcTemp = new Assimp::Importer();
	int i = (int)pcTemp->IsExtensionSupported(std::string ( szExtension ));
	delete pcTemp;
	return i;
}
// ------------------------------------------------------------------------------------------------
// Get a list of all file extensions supported by ASSIMP
void aiGetExtensionList(aiString* szOut)
{
	ai_assert(NULL != szOut);

	// lock the mutex
#if (defined AI_C_THREADSAFE)
	boost::mutex::scoped_lock lock(gMutex);
#endif

	std::string szTemp;
	if (!gActiveImports.empty())
	{
		(*(gActiveImports.begin())).second->GetExtensionList(szTemp);
		szOut->Set ( szTemp );
		return;
	}
	// need to create a temporary Importer instance.
	// TODO: Find a better solution ...
	Assimp::Importer* pcTemp = new Assimp::Importer();
	pcTemp->GetExtensionList(szTemp);
	szOut->Set ( szTemp );
	delete pcTemp;
}
// ------------------------------------------------------------------------------------------------
void aiGetMemoryRequirements(const C_STRUCT aiScene* pIn,
	C_STRUCT aiMemoryInfo* in);
{
// lock the mutex
#if (defined AI_C_THREADSAFE)
	boost::mutex::scoped_lock lock(gMutex);
#endif

	// find the importer associated with this data
	ImporterMap::iterator it = gActiveImports.find( pIn);
	// it should be there... else the user is playing fools with us
	if( it == gActiveImports.end())
	{
		DefaultLogger::get()->error("Unable to find the Importer instance for this scene. "
			"Are you sure it has been created by aiImportFile(ex)(...)?");
		return 0;
	}
	// get memory statistics
	it->second->GetMemoryRequirements(*in);
}

