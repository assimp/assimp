/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team

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

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

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

#include "BaseImporter.h"
#include "FileSystemFilter.h"
#include "Importer.h"
#include "ByteSwapper.h"
#include "../include/assimp/scene.h"
#include "../include/assimp/Importer.hpp"
#include "../include/assimp/postprocess.h"
#include <ios>
#include <list>
#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <sstream>
#include <cctype>


using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
BaseImporter::BaseImporter()
: progress()
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
aiScene* BaseImporter::ReadFile(const Importer* pImp, const std::string& pFile, IOSystem* pIOHandler)
{
	progress = pImp->GetProgressHandler();
	ai_assert(progress);

	// Gather configuration properties for this run
	SetupProperties( pImp );

	// Construct a file system filter to improve our success ratio at reading external files
	FileSystemFilter filter(pFile,pIOHandler);

	// create a scene object to hold the data
	ScopeGuard<aiScene> sc(new aiScene());

	// dispatch importing
	try
	{
		InternReadFile( pFile, sc, &filter);

	} catch( const std::exception& err )	{
		// extract error description
		mErrorText = err.what();
		DefaultLogger::get()->error(mErrorText);
		return NULL;
	}

	// return what we gathered from the import. 
	sc.dismiss();
	return sc;
}

// ------------------------------------------------------------------------------------------------
void BaseImporter::SetupProperties(const Importer* /*pImp*/)
{
	// the default implementation does nothing
}

// ------------------------------------------------------------------------------------------------
void BaseImporter::GetExtensionList(std::set<std::string>& extensions)
{
	const aiImporterDesc* desc = GetInfo();
	ai_assert(desc != NULL);

	const char* ext = desc->mFileExtensions;
	ai_assert(ext != NULL);

	const char* last = ext;
	do {
		if (!*ext || *ext == ' ') {
			extensions.insert(std::string(last,ext-last));
			ai_assert(ext-last > 0);
			last = ext;
			while(*last == ' ') {
				++last;
			}
		}
	}
	while(*ext++);
}

// ------------------------------------------------------------------------------------------------
/*static*/ bool BaseImporter::SearchFileHeaderForToken(IOSystem* pIOHandler,
	const std::string&	pFile,
	const char**		tokens, 
	unsigned int		numTokens,
	unsigned int		searchBytes /* = 200 */,
	bool				tokensSol /* false */)
{
	ai_assert(NULL != tokens && 0 != numTokens && 0 != searchBytes);
	if (!pIOHandler)
		return false;

	boost::scoped_ptr<IOStream> pStream (pIOHandler->Open(pFile));
	if (pStream.get() )	{
		// read 200 characters from the file
		boost::scoped_array<char> _buffer (new char[searchBytes+1 /* for the '\0' */]);
		char* buffer = _buffer.get();

		const unsigned int read = pStream->Read(buffer,1,searchBytes);
		if (!read)
			return false;

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


			const char* r = strstr(buffer,tokens[i]);
			if (!r) 
				continue;
			// We got a match, either we don't care where it is, or it happens to
			// be in the beginning of the file / line
			if (!tokensSol || r == buffer || r[-1] == '\r' || r[-1] == '\n') {
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

	if (!pIOHandler) {
		return false;
	}
	union {
		const char* magic;
		const uint16_t* magic_u16;
		const uint32_t* magic_u32;
	};
	magic = reinterpret_cast<const char*>(_magic);
	boost::scoped_ptr<IOStream> pStream (pIOHandler->Open(pFile));
	if (pStream.get() )	{

		// skip to offset
		pStream->Seek(offset,aiOrigin_SET);

		// read 'size' characters from the file
		union {
			char data[16];
			uint16_t data_u16[8];
			uint32_t data_u32[4];
		};
		if(size != pStream->Read(data,1,size)) {
			return false;
		}

		for (unsigned int i = 0; i < num; ++i) {
			// also check against big endian versions of tokens with size 2,4
			// that's just for convinience, the chance that we cause conflicts
			// is quite low and it can save some lines and prevent nasty bugs
			if (2 == size) {
				uint16_t rev = *magic_u16; 
				ByteSwap::Swap(&rev);
				if (data_u16[0] == *magic_u16 || data_u16[0] == rev) {
					return true;
				}
			}
			else if (4 == size) {
				uint32_t rev = *magic_u32;
				ByteSwap::Swap(&rev);
				if (data_u32[0] == *magic_u32 || data_u32[0] == rev) {
					return true;
				}
			}
			else {
				// any length ... just compare
				if(!memcmp(magic,data,size)) {
					return true;
				}
			}
			magic += size;
		}
	}
	return false;
}

#include "../contrib/ConvertUTF/ConvertUTF.h"

// ------------------------------------------------------------------------------------------------
void ReportResult(ConversionResult res)
{
	if(res == sourceExhausted) {
		DefaultLogger::get()->error("Source ends with incomplete character sequence, transformation to UTF-8 fails");
	}
	else if(res == sourceIllegal) {
		DefaultLogger::get()->error("Source contains illegal character sequence, transformation to UTF-8 fails");
	}
}

// ------------------------------------------------------------------------------------------------
// Convert to UTF8 data
void BaseImporter::ConvertToUTF8(std::vector<char>& data)
{
	ConversionResult result;
	if(data.size() < 8) {
		throw DeadlyImportError("File is too small");
	}

	// UTF 8 with BOM
	if((uint8_t)data[0] == 0xEF && (uint8_t)data[1] == 0xBB && (uint8_t)data[2] == 0xBF) {
		DefaultLogger::get()->debug("Found UTF-8 BOM ...");

		std::copy(data.begin()+3,data.end(),data.begin());
		data.resize(data.size()-3);
		return;
	}

	// UTF 32 BE with BOM
	if(*((uint32_t*)&data.front()) == 0xFFFE0000) {
	
		// swap the endianess ..
		for(uint32_t* p = (uint32_t*)&data.front(), *end = (uint32_t*)&data.back(); p <= end; ++p) {
			AI_SWAP4P(p);
		}
	}
	
	// UTF 32 LE with BOM
	if(*((uint32_t*)&data.front()) == 0x0000FFFE) {
		DefaultLogger::get()->debug("Found UTF-32 BOM ...");

		const uint32_t* sstart = (uint32_t*)&data.front()+1, *send = (uint32_t*)&data.back()+1;
		char* dstart,*dend;
		std::vector<char> output;
		do {
			output.resize(output.size()?output.size()*3/2:data.size()/2);
			dstart = &output.front(),dend = &output.back()+1;

			result = ConvertUTF32toUTF8((const UTF32**)&sstart,(const UTF32*)send,(UTF8**)&dstart,(UTF8*)dend,lenientConversion);
		} while(result == targetExhausted);

		ReportResult(result);

		// copy to output buffer. 
		const size_t outlen = (size_t)(dstart-&output.front());
		data.assign(output.begin(),output.begin()+outlen);
		return;
	}

	// UTF 16 BE with BOM
	if(*((uint16_t*)&data.front()) == 0xFFFE) {
	
		// swap the endianess ..
		for(uint16_t* p = (uint16_t*)&data.front(), *end = (uint16_t*)&data.back(); p <= end; ++p) {
			ByteSwap::Swap2(p);
		}
	}
	
	// UTF 16 LE with BOM
	if(*((uint16_t*)&data.front()) == 0xFEFF) {
		DefaultLogger::get()->debug("Found UTF-16 BOM ...");

		const uint16_t* sstart = (uint16_t*)&data.front()+1, *send = (uint16_t*)(&data.back()+1);
		char* dstart,*dend;
		std::vector<char> output;
		do {
			output.resize(output.size()?output.size()*3/2:data.size()*3/4);
			dstart = &output.front(),dend = &output.back()+1;

			result = ConvertUTF16toUTF8((const UTF16**)&sstart,(const UTF16*)send,(UTF8**)&dstart,(UTF8*)dend,lenientConversion);
		} while(result == targetExhausted);

		ReportResult(result);

		// copy to output buffer.
		const size_t outlen = (size_t)(dstart-&output.front());
		data.assign(output.begin(),output.begin()+outlen);
		return;
	}
}

// ------------------------------------------------------------------------------------------------
// Convert to UTF8 data to ISO-8859-1
void BaseImporter::ConvertUTF8toISO8859_1(std::string& data)
{
	unsigned int size = data.size();
	unsigned int i = 0, j = 0;

	while(i < size) {
		if((unsigned char) data[i] < 0x80) {
			data[j] = data[i];
		} else if(i < size - 1) {
			if((unsigned char) data[i] == 0xC2) {
				data[j] = data[++i];
			} else if((unsigned char) data[i] == 0xC3) {
				data[j] = ((unsigned char) data[++i] + 0x40);
			} else {
				std::stringstream stream;

				stream << "UTF8 code " << std::hex << data[i] << data[i + 1] << " can not be converted into ISA-8859-1.";

				DefaultLogger::get()->error(stream.str());

				data[j++] = data[i++];
				data[j] = data[i];
			}
		} else {
			DefaultLogger::get()->error("UTF8 code but only one character remaining");

			data[j] = data[i];
		}

		i++; j++;
	}

	data.resize(j);
}

// ------------------------------------------------------------------------------------------------
void BaseImporter::TextFileToBuffer(IOStream* stream,
	std::vector<char>& data)
{
	ai_assert(NULL != stream);

	const size_t fileSize = stream->FileSize();
	if(!fileSize) {
		throw DeadlyImportError("File is empty");
	}

	data.reserve(fileSize+1); 
	data.resize(fileSize); 
	if(fileSize != stream->Read( &data[0], 1, fileSize)) {
		throw DeadlyImportError("File read error");
	}

	ConvertToUTF8(data);

	// append a binary zero to simplify string parsing
	data.push_back(0);
}

// ------------------------------------------------------------------------------------------------
namespace Assimp
{
	// Represents an import request
	struct LoadRequest
	{
		LoadRequest(const std::string& _file, unsigned int _flags,const BatchLoader::PropertyMap* _map, unsigned int _id)
			: file(_file), flags(_flags), refCnt(1),scene(NULL), loaded(false), id(_id)
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
}

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
#ifdef ASSIMP_BUILD_DEBUG
		pp |= aiProcess_ValidateDataStructure;
#endif
		// setup config properties if necessary
		ImporterPimpl* pimpl = data->pImporter->Pimpl();
		pimpl->mFloatProperties  = (*it).map.floats;
		pimpl->mIntProperties    = (*it).map.ints;
		pimpl->mStringProperties = (*it).map.strings;
		pimpl->mMatrixProperties = (*it).map.matrices;

		if (!DefaultLogger::isNullLogger())
		{
			DefaultLogger::get()->info("%%% BEGIN EXTERNAL FILE %%%");
			DefaultLogger::get()->info("File: " + (*it).file);
		}
		data->pImporter->ReadFile((*it).file,pp);
		(*it).scene = data->pImporter->GetOrphanedScene();
		(*it).loaded = true;

		DefaultLogger::get()->info("%%% END EXTERNAL FILE %%%");
	}
}




