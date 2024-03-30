/*
 * Copyright (c) 2011 Apple Inc. All rights reserved.
 *
 * @APPLE_APACHE_LICENSE_HEADER_START@
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * @APPLE_APACHE_LICENSE_HEADER_END@
 */

//
//  EndianPortable.c
//
//  Copyright 2011 Apple Inc. All rights reserved.
//

#include <stdio.h>
#include "EndianPortable.h"

#define BSWAP16(x) (((x << 8) | ((x >> 8) & 0x00ff)))
#define BSWAP32(x) (((x << 24) | ((x << 8) & 0x00ff0000) | ((x >> 8) & 0x0000ff00) | ((x >> 24) & 0x000000ff)))
#define BSWAP64(x) ((((int64_t)x << 56) | (((int64_t)x << 40) & 0x00ff000000000000LL) | \
                    (((int64_t)x << 24) & 0x0000ff0000000000LL) | (((int64_t)x << 8) & 0x000000ff00000000LL) | \
                    (((int64_t)x >> 8) & 0x00000000ff000000LL) | (((int64_t)x >> 24) & 0x0000000000ff0000LL) | \
                    (((int64_t)x >> 40) & 0x000000000000ff00LL) | (((int64_t)x >> 56) & 0x00000000000000ffLL)))

#if defined(__i386__)
#define TARGET_RT_LITTLE_ENDIAN 1
#elif defined(__x86_64__)
#define TARGET_RT_LITTLE_ENDIAN 1
#elif defined (TARGET_OS_WIN32)
#define TARGET_RT_LITTLE_ENDIAN 1
#endif

uint16_t Swap16NtoB(uint16_t inUInt16)
{
#if TARGET_RT_LITTLE_ENDIAN
    return BSWAP16(inUInt16);
#else
    return inUInt16;
#endif
}

uint16_t Swap16BtoN(uint16_t inUInt16)
{
#if TARGET_RT_LITTLE_ENDIAN
    return BSWAP16(inUInt16);
#else
    return inUInt16;
#endif
}

uint32_t Swap32NtoB(uint32_t inUInt32)
{
#if TARGET_RT_LITTLE_ENDIAN
    return BSWAP32(inUInt32);
#else
    return inUInt32;
#endif
}

uint32_t Swap32BtoN(uint32_t inUInt32)
{
#if TARGET_RT_LITTLE_ENDIAN
    return BSWAP32(inUInt32);
#else
    return inUInt32;
#endif
}

uint64_t Swap64BtoN(uint64_t inUInt64)
{
#if TARGET_RT_LITTLE_ENDIAN
    return BSWAP64(inUInt64);
#else
    return inUInt64;
#endif
}

uint64_t Swap64NtoB(uint64_t inUInt64)
{
#if TARGET_RT_LITTLE_ENDIAN
    return BSWAP64(inUInt64);
#else
    return inUInt64;
#endif
}

float SwapFloat32BtoN(float in)
{
#if TARGET_RT_LITTLE_ENDIAN
	union {
		float f;
		int32_t i;
	} x;
	x.f = in;	
	x.i = BSWAP32(x.i);
	return x.f;
#else
	return in;
#endif
}

float SwapFloat32NtoB(float in)
{
#if TARGET_RT_LITTLE_ENDIAN
	union {
		float f;
		int32_t i;
	} x;
	x.f = in;	
	x.i = BSWAP32(x.i);
	return x.f;
#else
	return in;
#endif
}

double SwapFloat64BtoN(double in)
{
#if TARGET_RT_LITTLE_ENDIAN
	union {
		double f;
		int64_t i;
	} x;
	x.f = in;	
	x.i = BSWAP64(x.i);
	return x.f;
#else
	return in;
#endif
}

double SwapFloat64NtoB(double in)
{
#if TARGET_RT_LITTLE_ENDIAN
	union {
		double f;
		int64_t i;
	} x;
	x.f = in;	
	x.i = BSWAP64(x.i);
	return x.f;
#else
	return in;
#endif
}

void Swap16(uint16_t * inUInt16)
{
	*inUInt16 = BSWAP16(*inUInt16);
}

void Swap24(uint8_t * inUInt24)
{
	uint8_t tempVal = inUInt24[0];
	inUInt24[0] = inUInt24[2];
	inUInt24[2] = tempVal;
}

void Swap32(uint32_t * inUInt32)
{
	*inUInt32 = BSWAP32(*inUInt32);
}

