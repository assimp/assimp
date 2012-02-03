/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2008, assimp team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

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

----------------------------------------------------------------------
*/
#ifndef AI_Q3BSP_ZIPARCHIVE_H_INC
#define AI_Q3BSP_ZIPARCHIVE_H_INC

#include "../contrib/unzip/unzip.h"
#include "../include/assimp/IOStream.hpp"
#include "../include/assimp/IOSystem.hpp"
#include <string>
#include <vector>
#include <map>
#include <cassert>

namespace Assimp
{
namespace Q3BSP
{

// ------------------------------------------------------------------------------------------------
///	\class		ZipFile
///	\ingroup	Assimp::Q3BSP
///
///	\brief
// ------------------------------------------------------------------------------------------------
class ZipFile : public IOStream
{
public:
	ZipFile( const std::string &rFileName, unzFile zipFile ) :
		m_Name( rFileName ),
		m_zipFile( zipFile )
	{
		ai_assert( NULL != m_zipFile );
	}
	
	~ZipFile()
	{
		m_zipFile = NULL;
	}

	size_t Read(void* pvBuffer, size_t pSize, size_t pCount )
	{
		size_t bytes_read = 0;
		if ( NULL == m_zipFile )
			return bytes_read;
		
		// search file and place file pointer there
		if ( unzLocateFile( m_zipFile, m_Name.c_str(), 0 ) == UNZ_OK )
		{
			// get file size, etc.
			unz_file_info fileInfo;
			unzGetCurrentFileInfo( m_zipFile, &fileInfo, 0, 0, 0, 0, 0, 0 );
			const size_t size = pSize * pCount;
			assert( size <= fileInfo.uncompressed_size );
			
			// The file has EXACTLY the size of uncompressed_size. In C
			// you need to mark the last character with '\0', so add 
			// another character
			unzOpenCurrentFile( m_zipFile );
			const int ret = unzReadCurrentFile( m_zipFile, pvBuffer, fileInfo.uncompressed_size);
			size_t filesize = fileInfo.uncompressed_size;
			if ( ret < 0 || size_t(ret) != filesize )
			{
				return 0;
			}
			bytes_read = ret;
			unzCloseCurrentFile( m_zipFile );
		}
		return bytes_read;
	}

	size_t Write(const void* /*pvBuffer*/, size_t /*pSize*/, size_t /*pCount*/)
	{
		return 0;
	}

	size_t FileSize() const
	{
		if ( NULL == m_zipFile )
			return 0;
		if ( unzLocateFile( m_zipFile, m_Name.c_str(), 0 ) == UNZ_OK ) 
		{
			unz_file_info fileInfo;
			unzGetCurrentFileInfo( m_zipFile, &fileInfo, 0, 0, 0, 0, 0, 0 );
			return fileInfo.uncompressed_size;
		}
		return 0;
	}

	aiReturn Seek(size_t /*pOffset*/, aiOrigin /*pOrigin*/)
	{
		return aiReturn_FAILURE;
	}

    size_t Tell() const
	{
		return 0;
	}

	void Flush()
	{
		// empty
	}

private:
	std::string m_Name;
	unzFile m_zipFile;
};

// ------------------------------------------------------------------------------------------------
///	\class		Q3BSPZipArchive
///	\ingroup	Assimp::Q3BSP
///	
///	\brief	IMplements a zip archive like the WinZip archives. Will be also used to import data 
///	from a P3K archive ( Quake level format ).
// ------------------------------------------------------------------------------------------------
class Q3BSPZipArchive : public Assimp::IOSystem
{
public:
	static const unsigned int FileNameSize = 256;

public:
	Q3BSPZipArchive( const std::string & rFile );
	~Q3BSPZipArchive();
	bool Exists( const char* pFile) const;
	char getOsSeparator() const;
	IOStream* Open(const char* pFile, const char* pMode = "rb");
	void Close( IOStream* pFile);
	bool isOpen() const;
	void getFileList( std::vector<std::string> &rFileList );

private:
	bool mapArchive();

private:
	unzFile m_ZipFileHandle;
	std::map<std::string, IOStream*> m_ArchiveMap;
	std::vector<std::string> m_FileList;
	bool m_bDirty;
};

// ------------------------------------------------------------------------------------------------

} // Namespace Q3BSP
} // Namespace Assimp

#endif // AI_Q3BSP_ZIPARCHIVE_H_INC
