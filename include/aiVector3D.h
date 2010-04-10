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
/** @file aiVector3D.h
 *  @brief 3D vector structure, including operators when compiling in C++
 */
#ifndef AI_VECTOR3D_H_INC
#define AI_VECTOR3D_H_INC

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "./Compiler/pushpack1.h"

struct aiMatrix3x3;
struct aiMatrix4x4;

// ---------------------------------------------------------------------------
/** Represents a three-dimensional vector. */
struct aiVector3D
{
#ifdef __cplusplus
	aiVector3D () : x(0.0f), y(0.0f), z(0.0f) {}
	aiVector3D (float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
	aiVector3D (float _xyz) : x(_xyz), y(_xyz), z(_xyz) {}
	aiVector3D (const aiVector3D& o) : x(o.x), y(o.y), z(o.z) {}

public:

	// combined operators
	const aiVector3D& operator += (const aiVector3D& o);
	const aiVector3D& operator -= (const aiVector3D& o);
	const aiVector3D& operator *= (float f);
	const aiVector3D& operator /= (float f);

	// transform vector by matrix
	aiVector3D& operator *= (const aiMatrix3x3& mat);
	aiVector3D& operator *= (const aiMatrix4x4& mat);

	// access a single element
	float operator[](unsigned int i) const;
	float& operator[](unsigned int i);

	// comparison
	bool operator== (const aiVector3D& other) const;
	bool operator!= (const aiVector3D& other) const;

public:

	/** @brief Set the components of a vector
	 *  @param pX X component
	 *  @param pY Y component
	 *  @param pZ Z component  */
	void Set( float pX, float pY, float pZ = 0.f);

	/** @brief Get the squared length of the vector
	 *  @return Square length */
	float SquareLength() const;


	/** @brief Get the length of the vector
	 *  @return length */
	float Length() const;


	/** @brief Normalize the vector */
	aiVector3D& Normalize();

	
	/** @brief Componentwise multiplication of two vectors
	 *  
	 *  Note that vec*vec yields the dot product.
	 *  @param o Second factor */
	const aiVector3D SymMul(const aiVector3D& o);

#endif // __cplusplus

	float x, y, z;	
} PACK_STRUCT;

#include "./Compiler/poppack1.h"

#ifdef __cplusplus
} // end extern "C"


#endif // __cplusplus

#endif // AI_VECTOR3D_H_INC
