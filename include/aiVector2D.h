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
/** @file aiVector2D.h
 *  @brief 2D vector structure, including operators when compiling in C++
 */
#ifndef AI_VECTOR2D_H_INC
#define AI_VECTOR2D_H_INC

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "./Compiler/pushpack1.h"

// ----------------------------------------------------------------------------------
/** Represents a two-dimensional vector. 
 */
struct aiVector2D
{
#ifdef __cplusplus
	aiVector2D () : x(0.0f), y(0.0f) {}
	aiVector2D (float _x, float _y) : x(_x), y(_y) {}
	aiVector2D (float _xyz) : x(_xyz), y(_xyz) {}
	aiVector2D (const aiVector2D& o) : x(o.x), y(o.y) {}

	void Set( float pX, float pY) { 
		x = pX; y = pY;
	}
	
	float SquareLength() const {
		return x*x + y*y; 
	}
	
	float Length() const {
		return ::sqrt( SquareLength());
	}

	aiVector2D& Normalize() { 
		*this /= Length(); return *this;
	}

	const aiVector2D& operator += (const aiVector2D& o) {
		x += o.x; y += o.y;  return *this; 
	}
	const aiVector2D& operator -= (const aiVector2D& o) {
		x -= o.x; y -= o.y;  return *this; 
	}
	const aiVector2D& operator *= (float f) { 
		x *= f; y *= f;  return *this; 
	}
	const aiVector2D& operator /= (float f) {
		x /= f; y /= f;  return *this; 
	}

	float operator[](unsigned int i) const {
		return *(&x + i);
	}

	float& operator[](unsigned int i) {
		return *(&x + i);
	}

	bool operator== (const aiVector2D& other) const {
		return x == other.x && y == other.y;
	}

	bool operator!= (const aiVector2D& other) const {
		return x != other.x || y != other.y;
	}

	aiVector2D& operator= (float f)	{
		x = y = f;return *this;
	}

	const aiVector2D SymMul(const aiVector2D& o) {
		return aiVector2D(x*o.x,y*o.y);
	}

#endif // __cplusplus

	float x, y;	
} PACK_STRUCT;

#include "./Compiler/poppack1.h"

#ifdef __cplusplus
} // end extern "C"

// ----------------------------------------------------------------------------------
// symmetric addition
inline aiVector2D operator + (const aiVector2D& v1, const aiVector2D& v2)
{
	return aiVector2D( v1.x + v2.x, v1.y + v2.y);
}

// ----------------------------------------------------------------------------------
// symmetric subtraction
inline aiVector2D operator - (const aiVector2D& v1, const aiVector2D& v2)
{
	return aiVector2D( v1.x - v2.x, v1.y - v2.y);
}

// ----------------------------------------------------------------------------------
// scalar product
inline float operator * (const aiVector2D& v1, const aiVector2D& v2)
{
	return v1.x*v2.x + v1.y*v2.y;
}

// ----------------------------------------------------------------------------------
// scalar multiplication
inline aiVector2D operator * ( float f, const aiVector2D& v)
{
	return aiVector2D( f*v.x, f*v.y);
}

// ----------------------------------------------------------------------------------
// and the other way around
inline aiVector2D operator * ( const aiVector2D& v, float f)
{
	return aiVector2D( f*v.x, f*v.y);
}

// ----------------------------------------------------------------------------------
// scalar division
inline aiVector2D operator / ( const aiVector2D& v, float f)
{
	
	return v * (1/f);
}

// ----------------------------------------------------------------------------------
// vector division
inline aiVector2D operator / ( const aiVector2D& v, const aiVector2D& v2)
{
	return aiVector2D(v.x / v2.x,v.y / v2.y);
}

// ----------------------------------------------------------------------------------
// vector inversion
inline aiVector2D operator - ( const aiVector2D& v)
{
	return aiVector2D( -v.x, -v.y);
}

#endif // __cplusplus
#endif // AI_VECTOR2D_H_INC
