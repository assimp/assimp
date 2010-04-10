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

/** @file aiMatrix3x3.h
 *  @brief Definition of a 3x3 matrix, including operators when compiling in C++
 */
#ifndef AI_MATRIX3x3_H_INC
#define AI_MATRIX3x3_H_INC

#ifdef __cplusplus
extern "C" {
#endif

struct aiMatrix4x4;
struct aiVector2D;

// ---------------------------------------------------------------------------
/** @brief Represents a row-major 3x3 matrix
 *
 *  There's much confusion about matrix layouts (colum vs. row order). 
 *  This is *always* a row-major matrix. Even with the
 *  aiProcess_ConvertToLeftHanded flag.
 */
struct aiMatrix3x3
{
#ifdef __cplusplus

	aiMatrix3x3 () :	
		a1(1.0f), a2(0.0f), a3(0.0f), 
		b1(0.0f), b2(1.0f), b3(0.0f), 
		c1(0.0f), c2(0.0f), c3(1.0f) {}

	aiMatrix3x3 (	float _a1, float _a2, float _a3,
					float _b1, float _b2, float _b3,
					float _c1, float _c2, float _c3) :	
		a1(_a1), a2(_a2), a3(_a3), 
		b1(_b1), b2(_b2), b3(_b3), 
		c1(_c1), c2(_c2), c3(_c3)
	{}

public:

	// matrix multiplication. beware, not commutative
	aiMatrix3x3& operator *= (const aiMatrix3x3& m);
	aiMatrix3x3  operator  * (const aiMatrix3x3& m) const;

	// array access operators
	float* operator[]       (unsigned int p_iIndex);
	const float* operator[] (unsigned int p_iIndex) const;

	// comparison operators
	bool operator== (const aiMatrix4x4 m) const;
	bool operator!= (const aiMatrix4x4 m) const;

public:

	// -------------------------------------------------------------------
	/** @brief Construction from a 4x4 matrix. The remaining parts 
	 *  of the matrix are ignored.
	 */
	explicit aiMatrix3x3( const aiMatrix4x4& pMatrix);

	// -------------------------------------------------------------------
	/** @brief Transpose the matrix
	 */
	aiMatrix3x3& Transpose();

	// -------------------------------------------------------------------
	/** @brief Invert the matrix.
	 *  If the matrix is not invertible all elements are set to qnan.
	 *  Beware, use (f != f) to check whether a float f is qnan.
	 */
	aiMatrix3x3& Inverse();
	float Determinant() const;

public:
	// -------------------------------------------------------------------
	/** @brief Returns a rotation matrix for a rotation around z
	 *  @param a Rotation angle, in radians
	 *  @param out Receives the output matrix
	 *  @return Reference to the output matrix
	 */
	static aiMatrix3x3& RotationZ(float a, aiMatrix3x3& out);

	// -------------------------------------------------------------------
	/** @brief Returns a rotation matrix for a rotation around
	 *    an arbitrary axis.
	 *
	 *  @param a Rotation angle, in radians
	 *  @param axis Axis to rotate around
	 *  @param out To be filled
	 */
	static aiMatrix3x3& Rotation( float a, 
		const aiVector3D& axis, aiMatrix3x3& out);

	// -------------------------------------------------------------------
	/** @brief Returns a translation matrix 
	 *  @param v Translation vector
	 *  @param out Receives the output matrix
	 *  @return Reference to the output matrix
	 */
	static aiMatrix3x3& Translation( const aiVector2D& v, aiMatrix3x3& out);

	// -------------------------------------------------------------------
	/** @brief A function for creating a rotation matrix that rotates a
	 *  vector called "from" into another vector called "to".
	 * Input : from[3], to[3] which both must be *normalized* non-zero vectors
	 * Output: mtx[3][3] -- a 3x3 matrix in colum-major form
	 * Authors: Tomas Möller, John Hughes
	 *          "Efficiently Building a Matrix to Rotate One Vector to Another"
	 *          Journal of Graphics Tools, 4(4):1-4, 1999
	 */
	static aiMatrix3x3& FromToMatrix(const aiVector3D& from, 
		const aiVector3D& to, aiMatrix3x3& out);

#endif // __cplusplus


	float a1, a2, a3;
	float b1, b2, b3;
	float c1, c2, c3;
};

#ifdef __cplusplus
} // end of extern C
#endif

#endif // AI_MATRIX3x3_H_INC
