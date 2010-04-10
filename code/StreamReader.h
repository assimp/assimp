/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team

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

/** @file Defines the StreamReader class which reads data from
 *  a binary stream with a well-defined endianess. */

#ifndef AI_STREAMREADER_H_INCLUDED
#define AI_STREAMREADER_H_INCLUDED

#include "ByteSwap.h"
namespace Assimp	{

// --------------------------------------------------------------------------------------------
/** Wrapper class around IOStream to allow for consistent reading of binary data in both 
 *  little and big endian format. Don't attempt to instance the template directly. Use 
 *  StreamReaderLE to read from a little-endian stream and StreamReaderBE to read from a 
 *  BE stream. The class expects that the endianess of any input data is known at 
 *  compile-time, which should usually be true (#BaseImporter::ConvertToUTF8 implements
 *  runtime endianess conversions for text files). 
 *
 *  XXX switch from unsigned int for size types to size_t? or ptrdiff_t?*/
// --------------------------------------------------------------------------------------------
template <bool SwapEndianess = false>
class StreamReader
{
public:


	// ---------------------------------------------------------------------
	/** Construction from a given stream with a well-defined endianess
	 * 
	 *  The stream will be deleted afterwards.
	 *  @param stream Input stream
	 */
	StreamReader(IOStream* _stream)
	{
		if (!_stream) {
			throw DeadlyImportError("StreamReader: Unable to open file");
		}
		stream = _stream;

		const size_t s = stream->FileSize();
		if (!s) {
			throw DeadlyImportError("StreamReader: File is empty");
		}

		current = buffer = new int8_t[s];
		stream->Read(current,s,1);
		end = limit = &buffer[s];
	}

	~StreamReader() 
	{
		delete[] buffer;
		delete stream;
	}

public:

	// deprecated, use overloaded operator>> instead

	// ---------------------------------------------------------------------
	/** Read a float from the stream  */
	float GetF4()
	{
		return Get<float>();
	}

	// ---------------------------------------------------------------------
	/** Read a double from the stream  */
	double GetF8()	{
		return Get<double>();
	}

	// ---------------------------------------------------------------------
	/** Read a signed 16 bit integer from the stream */
	int16_t GetI2()	{
		return Get<int16_t>();
	}

	// ---------------------------------------------------------------------
	/** Read a signed 8 bit integer from the stream */
	int8_t GetI1()	{
		return Get<int8_t>();
	}

	// ---------------------------------------------------------------------
	/** Read an signed 32 bit integer from the stream */
	int32_t GetI4()	{
		return Get<int32_t>();
	}

	// ---------------------------------------------------------------------
	/** Read a signed 64 bit integer from the stream */
	int64_t GetI8()	{
		return Get<int64_t>();
	}

	// ---------------------------------------------------------------------
	/** Read a unsigned 16 bit integer from the stream */
	uint16_t GetU2()	{
		return Get<uint16_t>();
	}

	// ---------------------------------------------------------------------
	/** Read a unsigned 8 bit integer from the stream */
	uint8_t GetU1()	{
		return Get<uint8_t>();
	}

	// ---------------------------------------------------------------------
	/** Read an unsigned 32 bit integer from the stream */
	uint32_t GetU4()	{
		return Get<uint32_t>();
	}

	// ---------------------------------------------------------------------
	/** Read a unsigned 64 bit integer from the stream */
	uint64_t GetU8()	{
		return Get<uint64_t>();
	}

public:

	// ---------------------------------------------------------------------
	/** Get the remaining stream size (to the end of the srream) */
	unsigned int GetRemainingSize() const {
		return (unsigned int)(end - current);
	}


	// ---------------------------------------------------------------------
	/** Get the remaining stream size (to the current read limit). The
	 *  return value is the remaining size of the stream if no custom
	 *  read limit has been set. */
	unsigned int GetRemainingSizeToLimit() const {
		return (unsigned int)(limit - current);
	}


	// ---------------------------------------------------------------------
	/** Increase the file pointer (relative seeking)  */
	void IncPtr(int plus)	{
		current += plus;
		if (current > end) {
			throw DeadlyImportError("End of file was reached");
		}
	}

	// ---------------------------------------------------------------------
	/** Get the current file pointer */
	int8_t* GetPtr() const	{
		return current;
	}


	// ---------------------------------------------------------------------
	/** Set current file pointer (Get it from #GetPtr). This is if you
	 *  prefer to do pointer arithmetics on your own or want to copy 
	 *  large chunks of data at once. 
	 *  @param p The new pointer, which is validated against the size
	 *    limit and buffer boundaries. */
	void SetPtr(int8_t* p)	{

		current = p;
		if (current > end || current < buffer) {
			throw DeadlyImportError("End of file was reached");
		}
	}

	// ---------------------------------------------------------------------
	/** Copy n bytes to an external buffer
	 *  @param out Destination for copying
	 *  @param bytes Number of bytes to copy */
	void CopyAndAdvance(void* out, size_t bytes)	{

		int8_t* ur = GetPtr();
		SetPtr(ur+bytes); // fire exception if eof

		memcpy(out,ur,bytes);
	}


	// ---------------------------------------------------------------------
	/** Get the current offset from the beginning of the file */
	int GetCurrentPos() const	{
		return (unsigned int)(current - buffer);
	}

	// ---------------------------------------------------------------------
	/** Setup a temporary read limit
	 * 
	 *  @param limit Maximum number of bytes to be read from
	 *    the beginning of the file. Passing 0xffffffff
	 *    resets the limit to the original end of the stream. */
	void SetReadLimit(unsigned int _limit)	{

		if (0xffffffff == _limit) {
			limit = end;
			return;
		}

		limit = buffer + _limit;
		if (limit > end) {
			throw DeadlyImportError("StreamReader: Invalid read limit");
		}
	}

	// ---------------------------------------------------------------------
	/** Get the current read limit in bytes. Reading over this limit
	 *  accidentially raises an exception.  */
	int GetReadLimit() const	{
		return (unsigned int)(limit - buffer);
	}

	// ---------------------------------------------------------------------
	/** Skip to the read limit in bytes. Reading over this limit
	 *  accidentially raises an exception. */
	void SkipToReadLimit()	{
		current = limit;
	}

	// ---------------------------------------------------------------------
	/** overload operator>> and allow chaining of >> ops. */
	template <typename T>
	StreamReader& operator >> (T& f) {
		f = Get<T>(); 
		return *this;
	}

private:

	template <typename T, bool doit>
	struct ByteSwapper	{
		void operator() (T* inout) {
			ByteSwap::Swap(inout);
		}
	};

	template <typename T>
	struct ByteSwapper<T,false>	{
		void operator() (T*) {
		}
	};

	// ---------------------------------------------------------------------
	/** Generic read method. ByteSwap::Swap(T*) *must* be defined */
	template <typename T>
	T Get()	{
		if (current + sizeof(T) > limit) {
			throw DeadlyImportError("End of file or stream limit was reached");
		}

		T f = *((const T*)current);
		ByteSwapper<T,(SwapEndianess && sizeof(T)>1)> swapper;
		swapper(&f);

		current += sizeof(T);
		return f;
	}

	IOStream* stream;
	int8_t *buffer, *current, *end, *limit;
};


// --------------------------------------------------------------------------------------------
#ifdef AI_BUILD_BIG_ENDIAN
	typedef StreamReader<true>  StreamReaderLE;
	typedef StreamReader<false> StreamReaderBE;
#else
	typedef StreamReader<true>  StreamReaderBE;
	typedef StreamReader<false> StreamReaderLE;
#endif

} // end namespace Assimp

#endif // !! AI_STREAMREADER_H_INCLUDED
