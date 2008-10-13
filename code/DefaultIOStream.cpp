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
/** @file Default File I/O implementation for #Importer */

#include "AssimpPCH.h"

#include "DefaultIOStream.h"
#include "../include/aiAssert.h"
#include <sys/types.h> 
#include <sys/stat.h> 

using namespace Assimp;


// ---------------------------------------------------------------------------
DefaultIOStream::~DefaultIOStream()
{
	if (this->mFile)
	{
		::fclose(this->mFile);
	}	
}
// ---------------------------------------------------------------------------
size_t DefaultIOStream::Read(void* pvBuffer, 
								size_t pSize, 
								size_t pCount)
{
	ai_assert(NULL != pvBuffer && 0 != pSize && 0 != pCount);

	if (!this->mFile)
		return 0;

	return ::fread(pvBuffer, pSize, pCount, this->mFile);
}
// ---------------------------------------------------------------------------
size_t DefaultIOStream::Write(const void* pvBuffer, 
								 size_t pSize,
								 size_t pCount)
{
	ai_assert(NULL != pvBuffer && 0 != pSize && 0 != pCount);

	if (!this->mFile)return 0;

	::fseek(mFile, 0, SEEK_SET);
	return ::fwrite(pvBuffer, pSize, pCount, this->mFile);
}
// ---------------------------------------------------------------------------
aiReturn DefaultIOStream::Seek(size_t pOffset,
						   aiOrigin pOrigin)
{
	if (!this->mFile)return AI_FAILURE;

	return (0 == ::fseek(this->mFile, (long)pOffset,
		(aiOrigin_CUR == pOrigin ? SEEK_CUR :
		(aiOrigin_END == pOrigin ? SEEK_END : SEEK_SET))) 
		? AI_SUCCESS : AI_FAILURE);
}
// ---------------------------------------------------------------------------
size_t DefaultIOStream::Tell() const
{
	if (!this->mFile)return 0;

	return ::ftell(this->mFile);
}
// ---------------------------------------------------------------------------
size_t DefaultIOStream::FileSize() const
{
	ai_assert (!mFilename.empty());

	if (NULL == mFile)
		return 0;
#if defined _WIN32 && !defined __GNUC__
	struct __stat64 fileStat; 
	int err = _stat64(  mFilename.c_str(), &fileStat ); 
	if (0 != err) 
		return 0; 
	return (size_t) (fileStat.st_size); 
#else
	struct stat fileStat; 
	int err = stat(mFilename.c_str(), &fileStat ); 
	if (0 != err) 
		return 0; 
	return (size_t) (fileStat.st_size); 
#endif
}
// ---------------------------------------------------------------------------
