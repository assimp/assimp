/** @file Quaternion structure, including operators when compiling in C++ */
#ifndef AI_QUATERNION_H_INC
#define AI_QUATERNION_H_INC

#include <math.h>
#include "aiTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
/** Represents a quaternion in a 4D vector. */
struct aiQuaternion
{
#ifdef __cplusplus
	aiQuaternion() : w(0.0f), x(0.0f), y(0.0f), z(0.0f) {}
	aiQuaternion(float _w, float _x, float _y, float _z) : w(_w), x(_x), y(_y), z(_z) {}
	/** Construct from rotation matrix. Result is undefined if the matrix is not orthonormal. */
	aiQuaternion( const aiMatrix3x3& pRotMatrix);

	/** Returns a matrix representation of the quaternion */
	aiMatrix3x3 GetMatrix() const;
#endif // __cplusplus

	//! w,x,y,z components of the quaternion
	float w, x, y, z;	
} ;


#ifdef __cplusplus

// ---------------------------------------------------------------------------
// Constructs a quaternion from a rotation matrix
inline aiQuaternion::aiQuaternion( const aiMatrix3x3 &pRotMatrix)
{
	float t = 1 + pRotMatrix.a1 + pRotMatrix.b2 + pRotMatrix.c3;

	// large enough
	if( t > 0.00001f)
	{
		float s = sqrt( t) * 2.0f;
		x = (pRotMatrix.b3 - pRotMatrix.c2) / s;
		y = (pRotMatrix.c1 - pRotMatrix.a3) / s;
		z = (pRotMatrix.a2 - pRotMatrix.b1) / s;
		w = 0.25f * s;
	} // else we have to check several cases
	else if( pRotMatrix.a1 > pRotMatrix.b2 && pRotMatrix.a1 > pRotMatrix.c3 )  
	{	
		// Column 0: 
		float s = sqrt( 1.0f + pRotMatrix.a1 - pRotMatrix.b2 - pRotMatrix.c3) * 2.0f;
		x = -0.25f * s;
		y = (pRotMatrix.a2 + pRotMatrix.b1) / s;
		z = (pRotMatrix.c1 + pRotMatrix.a3) / s;
		w = (pRotMatrix.c2 - pRotMatrix.b3) / s;
	} 
	else if( pRotMatrix.b2 > pRotMatrix.c3) 
	{ 
		// Column 1: 
		float s = sqrt( 1.0f + pRotMatrix.b2 - pRotMatrix.a1 - pRotMatrix.c3) * 2.0f;
		x = (pRotMatrix.a2 + pRotMatrix.b1) / s;
		y = -0.25f * s;
		z = (pRotMatrix.b3 + pRotMatrix.c2) / s;
		w = (pRotMatrix.a3 - pRotMatrix.c1) / s;
	} else 
	{ 
		// Column 2:
		float s = sqrt( 1.0f + pRotMatrix.c3 - pRotMatrix.a1 - pRotMatrix.b2) * 2.0f;
		x = (pRotMatrix.c1 + pRotMatrix.a3) / s;
		y = (pRotMatrix.b3 + pRotMatrix.c2) / s;
		z = -0.25f * s;
		w = (pRotMatrix.b1 - pRotMatrix.a2) / s;
	}
}

// ---------------------------------------------------------------------------
// Returns a matrix representation of the quaternion
inline aiMatrix3x3 aiQuaternion::GetMatrix() const
{
	aiMatrix3x3 resMatrix;
	resMatrix.a1 = 1.0f - 2.0f * (y * y + z * z);
	resMatrix.a2 = 2.0f * (x * y + z * w);
	resMatrix.a3 = 2.0f * (x * z - y * w);
	resMatrix.b1 = 2.0f * (x * y - z * w);
	resMatrix.b2 = 1.0f - 2.0f * (x * x + z * z);
	resMatrix.b3 = 2.0f * (y * z + x * w);
	resMatrix.c1 = 2.0f * (x * z + y * w);
	resMatrix.c2 = 2.0f * (y * z - x * w);
	resMatrix.c3 = 1.0f - 2.0f * (x * x + y * y);

	return resMatrix;
}

} // end extern "C"
#endif // __cplusplus

#endif // AI_QUATERNION_H_INC