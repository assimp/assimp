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

/** @file Defines the StreamReader class which reads data from
 *  a binary stream with a well-defined endianess.
 */

#ifndef AI_STREAMREADER_H_INCLUDED
#define AI_STREAMREADER_H_INCLUDED

#include "ByteSwap.h"

namespace Assimp
{

/** Wrapper class around IOStream to allow for consistent reading
 *  of binary data in both little and big endian format.
 *  Don't use this type directly. Use StreamReaderLE to read
 *  from a little-endian stream and StreamReaderBE to read from a 
 *  BE stream. This class expects that the endianess of the data
 * is known at compile-time.
 */
template <bool SwapEndianess = false>
class ASSIMP_API StreamReader
{
public:

	/** Construction from a given stream with a well-defined endianess
	 *
	 *  @param stream Input stream
	 */
	inline StreamReader(IOStream* stream)
	{
		ai_assert(NULL != stream);

		this->input = input;
		this->stream = stream;

		size_t s = stream->GetFileSize();
		if (!s)throw new ImportErrorException("File is empty");

		current = buffer = new int8_t[s];
		end = &buffer[s];
	}

	inline ~StreamReader() {delete[] buffer;}


	/** Read a float from the stream 
	 */
	inline float GetF4()
	{
		return Get<float>();
	}

	/** Read a double from the stream 
	 */
	inline double GetF8()
	{
		return Get<double>();
	}

	/** Read a short from the stream
	 */
	inline int16_t GetI2()
	{
		return Get<int16_t>();
	}

	/** Read a char from the stream
	 */
	inline int8_t GetI1()
	{
		if (current + 1 >= end)
			throw new ImportErrorException("End of file was reached");

		return *current++;
	}

	/** Read an int from the stream
	 */
	inline int32_t GetI4()
	{
		return Get<int32_t>();
	}

	/** Read a long from the stream
	 */
	inline int64_t GetI8()
	{
		return Get<int64_t>();
	}

	/** Get the remaining stream size
	 */
	inline unsigned int GetRemainingSize()
	{
		return (unsigned int)(end - current);
	}

	// overload operator>> for those who prefer this way ...
	inline void operator >> (float& f) 
		{f = GetF4();}

	inline void operator >> (double& f) 
		{f = GetF8();}

	inline void operator >> (int16_t& f) 
		{f = GetI2();}

	inline void operator >> (int32_t& f) 
		{f = GetI4();}

	inline void operator >> (int64_t& f) 
		{f = GetI8();}

	inline void operator >> (int8_t& f) 
		{f = GetI1();}

private:

	/** Generic read method. ByteSwap::Swap(T*) must exist.
	 */
	template <typename T>
	inline T Get()
	{
		if (current + sizeof(T) >= end)
			throw new ImportErrorException("End of file was reached");

		T f = *((const T*)current);
		if (SwapEndianess)
		{
			ByteSwap::Swap(&f);
		}
		current += sizeof(T);
		return f;
	}

	IOStream* stream;
	int8_t *buffer, *current, *end;
};

#ifdef AI_BUILD_BIG_ENDIAN
	typedef StreamReader<true>  StreamReaderLE;
	typedef StreamReader<false> StreamReaderBE;
#else
	typedef StreamReader<true>  StreamReaderBE;
	typedef StreamReader<false> StreamReaderLE;
#endif

};

#endif // !! AI_STREAMREADER_H_INCLUDED