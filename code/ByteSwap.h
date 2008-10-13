/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team
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

----------------------------------------------------------------------
*/

/** @file Helper class tp perform various byte oder swappings 
   (e.g. little to big endian) */
#ifndef AI_BYTESWAP_H_INC
#define AI_BYTESWAP_H_INC

#include "../include/aiAssert.h"
#include "../include/aiTypes.h"

namespace Assimp
{

// ---------------------------------------------------------------------------
/** \brief Defines some useful byte order swap routines.
 * 
 * This can e.g. be used for the conversion from and to littel and big endian.
 * This is required by some loaders since some model formats
 */
class ByteSwap
{
	ByteSwap() {}

public:
	//! Swap the byte oder of 2 byte of data
	//! \param szOut Buffer to be swapped
	static inline void Swap2(void* _szOut)
	{
		ai_assert(NULL != _szOut);
		int8_t* szOut = (int8_t*)_szOut;
		std::swap(szOut[0],szOut[1]);
	}

	//! Swap the byte oder of 4 byte of data
	//! \param szOut Buffer to be swapped
	static inline void Swap4(void* _szOut)
	{
		ai_assert(NULL != _szOut);

#if _MSC_VER >= 1400 && (defined _M_X86)
		__asm
		{
			mov edi, _szOut
			mov eax, dword_ptr[edi]
			bswap eax
			mov dword_ptr[edi], eax
		};
#else
		int8_t* szOut = (int8_t*)_szOut;
		std::swap(szOut[0],szOut[3]);
		std::swap(szOut[1],szOut[2]);
#endif
	}

	//! Swap the byte oder of 8 byte of data
	//! \param szOut Buffer to be swapped
	static inline void Swap8(void* _szOut)
	{
		ai_assert(NULL != _szOut);
		int8_t* szOut = (int8_t*)_szOut;
		std::swap(szOut[0],szOut[7]);
		std::swap(szOut[1],szOut[6]);
		std::swap(szOut[2],szOut[5]);
		std::swap(szOut[3],szOut[4]);
	}

	//! Swap a single precision float
	//! \param fOut Float value to be swapped
	static inline void Swap(float* fOut)
	{
		Swap4(fOut);
	}

	//! Swap a double precision float
	//! \param fOut Double value to be swapped
	static inline void Swap(double* fOut)
	{
		Swap8(fOut);
	}

	//! Swap a 16 bit integer
	//! \param fOut Integer to be swapped
	static inline void Swap(int16_t* fOut)
	{
		Swap2(fOut);
	}

	//! Swap a 32 bit integer
	//! \param fOut Integer to be swapped
	static inline void Swap(int32_t* fOut)	
	{
		Swap4(fOut);
	}

	//! Swap a 64 bit integer
	//! \param fOut Integer to be swapped
	static inline void Swap(int64_t* fOut)
	{
		Swap8(fOut);
	}
};

} // Namespace Assimp

// byteswap macros for BigEndian/LittleEndian support 
#if (defined AI_BUILD_BIG_ENDIAN)
#	define AI_LSWAP2(p)
#	define AI_LSWAP4(p)
#	define AI_LSWAP8(p)
#	define AI_LSWAP2P(p)
#	define AI_LSWAP4P(p)
#	define AI_LSWAP8P(p)
#	define LE_NCONST const
#	define AI_SWAP2(p) ByteSwap::Swap2(&(p))
#	define AI_SWAP4(p) ByteSwap::Swap4(&(p))
#	define AI_SWAP8(p) ByteSwap::Swap8(&(p))
#	define AI_SWAP2P(p) ByteSwap::Swap2((p))
#	define AI_SWAP4P(p) ByteSwap::Swap4((p))
#	define AI_SWAP8P(p) ByteSwap::Swap8((p))
#	define BE_NCONST
#else
#	define AI_SWAP2(p)
#	define AI_SWAP4(p)
#	define AI_SWAP8(p)
#	define AI_SWAP2P(p)
#	define AI_SWAP4P(p)
#	define AI_SWAP8P(p)
#	define BE_NCONST const
#	define AI_LSWAP2(p)		ByteSwap::Swap2(&(p))
#	define AI_LSWAP4(p)		ByteSwap::Swap4(&(p))
#	define AI_LSWAP8(p)		ByteSwap::Swap8(&(p))
#	define AI_LSWAP2P(p)	ByteSwap::Swap2((p))
#	define AI_LSWAP4P(p)	ByteSwap::Swap4((p))
#	define AI_LSWAP8P(p)	ByteSwap::Swap8((p))
#	define LE_NCONST
#endif



#endif //!! AI_BYTESWAP_H_INC
