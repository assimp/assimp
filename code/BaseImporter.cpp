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

/** @file  BaseImporter.cpp
 *  @brief Implementation of BaseImporter 
 */

#include "AssimpPCH.h"
#include "BaseImporter.h"
#include "FileSystemFilter.h"

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
	// Construct a file system filter to improve our success ratio reading external files
	FileSystemFilter filter(pFile,pIOHandler);

	// create a scene object to hold the data
	aiScene* scene = new aiScene();

	// dispatch importing
	try
	{
		InternReadFile( pFile, scene, &filter);
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
/*static*/ bool BaseImporter::SearchFileHeaderForToken(IOSystem* pIOHandler,
	const std::string&	pFile,
	const char**		tokens, 
	unsigned int		numTokens,
	unsigned int		searchBytes /* = 200 */)
{
	ai_assert(NULL != tokens && 0 != numTokens && 0 != searchBytes);
	if (!pIOHandler)
		return false;

	boost::scoped_ptr<IOStream> pStream (pIOHandler->Open(pFile));
	if (pStream.get() )	{
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
		while (cur != end)	{
			if (*cur)
				*cur2++ = *cur;
			++cur;
		}
		*cur2 = '\0';

		for (unsigned int i = 0; i < numTokens;++i)	{
			ai_assert(NULL != tokens[i]);

			if (::strstr(buffer,tokens[i]))	{
				DefaultLogger::get()->debug(std::string("Found positive match for header keyword: ") + tokens[i]);
				return true;
			}
		}
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
// Simple check for file extension
/*static*/ bool BaseImporter::SimpleExtensionCheck (const std::string& pFile, 
	const char* ext0,
	const char* ext1,
	const char* ext2)
{
	std::string::size_type pos = pFile.find_last_of('.');

	// no file extension - can't read
	if( pos == std::string::npos)
		return false;
	
	const char* ext_real = & pFile[ pos+1 ];
	if( !ASSIMP_stricmp(ext_real,ext0) )
		return true;

	// check for other, optional, file extensions
	if (ext1 && !ASSIMP_stricmp(ext_real,ext1))
		return true;

	if (ext2 && !ASSIMP_stricmp(ext_real,ext2))
		return true;

	return false;
}

// ------------------------------------------------------------------------------------------------
// Get file extension from path
/*static*/ std::string BaseImporter::GetExtension (const std::string& pFile)
{
	std::string::size_type pos = pFile.find_last_of('.');

	// no file extension at all
	if( pos == std::string::npos)
		return "";

	std::string ret = pFile.substr(pos+1);
	std::transform(ret.begin(),ret.end(),ret.begin(),::tolower); // thanks to Andy Maloney for the hint
	return ret;
}

// ------------------------------------------------------------------------------------------------
// Check for magic bytes at the beginning of the file.
/* static */ bool BaseImporter::CheckMagicToken(IOSystem* pIOHandler, const std::string& pFile, 
	const void* _magic, unsigned int num, unsigned int offset, unsigned int size)
{
	ai_assert(size <= 16 && _magic);

	if (!pIOHandler)
		return false;

	const char* magic = (const char*)_magic;
	boost::scoped_ptr<IOStream> pStream (pIOHandler->Open(pFile));
	if (pStream.get() )	{

		// skip to offset
		pStream->Seek(offset,aiOrigin_SET);

		// read 'size' characters from the file
		char data[16];
		if(size != pStream->Read(data,1,size))
			return false;

		for (unsigned int i = 0; i < num; ++i) {
			// also check against big endian versions of tokens with size 2,4
			// that's just for convinience, the chance that we cause conflicts
			// is quite low and it can save some lines and prevent nasty bugs
			if (2 == size) {
				int16_t rev = *((int16_t*)magic);
				ByteSwap::Swap(&rev);
				if (*((int16_t*)data) == ((int16_t*)magic)[i] || *((int16_t*)data) == rev)
					return true;
			}
			else if (4 == size) {
				int32_t rev = *((int32_t*)magic);
				ByteSwap::Swap(&rev);
				if (*((int32_t*)data) == ((int32_t*)magic)[i] || *((int32_t*)data) == rev)
					return true;
			}
			else {
				// any length ... just compare
				if(!::memcmp(magic,data,size))
					return true;
			}
			magic += size;
		}
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
// Represents an import request
struct LoadRequest
{
	LoadRequest(const std::string& _file, unsigned int _flags,const BatchLoader::PropertyMap* _map, unsigned int _id)
		:	file	(_file)
		,	flags	(_flags)
		,	refCnt	(1)
		,	scene	(NULL)            
		,	loaded	(false)
		,	id		(_id)
	{
		if (_map)
			map = *_map;
	}

	const std::string file;
	unsigned int flags;
	unsigned int refCnt;
	aiScene* scene;
	bool loaded;
	BatchLoader::PropertyMap map;
	unsigned int id;

	bool operator== (const std::string& f) {
		return file == f;
	}
};

// ------------------------------------------------------------------------------------------------
// BatchLoader::pimpl data structure
struct Assimp::BatchData
{
	BatchData()
		:	next_id(0xffff)
	{}

	// IO system to be used for all imports
	IOSystem* pIOSystem;

	// Importer used to load all meshes
	Importer* pImporter;

	// List of all imports
	std::list<LoadRequest> requests;

	// Base path
	std::string pathBase;

	// Id for next item
	unsigned int next_id;
};

// ------------------------------------------------------------------------------------------------
BatchLoader::BatchLoader(IOSystem* pIO)
{
	ai_assert(NULL != pIO);

	data = new BatchData();
	data->pIOSystem = pIO;

	data->pImporter = new Importer();
	data->pImporter->SetIOHandler(data->pIOSystem);
}

// ------------------------------------------------------------------------------------------------
BatchLoader::~BatchLoader()
{
	// delete all scenes wthat have not been polled by the user
	for (std::list<LoadRequest>::iterator it = data->requests.begin();it != data->requests.end(); ++it)	{

		delete (*it).scene;
	}
	data->pImporter->SetIOHandler(NULL); /* get pointer back into our posession */
	delete data->pImporter;
	delete data;
}


// ------------------------------------------------------------------------------------------------
unsigned int BatchLoader::AddLoadRequest	(const std::string& file,
	unsigned int steps /*= 0*/, const PropertyMap* map /*= NULL*/)
{
	ai_assert(!file.empty());
	
	// check whether we have this loading request already
	std::list<LoadRequest>::iterator it;
	for (it = data->requests.begin();it != data->requests.end(); ++it)	{

		// Call IOSystem's path comparison function here
		if (data->pIOSystem->ComparePaths((*it).file,file))	{

			if (map) {
				if (!((*it).map == *map))
					continue;
			}
			else if (!(*it).map.empty())
				continue;

			(*it).refCnt++;
			return (*it).id;
		}
	}

	// no, we don't have it. So add it to the queue ...
	data->requests.push_back(LoadRequest(file,steps,map,data->next_id));
	return data->next_id++;
}

// ------------------------------------------------------------------------------------------------
aiScene* BatchLoader::GetImport		(unsigned int which)
{
	for (std::list<LoadRequest>::iterator it = data->requests.begin();it != data->requests.end(); ++it)	{

		if ((*it).id == which && (*it).loaded)	{

			aiScene* sc = (*it).scene;
			if (!(--(*it).refCnt))	{
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
	// no threaded implementation for the moment
	for (std::list<LoadRequest>::iterator it = data->requests.begin();it != data->requests.end(); ++it)	{
		// force validation in debug builds
		unsigned int pp = (*it).flags;
#ifdef _DEBUG
		pp |= aiProcess_ValidateDataStructure;
#endif
		// setup config properties if necessary
		data->pImporter->pimpl->mFloatProperties  = (*it).map.floats;
		data->pImporter->pimpl->mIntProperties    = (*it).map.ints;
		data->pImporter->pimpl->mStringProperties = (*it).map.strings;

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




