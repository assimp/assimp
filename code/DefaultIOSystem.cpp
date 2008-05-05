/** @file Default implementation of IOSystem using the standard C file functions */
#include <stdlib.h>
#include <string>

#include "DefaultIOSystem.h"
#include "DefaultIOStream.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor. 
DefaultIOSystem::DefaultIOSystem()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor. 
DefaultIOSystem::~DefaultIOSystem()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Tests for the existence of a file at the given path.
bool DefaultIOSystem::Exists( const std::string& pFile) const
{
	FILE* file = fopen( pFile.c_str(), "rb");
	if( !file)
		return false;

	fclose( file);
	return true;
}

// ------------------------------------------------------------------------------------------------
// Open a new file with a given path.
IOStream* DefaultIOSystem::Open( const std::string& strFile, const std::string& strMode)
{
	FILE* file = fopen( strFile.c_str(), strMode.c_str());
	if( NULL == file) 
		return NULL;

	return new DefaultIOStream( file, strFile);
}

// ------------------------------------------------------------------------------------------------
// Closes the given file and releases all resources associated with it.
void DefaultIOSystem::Close( IOStream* pFile)
{
	delete pFile;
}

// ------------------------------------------------------------------------------------------------
// Returns the operation specific directory separator
std::string DefaultIOSystem::getOsSeparator() const
{
#ifndef _WIN32
	std::string sep = "/";
#else
	std::string sep = "\\";
#endif
	return sep;
}
