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

#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_Q3BSP_IMPORTER

#include "Q3BSPZipArchive.h"
#include <algorithm>
#include <cassert>

namespace Assimp
{
namespace Q3BSP
{

// ------------------------------------------------------------------------------------------------
//	Constructor.
Q3BSPZipArchive::Q3BSPZipArchive( const std::string& rFile ) :
	m_ZipFileHandle( NULL ),
	m_FileList(),
	m_bDirty( true )
{
	if ( !rFile.empty() )
	{
		m_ZipFileHandle = unzOpen( rFile.c_str() );
		if ( NULL != m_ZipFileHandle )
		{
			mapArchive();
		}
	}
}

// ------------------------------------------------------------------------------------------------
//	Destructor.
Q3BSPZipArchive::~Q3BSPZipArchive()
{
	if ( NULL != m_ZipFileHandle )
	{
		unzClose( m_ZipFileHandle );
	}
	m_ZipFileHandle = NULL;
	m_FileList.clear();
}

// ------------------------------------------------------------------------------------------------
//	Returns true, if the archive is already open.
bool Q3BSPZipArchive::isOpen() const
{
	return ( NULL != m_ZipFileHandle );
}

// ------------------------------------------------------------------------------------------------
//	Returns true, if the filename is part of the archive.
bool Q3BSPZipArchive::Exists( const char* pFile ) const
{
	ai_assert( NULL != pFile );
	if ( NULL == pFile )
	{
		return false;
	}

	std::string rFile( pFile );
	std::vector<std::string>::const_iterator it = std::find( m_FileList.begin(), m_FileList.end(), rFile );
	if ( m_FileList.end() == it )
	{
		return false;
	}

	return true;
}

// ------------------------------------------------------------------------------------------------
//	Returns the separator delimiter.
char Q3BSPZipArchive::getOsSeparator() const
{
	return '/';
}

// ------------------------------------------------------------------------------------------------
//	Opens a file, which is part of the archive.
IOStream *Q3BSPZipArchive::Open( const char* pFile, const char* /*pMode*/ )
{
	ai_assert( NULL != pFile );

	std::string rItem( pFile );
	std::vector<std::string>::iterator it = std::find( m_FileList.begin(), m_FileList.end(), rItem );
	if ( m_FileList.end() == it )
		return NULL;

	ZipFile *pZipFile = new ZipFile( *it, m_ZipFileHandle );
	m_ArchiveMap[ rItem ] = pZipFile;

	return pZipFile;
}

// ------------------------------------------------------------------------------------------------
//	Close a filestream.
void Q3BSPZipArchive::Close( IOStream *pFile )
{
	ai_assert( NULL != pFile );

	std::map<std::string, IOStream*>::iterator it;
	for ( it = m_ArchiveMap.begin(); it != m_ArchiveMap.end(); ++it )
	{
		if ( (*it).second == pFile )
		{
			ZipFile *pZipFile = reinterpret_cast<ZipFile*>( (*it).second ); 
			delete pZipFile;
			m_ArchiveMap.erase( it );
			break;
		}
	}
}
// ------------------------------------------------------------------------------------------------
//	Returns the file-list of the archive.
void Q3BSPZipArchive::getFileList( std::vector<std::string> &rFileList )
{
	rFileList = m_FileList;
}

// ------------------------------------------------------------------------------------------------
//	Maps the archive content.
bool Q3BSPZipArchive::mapArchive()
{
	if ( NULL == m_ZipFileHandle )
		return false;

	if ( !m_bDirty )
		return true;

	if ( !m_FileList.empty() )
		m_FileList.resize( 0 );

	//	At first ensure file is already open
	if ( UNZ_OK == unzGoToFirstFile( m_ZipFileHandle ) ) 
	{
		char filename[ FileNameSize ];
		unzGetCurrentFileInfo( m_ZipFileHandle, NULL, filename, FileNameSize, NULL, 0, NULL, 0 );
		m_FileList.push_back( filename );
		unzCloseCurrentFile( m_ZipFileHandle );
			
		// Loop over all files
		while ( unzGoToNextFile( m_ZipFileHandle ) != UNZ_END_OF_LIST_OF_FILE )  
		{
			char filename[ FileNameSize ];
			unzGetCurrentFileInfo( m_ZipFileHandle, NULL, filename, FileNameSize, NULL, 0, NULL, 0 );
			m_FileList.push_back( filename );
			unzCloseCurrentFile( m_ZipFileHandle );
		}
	}
	
	std::sort( m_FileList.begin(), m_FileList.end() );
	m_bDirty = false;

	return true;
}

// ------------------------------------------------------------------------------------------------

} // Namespace Q3BSP
} // Namespace Assimp

#endif // ASSIMP_BUILD_NO_Q3BSP_IMPORTER
