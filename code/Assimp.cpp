/** @file Implementation of the Plain-C API */
#include <map>

#include "../include/assimp.h"
#include "../include/assimp.hpp"

/** Stores the importer objects for all active import processes */
typedef std::map< const aiScene*, Assimp::Importer* > ImporterMap;
/** Local storage of all active import processes */
static ImporterMap gActiveImports;

/** Error message of the last failed import process */
static std::string gLastErrorString;


// ------------------------------------------------------------------------------------------------
// Reads the given file and returns its content. 
const aiScene* aiImportFile( const char* pFile, unsigned int pFlags)
{
	// create an Importer for this file
	Assimp::Importer* imp = new Assimp::Importer;
	// and have it read the file
	const aiScene* scene = imp->ReadFile( pFile, pFlags);

	// if succeeded, place it in the collection of active processes
	if( scene)
	{
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
	// find the importer associated with this data
	ImporterMap::iterator it = gActiveImports.find( pScene);
	// it should be there... else the user is playing fools with us
	if( it == gActiveImports.end())
		return;

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
