/** @file Default file I/O using fXXX()-family of functions */
#ifndef AI_DEFAULTIOSTREAM_H_INC
#define AI_DEFAULTIOSTREAM_H_INC

#include <string>
#include <stdio.h>
#include "IOStream.h"

namespace Assimp
{

// ---------------------------------------------------------------------------
//!	\class	DefaultIOStream
//!	\brief	Default IO implementation, use standard IO operations
// ---------------------------------------------------------------------------
class DefaultIOStream : public IOStream
{
	friend class DefaultIOSystem;

protected:
	DefaultIOStream ();
	DefaultIOStream (FILE* pFile, const std::string &strFilename);

public:
	/** Destructor public to allow simple deletion to close the file. */
	~DefaultIOStream ();

	// -------------------------------------------------------------------
	// -------------------------------------------------------------------
    size_t Read(void* pvBuffer, 
		size_t pSize, 
		size_t pCount);


	// -------------------------------------------------------------------
	// -------------------------------------------------------------------
    size_t Write(const void* pvBuffer, 
		size_t pSize,
		size_t pCount);


	// -------------------------------------------------------------------
	// -------------------------------------------------------------------
	aiReturn Seek(size_t pOffset,
		aiOrigin pOrigin);


	// -------------------------------------------------------------------
	// -------------------------------------------------------------------
    size_t Tell() const;

	//!	Returns filesize
	size_t FileSize() const;

private:
	//!	File datastructure, using clib
	FILE* mFile;
	//!	Filename
	std::string	mFilename;
};
// ---------------------------------------------------------------------------
inline DefaultIOStream::DefaultIOStream () : 
	mFile(NULL), 
	mFilename("") 
{
	// empty
}

// ---------------------------------------------------------------------------
inline DefaultIOStream::DefaultIOStream (FILE* pFile, 
		const std::string &strFilename) :
	mFile(pFile), 
	mFilename(strFilename)
{
	// empty
}

// ---------------------------------------------------------------------------

} // ns assimp

#endif //!!AI_DEFAULTIOSTREAM_H_INC
