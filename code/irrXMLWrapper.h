
#ifndef INCLUDED_AI_IRRXML_WRAPPER
#define INCLUDED_AI_IRRXML_WRAPPER

// some long includes ....
#include "./../contrib/irrXML/irrXML.h"
#include "./../include/IOStream.h"

namespace Assimp 
{

// ---------------------------------------------------------------------------------
/** @brief Utility class to make IrrXML work together with our custom IO system
 *
 *  See the IrrXML docs for more details.
 */
class CIrrXML_IOStreamReader : public irr::io::IFileReadCallBack
{
public:

	//! Construction from an existing IOStream
	CIrrXML_IOStreamReader(IOStream* _stream)
		: stream (_stream)
	{}

	//! Virtual destructor
	virtual ~CIrrXML_IOStreamReader() {};

	//!   Reads an amount of bytes from the file.
	/**  @param buffer:       Pointer to output buffer.
	 *   @param sizeToRead:   Amount of bytes to read 
	 *   @return              Returns how much bytes were read.
	 */
	virtual int read(void* buffer, int sizeToRead)	{
		return (int)stream->Read(buffer,1,sizeToRead);
	}

	//! Returns size of file in bytes
	virtual int getSize()	{
		return (int)stream->FileSize();
	}

private:
	IOStream* stream;
}; // ! class CIrrXML_IOStreamReader

} // ! Assimp

#endif // !! INCLUDED_AI_IRRXML_WRAPPER
