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

namespace Assimp {
namespace Q3BSP {

ZipFile::ZipFile(const std::string &rFileName, unzFile zipFile) : m_Name(rFileName), m_zipFile(zipFile) {
	ai_assert(m_zipFile != NULL);
}
	
ZipFile::~ZipFile() {
	m_zipFile = NULL;
}

size_t ZipFile::Read(void* pvBuffer, size_t pSize, size_t pCount ) {
	size_t bytes_read = 0;

	DefaultLogger::get()->warn("file: \"" + m_Name + "\".");

	if(m_zipFile != NULL) {
		DefaultLogger::get()->warn("file: zip exist.");

		// search file and place file pointer there
		if(unzLocateFile(m_zipFile, m_Name.c_str(), 0) == UNZ_OK) {
			DefaultLogger::get()->warn("file: file located in the zip archive.");

			// get file size, etc.
			unz_file_info fileInfo;
			if(unzGetCurrentFileInfo(m_zipFile, &fileInfo, 0, 0, 0, 0, 0, 0) == UNZ_OK) {
				const size_t size = pSize * pCount;
				assert(size <= fileInfo.uncompressed_size);

				std::stringstream size_str;
				size_str << fileInfo.uncompressed_size;
				DefaultLogger::get()->warn("file: size = " + size_str.str() + ".");
			
				// The file has EXACTLY the size of uncompressed_size. In C
				// you need to mark the last character with '\0', so add 
				// another character
				if(unzOpenCurrentFile(m_zipFile) == UNZ_OK) {
					DefaultLogger::get()->warn("file: file opened.");

					if(unzReadCurrentFile(m_zipFile, pvBuffer, fileInfo.uncompressed_size) == (long int) fileInfo.uncompressed_size) {
						std::string file((char*) pvBuffer, ((char*) pvBuffer) + (fileInfo.uncompressed_size < 1000 ? fileInfo.uncompressed_size : 1000));
						DefaultLogger::get()->warn("file: data = \"" + file + "\".");

						if(unzCloseCurrentFile(m_zipFile) == UNZ_OK) {
							DefaultLogger::get()->warn("file: file closed.");

							bytes_read = fileInfo.uncompressed_size;
						}
					}
				}
			}
		}
	}

	return bytes_read;
}

size_t ZipFile::Write(const void* /*pvBuffer*/, size_t /*pSize*/, size_t /*pCount*/) {
	return 0;
}

size_t ZipFile::FileSize() const {
	size_t size = 0;

	if(m_zipFile != NULL) {
		if(unzLocateFile(m_zipFile, m_Name.c_str(), 0) == UNZ_OK) {
			unz_file_info fileInfo;
			if(unzGetCurrentFileInfo(m_zipFile, &fileInfo, 0, 0, 0, 0, 0, 0) == UNZ_OK) {
				size = fileInfo.uncompressed_size;
			}
		}
	}

	return size;
}

aiReturn ZipFile::Seek(size_t /*pOffset*/, aiOrigin /*pOrigin*/) {
	return aiReturn_FAILURE;
}

size_t ZipFile::Tell() const {
	return 0;
}

void ZipFile::Flush() {
	// empty
}

// ------------------------------------------------------------------------------------------------
//	Constructor.
Q3BSPZipArchive::Q3BSPZipArchive(const std::string& rFile) : m_ZipFileHandle(NULL), m_ArchiveMap(), m_FileList(), m_bDirty(true) {
	if (! rFile.empty()) {
		m_ZipFileHandle = unzOpen(rFile.c_str());
		if(m_ZipFileHandle != NULL) {
			mapArchive();
		}
	}
}

// ------------------------------------------------------------------------------------------------
//	Destructor.
Q3BSPZipArchive::~Q3BSPZipArchive() {
	for( std::map<IOStream*, std::string>::iterator it(m_ArchiveMap.begin()), end(m_ArchiveMap.end()); it != end; ++it ) {
		ZipFile *pZipFile = reinterpret_cast<ZipFile*>(it->first);
		delete pZipFile;
	}
	m_ArchiveMap.clear();

	m_FileList.clear();

	if(m_ZipFileHandle != NULL) {
		unzClose(m_ZipFileHandle);
		m_ZipFileHandle = NULL;
	}
}

// ------------------------------------------------------------------------------------------------
//	Returns true, if the archive is already open.
bool Q3BSPZipArchive::isOpen() const {
	return (m_ZipFileHandle != NULL);
}

// ------------------------------------------------------------------------------------------------
//	Returns true, if the filename is part of the archive.
bool Q3BSPZipArchive::Exists(const char* pFile) const {
	ai_assert(pFile != NULL);

	bool exist = false;

	if (pFile != NULL) {
		std::string rFile(pFile);
		std::set<std::string>::const_iterator it = m_FileList.find(rFile);

		if(it != m_FileList.end()) {
			exist = true;
		}
	}

	return exist;
}

// ------------------------------------------------------------------------------------------------
//	Returns the separator delimiter.
char Q3BSPZipArchive::getOsSeparator() const {
#ifndef _WIN32
	return '/';
#else
	return '\\';
#endif
}

// ------------------------------------------------------------------------------------------------
//	Opens a file, which is part of the archive.
IOStream *Q3BSPZipArchive::Open(const char* pFile, const char* /*pMode*/) {
	ai_assert(pFile != NULL);

	IOStream* result = NULL;

	std::string rItem(pFile);
	std::set<std::string>::iterator it = m_FileList.find(rItem);

	if(it != m_FileList.end()) {
		ZipFile *pZipFile = new ZipFile(*it, m_ZipFileHandle);
		m_ArchiveMap[pZipFile] = rItem;

		result = pZipFile;
	}

	return result;
}

// ------------------------------------------------------------------------------------------------
//	Close a filestream.
void Q3BSPZipArchive::Close(IOStream *pFile) {
	ai_assert(pFile != NULL);

	std::map<IOStream*, std::string>::iterator it = m_ArchiveMap.find(pFile);

	if(it != m_ArchiveMap.end()) {
		ZipFile *pZipFile = reinterpret_cast<ZipFile*>((*it).first); 
		delete pZipFile;

		m_ArchiveMap.erase(it);
	}
}
// ------------------------------------------------------------------------------------------------
//	Returns the file-list of the archive.
void Q3BSPZipArchive::getFileList(std::vector<std::string> &rFileList) {
	rFileList.clear();

	std::copy(m_FileList.begin(), m_FileList.end(), rFileList.begin());
}

// ------------------------------------------------------------------------------------------------
//	Maps the archive content.
bool Q3BSPZipArchive::mapArchive() {
	bool success = false;

	if(m_ZipFileHandle != NULL) {
		if(m_bDirty) {
			m_FileList.clear();

			//	At first ensure file is already open
			if(unzGoToFirstFile(m_ZipFileHandle) == UNZ_OK) {	
				// Loop over all files  
				do {
					char filename[FileNameSize];

					if(unzGetCurrentFileInfo(m_ZipFileHandle, NULL, filename, FileNameSize, NULL, 0, NULL, 0) == UNZ_OK) {
						m_FileList.insert(filename);
					}
				} while(unzGoToNextFile(m_ZipFileHandle) != UNZ_END_OF_LIST_OF_FILE);
			}
	
			m_bDirty = false;
		}

		success = true;
	}

	return success;
}

// ------------------------------------------------------------------------------------------------

} // Namespace Q3BSP
} // Namespace Assimp

#endif // ASSIMP_BUILD_NO_Q3BSP_IMPORTER
