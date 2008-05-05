/** @file Implementation of the few default functions of the base importer class */
#include "BaseImporter.h"
#include "../include/aiScene.h"
#include "aiAssert.h"
using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
BaseImporter::BaseImporter()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
BaseImporter::~BaseImporter()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Imports the given file and returns the imported data.
aiScene* BaseImporter::ReadFile( const std::string& pFile, IOSystem* pIOHandler)
{
	// create a scene object to hold the data
	aiScene* scene = new aiScene;

	// dispatch importing
	try
	{
		InternReadFile( pFile, scene, pIOHandler);
	} catch( ImportErrorException* exception)
	{
		// extract error description
		mErrorText = exception->GetErrorText();
		delete exception;

		// and kill the partially imported data
		delete scene;
		scene = NULL;
	}

	// return what we gathered from the import. 
	return scene;
}
