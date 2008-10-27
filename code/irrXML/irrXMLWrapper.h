
#ifndef AI_IRRXML_WRAPPER_H_INCLUDED
#define AI_IRRXML_WRAPPER_H_INCLUDED

#include "irrXML.h"
#include "./../../include/IOStream.h"

namespace Assimp {
using namespace irr;
using namespace irr::io;

class CIrrXML_IOStreamReader
{
public:

	CIrrXML_IOStreamReader(IOStream* _stream)
		: stream (_stream)
	{}

	//! virtual destructor
	virtual ~CIrrXML_IOStreamReader() {};

	//! Reads an amount of bytes from the file.
	/** \param buffer: Pointer to buffer where to read bytes will be written to.
	\param sizeToRead: Amount of bytes to read from the file.
	\return Returns how much bytes were read. */
	virtual int read(void* buffer, int sizeToRead)
	{
		return (int)stream->Read(buffer,1,sizeToRead);
	}

	//! Returns size of file in bytes
	virtual int getSize() 
	{
		return (int)stream->FileSize();
	}

private:

	IOStream* stream;
};


} // ! Assimp

#endif
