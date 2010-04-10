/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team
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

/** @file aiQuaternion.h
 *  @brief Quaternion structure, including operators when compiling in C++
 */
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

	/** Construct from euler angles */
	aiQuaternion( float rotx, float roty, float rotz);

	/** Construct from an axis-angle pair */
	aiQuaternion( aiVector3D axis, float angle);

	/** Construct from a normalized quaternion stored in a vec3 */
	aiQuaternion( aiVector3D normalized);

	/** Returns a matrix representation of the quaternion */
	aiMatrix3x3 GetMatrix() const;


	bool operator== (const aiQuaternion& o) const
		{return x == o.x && y == o.y && z == o.z && w == o.w;}

	bool operator!= (const aiQuaternion& o) const
		{return !(*this == o);}

	/** Normalize the quaternion */
	aiQuaternion& Normalize();

	/** Compute quaternion conjugate */
	aiQuaternion& Conjugate ();

	/** Rotate a point by this quaternion */
	aiVector3D Rotate (const aiVector3D& in);

	/** Multiply two quaternions */
	aiQuaternion operator* (const aiQuaternion& two) const;

	/** Performs a spherical interpolation between two quaternions and writes the result into the third.
	 * @param pOut Target object to received the interpolated rotation.
	 * @param pStart Start rotation of the interpolation at factor == 0.
	 * @param pEnd End rotation, factor == 1.
	 * @param pFactor Interpolation factor between 0 and 1. Values outside of this range yield undefined results.
	 */
	static void Interpolate( aiQuaternion& pOut, const aiQuaternion& pStart, const aiQuaternion& pEnd, float pFactor);

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
	if( t > 0.001f)
	{
		float s = sqrt( t) * 2.0f;
		x = (pRotMatrix.c2 - pRotMatrix.b3) / s;
		y = (pRotMatrix.a3 - pRotMatrix.c1) / s;
		z = (pRotMatrix.b1 - pRotMatrix.a2) / s;
		w = 0.25f * s;
	} // else we have to check several cases
	else if( pRotMatrix.a1 > pRotMatrix.b2 && pRotMatrix.a1 > pRotMatrix.c3 )  
	{	
    // Column 0: 
		float s = sqrt( 1.0f + pRotMatrix.a1 - pRotMatrix.b2 - pRotMatrix.c3) * 2.0f;
		x = 0.25f * s;
		y = (pRotMatrix.b1 + pRotMatrix.a2) / s;
		z = (pRotMatrix.a3 + pRotMatrix.c1) / s;
		w = (pRotMatrix.c2 - pRotMatrix.b3) / s;
	} 
	else if( pRotMatrix.b2 > pRotMatrix.c3) 
	{ 
    // Column 1: 
		float s = sqrt( 1.0f + pRotMatrix.b2 - pRotMatrix.a1 - pRotMatrix.c3) * 2.0f;
		x = (pRotMatrix.b1 + pRotMatrix.a2) / s;
		y = 0.25f * s;
		z = (pRotMatrix.c2 + pRotMatrix.b3) / s;
		w = (pRotMatrix.a3 - pRotMatrix.c1) / s;
	} else 
	{ 
    // Column 2:
		float s = sqrt( 1.0f + pRotMatrix.c3 - pRotMatrix.a1 - pRotMatrix.b2) * 2.0f;
		x = (pRotMatrix.a3 + pRotMatrix.c1) / s;
		y = (pRotMatrix.c2 + pRotMatrix.b3) / s;
		z = 0.25f * s;
		w = (pRotMatrix.b1 - pRotMatrix.a2) / s;
	}
}

// ---------------------------------------------------------------------------
// Construction from euler angles
inline aiQuaternion::aiQuaternion( float fPitch, float fYaw, float fRoll )
{
	const float fSinPitch(sin(fPitch*0.5F));
	const float fCosPitch(cos(fPitch*0.5F));
	const float fSinYaw(sin(fYaw*0.5F));
	const float fCosYaw(cos(fYaw*0.5F));
	const float fSinRoll(sin(fRoll*0.5F));
	const float fCosRoll(cos(fRoll*0.5F));
	const float fCosPitchCosYaw(fCosPitch*fCosYaw);
	const float fSinPitchSinYaw(fSinPitch*fSinYaw);
	x = fSinRoll * fCosPitchCosYaw     - fCosRoll * fSinPitchSinYaw;
	y = fCosRoll * fSinPitch * fCosYaw + fSinRoll * fCosPitch * fSinYaw;
	z = fCosRoll * fCosPitch * fSinYaw - fSinRoll * fSinPitch * fCosYaw;
	w = fCosRoll * fCosPitchCosYaw     + fSinRoll * fSinPitchSinYaw;
}

// ---------------------------------------------------------------------------
// Returns a matrix representation of the quaternion
inline aiMatrix3x3 aiQuaternion::GetMatrix() const
{
	aiMatrix3x3 resMatrix;
	resMatrix.a1 = 1.0f - 2.0f * (y * y + z * z);
	resMatrix.a2 = 2.0f * (x * y - z * w);
	resMatrix.a3 = 2.0f * (x * z + y * w);
	resMatrix.b1 = 2.0f * (x * y + z * w);
	resMatrix.b2 = 1.0f - 2.0f * (x * x + z * z);
	resMatrix.b3 = 2.0f * (y * z - x * w);
	resMatrix.c1 = 2.0f * (x * z - y * w);
	resMatrix.c2 = 2.0f * (y * z + x * w);
	resMatrix.c3 = 1.0f - 2.0f * (x * x + y * y);

	return resMatrix;
}

// ---------------------------------------------------------------------------
// Construction from an axis-angle pair
inline aiQuaternion::aiQuaternion( aiVector3D axis, float angle)
{
	axis.Normalize();

	const float sin_a = sin( angle / 2 );
    const float cos_a = cos( angle / 2 );
    x    = axis.x * sin_a;
    y    = axis.y * sin_a;
    z    = axis.z * sin_a;
    w    = cos_a;
}
// ---------------------------------------------------------------------------
// Construction from am existing, normalized quaternion
inline aiQuaternion::aiQuaternion( aiVector3D normalized)
{
	x = normalized.x;
	y = normalized.y;
	z = normalized.z;

	const float t = 1.0f - (x*x) - (y*y) - (z*z);

	if (t < 0.0f)
		w = 0.0f;
	else w = sqrt (t);
}

// ---------------------------------------------------------------------------
// Performs a spherical interpolation between two quaternions 
// Implementation adopted from the gmtl project. All others I found on the net fail in some cases.
// Congrats, gmtl!
inline void aiQuaternion::Interpolate( aiQuaternion& pOut, const aiQuaternion& pStart, const aiQuaternion& pEnd, float pFactor)
{
  // calc cosine theta
  float cosom = pStart.x * pEnd.x + pStart.y * pEnd.y + pStart.z * pEnd.z + pStart.w * pEnd.w;

  // adjust signs (if necessary)
  aiQuaternion end = pEnd;
  if( cosom < 0.0f)
  {
    cosom = -cosom;
    end.x = -end.x;   // Reverse all signs
    end.y = -end.y;
    end.z = -end.z;
    end.w = -end.w;
  } 

  // Calculate coefficients
  float sclp, sclq;
  if( (1.0f - cosom) > 0.0001f) // 0.0001 -> some epsillon
  {
    // Standard case (slerp)
    float omega, sinom;
    omega = acos( cosom); // extract theta from dot product's cos theta
    sinom = sin( omega);
    sclp  = sin( (1.0f - pFactor) * omega) / sinom;
    sclq  = sin( pFactor * omega) / sinom;
  } else
  {
    // Very close, do linear interp (because it's faster)
    sclp = 1.0f - pFactor;
    sclq = pFactor;
  }

  pOut.x = sclp * pStart.x + sclq * end.x;
  pOut.y = sclp * pStart.y + sclq * end.y;
  pOut.z = sclp * pStart.z + sclq * end.z;
  pOut.w = sclp * pStart.w + sclq * end.w;
}

// ---------------------------------------------------------------------------
inline aiQuaternion& aiQuaternion::Normalize()
{
	// compute the magnitude and divide through it
	const float mag = x*x+y*y+z*z+w*w;
	if (mag)
	{
		x /= mag;
		y /= mag;
		z /= mag;
		w /= mag;
	}
	return *this;
}

// ---------------------------------------------------------------------------
inline aiQuaternion aiQuaternion::operator* (const aiQuaternion& t) const
{
	return aiQuaternion(w*t.w - x*t.x - y*t.y - z*t.z,
		w*t.x + x*t.w + y*t.z - z*t.y,
		w*t.y + y*t.w + z*t.x - x*t.z,
		w*t.z + z*t.w + x*t.y - y*t.x);
}

// ---------------------------------------------------------------------------
inline aiQuaternion& aiQuaternion::Conjugate ()
{
	x = -x;
	y = -y;
	z = -z;
	return *this;
}

// ---------------------------------------------------------------------------
inline aiVector3D aiQuaternion::Rotate (const aiVector3D& v)
{
	aiQuaternion q2(0.f,v.x,v.y,v.z), q = *this, qinv = q;
	q.Conjugate();

	q = q*q2*qinv;
	return aiVector3D(q.x,q.y,q.z);

}

} // end extern "C"

#endif // __cplusplus

#endif // AI_QUATERNION_H_INC
