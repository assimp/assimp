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
/** @file aiMatrix4x4.h
 *  @brief 4x4 matrix structure, including operators when compiling in C++
 */
#ifndef AI_MATRIX4X4_H_INC
#define AI_MATRIX4X4_H_INC

#ifdef __cplusplus
extern "C" {
#endif

struct aiMatrix3x3;
struct aiQuaternion;

#include "./Compiler/pushpack1.h"

// ---------------------------------------------------------------------------
/** @brief Represents a row-major 4x4 matrix, use this for homogeneous
 *   coordinates.
 *
 *  There's much confusion about matrix layouts (colum vs. row order). 
 *  This is *always* a row-major matrix. Even with the
 *  aiProcess_ConvertToLeftHanded flag.
 */
struct aiMatrix4x4
{
#ifdef __cplusplus

		// default c'tor, init to zero
	aiMatrix4x4 () :	
		a1(1.0f), a2(0.0f), a3(0.0f), a4(0.0f), 
		b1(0.0f), b2(1.0f), b3(0.0f), b4(0.0f), 
		c1(0.0f), c2(0.0f), c3(1.0f), c4(0.0f),
		d1(0.0f), d2(0.0f), d3(0.0f), d4(1.0f)
	{}

		// from single values
	aiMatrix4x4 (	float _a1, float _a2, float _a3, float _a4,
					float _b1, float _b2, float _b3, float _b4,
					float _c1, float _c2, float _c3, float _c4,
					float _d1, float _d2, float _d3, float _d4) :	
		a1(_a1), a2(_a2), a3(_a3), a4(_a4),  
		b1(_b1), b2(_b2), b3(_b3), b4(_b4), 
		c1(_c1), c2(_c2), c3(_c3), c4(_c4),
		d1(_d1), d2(_d2), d3(_d3), d4(_d4)
	{}


	// -------------------------------------------------------------------
	/** @brief Constructor from 3x3 matrix. 
	 *  The remaining elements are set to identity.
	 */
	explicit aiMatrix4x4( const aiMatrix3x3& m);

public:

	// array access operators
	float* operator[]       (unsigned int p_iIndex);
	const float* operator[] (unsigned int p_iIndex) const;

	// comparison operators
	bool operator== (const aiMatrix4x4 m) const;
	bool operator!= (const aiMatrix4x4 m) const;

	// Matrix multiplication. Not commutative.
	aiMatrix4x4& operator *= (const aiMatrix4x4& m);
	aiMatrix4x4  operator *  (const aiMatrix4x4& m) const;

public:

	// -------------------------------------------------------------------
	/** @brief Transpose the matrix
	 */
	aiMatrix4x4& Transpose();

	// -------------------------------------------------------------------
	/** @brief Invert the matrix.
	 *  If the matrix is not invertible all elements are set to qnan.
	 *  Beware, use (f != f) to check whether a float f is qnan.
	 */
	aiMatrix4x4& Inverse();
	float Determinant() const;


	// -------------------------------------------------------------------
	/** @brief Returns true of the matrix is the identity matrix.
	 *  The check is performed against a not so small epsilon.
	 */
	inline bool IsIdentity() const;

	// -------------------------------------------------------------------
	/** @brief Decompose a trafo matrix into its original components
	 *  @param scaling Receives the output scaling for the x,y,z axes
	 *  @param rotation Receives the output rotation as a hamilton
	 *   quaternion 
	 *  @param position Receives the output position for the x,y,z axes
	 */
	void Decompose (aiVector3D& scaling, aiQuaternion& rotation,
		aiVector3D& position) const;

	// -------------------------------------------------------------------
	/** @brief Decompose a trafo matrix with no scaling into its 
	 *    original components
	 *  @param rotation Receives the output rotation as a hamilton
	 *    quaternion 
	 *  @param position Receives the output position for the x,y,z axes
	 */
	void DecomposeNoScaling (aiQuaternion& rotation,
		aiVector3D& position) const;


	// -------------------------------------------------------------------
	/** @brief Creates a trafo matrix from a set of euler angles
	 *  @param x Rotation angle for the x-axis, in radians
	 *  @param y Rotation angle for the y-axis, in radians
	 *  @param z Rotation angle for the z-axis, in radians
	 */
	aiMatrix4x4& FromEulerAnglesXYZ(float x, float y, float z);
	aiMatrix4x4& FromEulerAnglesXYZ(const aiVector3D& blubb);

public:
	// -------------------------------------------------------------------
	/** @brief Returns a rotation matrix for a rotation around the x axis
	 *  @param a Rotation angle, in radians
	 *  @param out Receives the output matrix
	 *  @return Reference to the output matrix
	 */
	static aiMatrix4x4& RotationX(float a, aiMatrix4x4& out);

	// -------------------------------------------------------------------
	/** @brief Returns a rotation matrix for a rotation around the y axis
	 *  @param a Rotation angle, in radians
	 *  @param out Receives the output matrix
	 *  @return Reference to the output matrix
	 */
	static aiMatrix4x4& RotationY(float a, aiMatrix4x4& out);

	// -------------------------------------------------------------------
	/** @brief Returns a rotation matrix for a rotation around the z axis
	 *  @param a Rotation angle, in radians
	 *  @param out Receives the output matrix
	 *  @return Reference to the output matrix
	 */
	static aiMatrix4x4& RotationZ(float a, aiMatrix4x4& out);

	// -------------------------------------------------------------------
	/** Returns a rotation matrix for a rotation around an arbitrary axis.
	 *  @param a Rotation angle, in radians
	 *  @param axis Rotation axis, should be a normalized vector.
	 *  @param out Receives the output matrix
	 *  @return Reference to the output matrix
	 */
	static aiMatrix4x4& Rotation(float a, const aiVector3D& axis, 
		aiMatrix4x4& out);

	// -------------------------------------------------------------------
	/** @brief Returns a translation matrix 
	 *  @param v Translation vector
	 *  @param out Receives the output matrix
	 *  @return Reference to the output matrix
	 */
	static aiMatrix4x4& Translation( const aiVector3D& v, aiMatrix4x4& out);

	// -------------------------------------------------------------------
	/** @brief Returns a scaling matrix 
	 *  @param v Scaling vector
	 *  @param out Receives the output matrix
	 *  @return Reference to the output matrix
	 */
	static aiMatrix4x4& Scaling( const aiVector3D& v, aiMatrix4x4& out);

	// -------------------------------------------------------------------
	/** @brief A function for creating a rotation matrix that rotates a
	 *  vector called "from" into another vector called "to".
	 * Input : from[3], to[3] which both must be *normalized* non-zero vectors
	 * Output: mtx[3][3] -- a 3x3 matrix in colum-major form
	 * Authors: Tomas Möller, John Hughes
	 *          "Efficiently Building a Matrix to Rotate One Vector to Another"
	 *          Journal of Graphics Tools, 4(4):1-4, 1999
	 */
	static aiMatrix4x4& FromToMatrix(const aiVector3D& from, 
		const aiVector3D& to, aiMatrix4x4& out);

#endif // __cplusplus

	float a1, a2, a3, a4;
	float b1, b2, b3, b4;
	float c1, c2, c3, c4;
	float d1, d2, d3, d4;

} PACK_STRUCT; // !class aiMatrix4x4


#include "./Compiler/poppack1.h"

#ifdef __cplusplus
} // end extern "C"


#endif // __cplusplus
#endif // AI_MATRIX4X4_H_INC
