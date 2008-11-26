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

/** @file Implementation of BaseImporter */

#include "AssimpPCH.h"
#include "BaseImporter.h"


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
	aiScene* scene = new aiScene();

	// dispatch importing
	try
	{
		InternReadFile( pFile, scene, pIOHandler);
	} catch( ImportErrorException* exception)
	{
		// extract error description
		mErrorText = exception->GetErrorText();

		DefaultLogger::get()->error(mErrorText);

		delete exception;

		// and kill the partially imported data
		delete scene;
		scene = NULL;
	}

	// return what we gathered from the import. 
	return scene;
}

// ------------------------------------------------------------------------------------------------
void BaseImporter::SetupProperties(const Importer* pImp)
{
	// the default implementation does nothing
}

// ------------------------------------------------------------------------------------------------
bool BaseImporter::SearchFileHeaderForToken(IOSystem* pIOHandler,
	const std::string&	pFile,
	const char**		tokens, 
	unsigned int		numTokens,
	unsigned int		searchBytes /* = 200 */)
{
	ai_assert(NULL != tokens && 0 != numTokens && NULL != pIOHandler && 0 != searchBytes);

	boost::scoped_ptr<IOStream> pStream (pIOHandler->Open(pFile));
	if (pStream.get() )
	{
		// read 200 characters from the file
		boost::scoped_array<char> _buffer (new char[searchBytes+1 /* for the '\0' */]);
		char* buffer = _buffer.get();

		unsigned int read = (unsigned int)pStream->Read(buffer,1,searchBytes);
		if (!read)return false;

		for (unsigned int i = 0; i < read;++i)
			buffer[i] = ::tolower(buffer[i]);

		// It is not a proper handling of unicode files here ...
		// ehm ... but it works in most cases.
		char* cur = buffer,*cur2 = buffer,*end = &buffer[read];
		while (cur != end)
		{
			if (*cur)*cur2++ = *cur;
			++cur;
		}
		*cur2 = '\0';

		for (unsigned int i = 0; i < numTokens;++i)
		{
			ai_assert(NULL != tokens[i]);
			if (::strstr(buffer,tokens[i]))return true;
		}
	}
	return false;
}


// ------------------------------------------------------------------------------------------------
// Represents an import request
struct LoadRequest
{
	LoadRequest(const std::string& _file, unsigned int _flags,const BatchLoader::PropertyMap* _map)
		:	file	(_file)
		,	flags	(_flags)
		,	scene	(NULL)
		,	refCnt	(1)
		,	loaded	(false)
		,	map		(*_map)
	{}

	const std::string file;
	unsigned int flags;
	unsigned int refCnt;
	aiScene* scene;
	bool loaded;
	BatchLoader::PropertyMap map;

	bool operator== (const std::string& f)
		{return file == f;}
};

// ------------------------------------------------------------------------------------------------
// BatchLoader::pimpl data structure
struct BatchData
{
	// IO system to be used for all imports
	IOSystem* pIOSystem;

	// Importer used to load all meshes
	Importer* pImporter;

	// List of all imports
	std::list<LoadRequest> requests;

	// Base path
	std::string pathBase;
};

// ------------------------------------------------------------------------------------------------
BatchLoader::BatchLoader(IOSystem* pIO)
{
	ai_assert(NULL != pIO);

	pimpl = new BatchData();
	BatchData* data = ( BatchData* )pimpl;
	data->pIOSystem = pIO;
	data->pImporter = new Importer();
}

// ------------------------------------------------------------------------------------------------
BatchLoader::~BatchLoader()
{
	// delete all scenes wthat have not been polled by the user
	BatchData* data = ( BatchData* )pimpl;
	for (std::list<LoadRequest>::iterator it = data->requests.begin();
		it != data->requests.end(); ++it)
	{
		delete (*it).scene;
	}
	delete data->pImporter;
	delete data;
}

// ------------------------------------------------------------------------------------------------
void BatchLoader::SetBasePath (const std::string& pBase)
{
	BatchData* data = ( BatchData* )pimpl;
	data->pathBase = pBase;

	// file name? we just need the directory
	std::string::size_type ss,ss2;
	if (std::string::npos != (ss = data->pathBase.find_first_of('.')))
	{
		if (std::string::npos != (ss2 = data->pathBase.find_last_of('\\')) ||
			std::string::npos != (ss2 = data->pathBase.find_last_of('/')))
		{
			if (ss > ss2)
				data->pathBase.erase(ss2,data->pathBase.length()-ss2);
		}
		else return;
	}

	// make sure the directory is terminated properly
	char s;
	if ((s = *(data->pathBase.end()-1)) != '\\' && s != '/')
		data->pathBase.append("\\");
}

// ------------------------------------------------------------------------------------------------
void BatchLoader::AddLoadRequest	(const std::string& file,
	unsigned int steps /*= 0*/, const PropertyMap* map /*= NULL*/)
{
	ai_assert(!file.empty());

	// no threaded implementation for the moment
	BatchData* data = ( BatchData* )pimpl;

	std::string real;

	// build a full path if this is a relative path and 
	// we have a new base directory given
	if (file.length() > 2 && file[1] != ':' && data->pathBase.length())
	{
		real = data->pathBase + file;
	}
	else real = file;
	
	// check whether we have this loading request already
	std::list<LoadRequest>::iterator it;
	for (it = data->requests.begin();it != data->requests.end(); ++it)
	{
		// Call IOSystem's path comparison function here
		if (data->pIOSystem->ComparePaths((*it).file,real))
		{
			(*it).refCnt++;
			return;
		}
	}

	// no, we don't have it. So add it to the queue ...
	data->requests.push_back(LoadRequest(real,steps,map));
}

// ------------------------------------------------------------------------------------------------
aiScene* BatchLoader::GetImport		(const std::string& file)
{
	// no threaded implementation for the moment
	BatchData* data = ( BatchData* )pimpl;
	std::string real;

	// build a full path if this is a relative path and 
	// we have a new base directory given
	if (file.length() > 2 && file[1] != ':' && data->pathBase.length())
	{
		real = data->pathBase + file;
	}
	else real = file;
	for (std::list<LoadRequest>::iterator it = data->requests.begin();it != data->requests.end(); ++it)
	{
		// Call IOSystem's path comparison function here
		if (data->pIOSystem->ComparePaths((*it).file,real) && (*it).loaded)
		{
			aiScene* sc = (*it).scene;
			if (!(--(*it).refCnt))
			{
				data->requests.erase(it);
			}
			return sc;
		}
	}
	return NULL;
}

// ------------------------------------------------------------------------------------------------
void BatchLoader::LoadAll()
{
	BatchData* data = ( BatchData* )pimpl;

	// no threaded implementation for the moment
	for (std::list<LoadRequest>::iterator it = data->requests.begin();
		it != data->requests.end(); ++it)
	{
		// force validation in debug builds
		unsigned int pp = (*it).flags;
#ifdef _DEBUG
		pp |= aiProcess_ValidateDataStructure;
#endif
		// setup config properties if necessary
		data->pImporter->mFloatProperties  = (*it).map.floats;
		data->pImporter->mIntProperties    = (*it).map.ints;
		data->pImporter->mStringProperties = (*it).map.strings;

		if (!DefaultLogger::isNullLogger())
		{
			DefaultLogger::get()->info("%%% BEGIN EXTERNAL FILE %%%");
			DefaultLogger::get()->info("File: " + (*it).file);
		}
		data->pImporter->ReadFile((*it).file,pp);
		(*it).scene = const_cast<aiScene*>(data->pImporter->GetOrphanedScene());
		(*it).loaded = true;

		DefaultLogger::get()->info("%%% END EXTERNAL FILE %%%");
	}
}




