/** @file Default File I/O implementation for #Importer */

#include "DefaultIOStream.h"
#include "aiAssert.h"
#include <sys/types.h> 
#include <sys/stat.h> 

using namespace Assimp;


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
DefaultIOStream::~DefaultIOStream()
{
	if (this->mFile)
	{
		fclose(this->mFile);
	}	
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
size_t DefaultIOStream::Read(void* pvBuffer, 
								size_t pSize, 
								size_t pCount)
{
	if (!this->mFile)
		return 0;

	return fread(pvBuffer, pSize, pCount, this->mFile);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
size_t DefaultIOStream::Write(const void* pvBuffer, 
								 size_t pSize,
								 size_t pCount)
{
	if (!this->mFile)return 0;

	fseek(mFile, 0, SEEK_SET);
	return fwrite(pvBuffer, pSize, pCount, this->mFile);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
aiReturn DefaultIOStream::Seek(size_t pOffset,
						   aiOrigin pOrigin)
{
	if (!this->mFile)return AI_FAILURE;

	return (0 == fseek(this->mFile, pOffset,
		(aiOrigin_CUR == pOrigin ? SEEK_CUR :
		(aiOrigin_END == pOrigin ? SEEK_END : SEEK_SET))) 
		? AI_SUCCESS : AI_FAILURE);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
size_t DefaultIOStream::Tell() const
{
	if (!this->mFile)return 0;

	return ftell(this->mFile);
}

// ---------------------------------------------------------------------------
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
