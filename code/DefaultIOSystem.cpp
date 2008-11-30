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
/** @file Default implementation of IOSystem using the standard C file functions */

#include "AssimpPCH.h"

#include <stdlib.h>
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
	FILE* file = ::fopen( pFile.c_str(), "rb");
	if( !file)
		return false;

	::fclose( file);
	return true;
}

// ------------------------------------------------------------------------------------------------
// Open a new file with a given path.
IOStream* DefaultIOSystem::Open( const std::string& strFile, const std::string& strMode)
{
	FILE* file = ::fopen( strFile.c_str(), strMode.c_str());
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

// ------------------------------------------------------------------------------------------------
// IOSystem default implementation (ComparePaths isn't a pure virtual function)
bool IOSystem::ComparePaths (const std::string& one, 
	const std::string& second)
{
	return !ASSIMP_stricmp(one,second);
}

// this should be sufficient for all platforms :D
#define PATHLIMIT 1024

// ------------------------------------------------------------------------------------------------
// Convert a relative path into an absolute path
inline void MakeAbsolutePath (const std::string& in, char* _out)
{
	#ifdef WIN32
    ::_fullpath(_out, in.c_str(),PATHLIMIT);
  #else
    realpath(in.c_str(), _out);     //TODO not a save implementation realpath assumes that _out has the size PATH_MAX defined in limits.h; an error handling should be added to both versions
  #endif
}

// ------------------------------------------------------------------------------------------------
// DefaultIOSystem's more specialized implementation
bool DefaultIOSystem::ComparePaths (const std::string& one, 
	const std::string& second)
{
	// chances are quite good both paths are formatted identically,
	// so we can hopefully return here already
	if( !ASSIMP_stricmp(one,second) )
		return true;

	char temp1[PATHLIMIT];
	char temp2[PATHLIMIT];
	
	MakeAbsolutePath (one, temp1);
	MakeAbsolutePath (second, temp2);

	return !ASSIMP_stricmp(temp1,temp2);
}

#undef PATHLIMIT
