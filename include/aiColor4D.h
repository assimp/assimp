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
/** @file aiColor4D.h
 *  @brief RGBA color structure, including operators when compiling in C++
 */
#ifndef AI_COLOR4D_H_INC
#define AI_COLOR4D_H_INC

#ifdef __cplusplus
extern "C" {
#endif

#include "./Compiler/pushpack1.h"
// ----------------------------------------------------------------------------------
/** Represents a color in Red-Green-Blue space including an 
*   alpha component. Color values range from 0 to 1. */
// ----------------------------------------------------------------------------------
struct aiColor4D
{
#ifdef __cplusplus
	aiColor4D () : r(0.0f), g(0.0f), b(0.0f), a(0.0f) {}
	aiColor4D (float _r, float _g, float _b, float _a) 
		: r(_r), g(_g), b(_b), a(_a) {}
	aiColor4D (float _r) : r(_r), g(_r), b(_r), a(_r) {}
	aiColor4D (const aiColor4D& o) 
		: r(o.r), g(o.g), b(o.b), a(o.a) {}

	// combined operators
	const aiColor4D& operator += (const aiColor4D& o);
	const aiColor4D& operator -= (const aiColor4D& o);
	const aiColor4D& operator *= (float f);
	const aiColor4D& operator /= (float f);

	// comparison
	bool operator == (const aiColor4D& other) const;
	bool operator != (const aiColor4D& other) const;

	// color tuple access, rgba order
	inline float operator[](unsigned int i) const;
	inline float& operator[](unsigned int i);

	/** check whether a color is (close to) black */
	inline bool IsBlack() const;

#endif // !__cplusplus

	// Red, green, blue and alpha color values 
	float r, g, b, a;
} PACK_STRUCT;  // !struct aiColor4D

#include "./Compiler/poppack1.h"
#ifdef __cplusplus
} // end extern "C"

#endif // __cplusplus
#endif // AI_VECTOR3D_H_INC
