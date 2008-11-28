/** @file Inline implementation of the 4x4 matrix operators */
#ifndef AI_MATRIX4x4_INL_INC
#define AI_MATRIX4x4_INL_INC

#include "aiMatrix4x4.h"

#ifdef __cplusplus
#include "aiMatrix3x3.h"

#include <algorithm>
#include <limits>
#include <math.h>

#include "aiAssert.h"
#include "aiQuaternion.h"

// ---------------------------------------------------------------------------
inline aiMatrix4x4::aiMatrix4x4( const aiMatrix3x3& m)
{
	a1 = m.a1; a2 = m.a2; a3 = m.a3; a4 = 0.0f;
	b1 = m.b1; b2 = m.b2; b3 = m.b3; b4 = 0.0f;
	c1 = m.c1; c2 = m.c2; c3 = m.c3; c4 = 0.0f;
	d1 = 0.0f; d2 = 0.0f; d3 = 0.0f; d4 = 1.0f;
}

// ---------------------------------------------------------------------------
inline aiMatrix4x4& aiMatrix4x4::operator *= (const aiMatrix4x4& m)
{
	*this = aiMatrix4x4(
		m.a1 * a1 + m.b1 * a2 + m.c1 * a3 + m.d1 * a4,
		m.a2 * a1 + m.b2 * a2 + m.c2 * a3 + m.d2 * a4,
		m.a3 * a1 + m.b3 * a2 + m.c3 * a3 + m.d3 * a4,
		m.a4 * a1 + m.b4 * a2 + m.c4 * a3 + m.d4 * a4,
		m.a1 * b1 + m.b1 * b2 + m.c1 * b3 + m.d1 * b4,
		m.a2 * b1 + m.b2 * b2 + m.c2 * b3 + m.d2 * b4,
		m.a3 * b1 + m.b3 * b2 + m.c3 * b3 + m.d3 * b4,
		m.a4 * b1 + m.b4 * b2 + m.c4 * b3 + m.d4 * b4,
		m.a1 * c1 + m.b1 * c2 + m.c1 * c3 + m.d1 * c4,
		m.a2 * c1 + m.b2 * c2 + m.c2 * c3 + m.d2 * c4,
		m.a3 * c1 + m.b3 * c2 + m.c3 * c3 + m.d3 * c4,
		m.a4 * c1 + m.b4 * c2 + m.c4 * c3 + m.d4 * c4,
		m.a1 * d1 + m.b1 * d2 + m.c1 * d3 + m.d1 * d4,
		m.a2 * d1 + m.b2 * d2 + m.c2 * d3 + m.d2 * d4,
		m.a3 * d1 + m.b3 * d2 + m.c3 * d3 + m.d3 * d4,
		m.a4 * d1 + m.b4 * d2 + m.c4 * d3 + m.d4 * d4);
	return *this;
}

// ---------------------------------------------------------------------------
inline aiMatrix4x4 aiMatrix4x4::operator* (const aiMatrix4x4& m) const
{
	aiMatrix4x4 temp( *this);
	temp *= m;
	return temp;
}


// ---------------------------------------------------------------------------
inline aiMatrix4x4& aiMatrix4x4::Transpose()
{
	// (float&) don't remove, GCC complains cause of packed fields
	std::swap( (float&)b1, (float&)a2);
	std::swap( (float&)c1, (float&)a3);
	std::swap( (float&)c2, (float&)b3);
	std::swap( (float&)d1, (float&)a4);
	std::swap( (float&)d2, (float&)b4);
	std::swap( (float&)d3, (float&)c4);
	return *this;
}


// ---------------------------------------------------------------------------
inline float aiMatrix4x4::Determinant() const
{
	return a1*b2*c3*d4 - a1*b2*c4*d3 + a1*b3*c4*d2 - a1*b3*c2*d4 
		+ a1*b4*c2*d3 - a1*b4*c3*d2 - a2*b3*c4*d1 + a2*b3*c1*d4 
		- a2*b4*c1*d3 + a2*b4*c3*d1 - a2*b1*c3*d4 + a2*b1*c4*d3 
		+ a3*b4*c1*d2 - a3*b4*c2*d1 + a3*b1*c2*d4 - a3*b1*c4*d2 
		+ a3*b2*c4*d1 - a3*b2*c1*d4 - a4*b1*c2*d3 + a4*b1*c3*d2
		- a4*b2*c3*d1 + a4*b2*c1*d3 - a4*b3*c1*d2 + a4*b3*c2*d1;
}

// ---------------------------------------------------------------------------
inline aiMatrix4x4& aiMatrix4x4::Inverse()
{
	// Compute the reciprocal determinant
	float det = Determinant();
	if(det == 0.0f) 
	{
		// Matrix not invertible. Setting all elements to nan is not really
		// correct in a mathematical sense but it is easy to debug for the
		// programmer.
		const float nan = std::numeric_limits<float>::quiet_NaN();
		*this = aiMatrix4x4(
			nan,nan,nan,nan,
			nan,nan,nan,nan,
			nan,nan,nan,nan,
			nan,nan,nan,nan);

		return *this;
	}

	float invdet = 1.0f / det;

	aiMatrix4x4 res;
	res.a1 = invdet  * (b2 * (c3 * d4 - c4 * d3) + b3 * (c4 * d2 - c2 * d4) + b4 * (c2 * d3 - c3 * d2));
	res.a2 = -invdet * (a2 * (c3 * d4 - c4 * d3) + a3 * (c4 * d2 - c2 * d4) + a4 * (c2 * d3 - c3 * d2));
	res.a3 = invdet  * (a2 * (b3 * d4 - b4 * d3) + a3 * (b4 * d2 - b2 * d4) + a4 * (b2 * d3 - b3 * d2));
	res.a4 = -invdet * (a2 * (b3 * c4 - b4 * c3) + a3 * (b4 * c2 - b2 * c4) + a4 * (b2 * c3 - b3 * c2));
	res.b1 = -invdet * (b1 * (c3 * d4 - c4 * d3) + b3 * (c4 * d1 - c1 * d4) + b4 * (c1 * d3 - c3 * d1));
	res.b2 = invdet  * (a1 * (c3 * d4 - c4 * d3) + a3 * (c4 * d1 - c1 * d4) + a4 * (c1 * d3 - c3 * d1));
	res.b3 = -invdet * (a1 * (b3 * d4 - b4 * d3) + a3 * (b4 * d1 - b1 * d4) + a4 * (b1 * d3 - b3 * d1));
	res.b4 = invdet  * (a1 * (b3 * c4 - b4 * c3) + a3 * (b4 * c1 - b1 * c4) + a4 * (b1 * c3 - b3 * c1));
	res.c1 = invdet  * (b1 * (c2 * d4 - c4 * d2) + b2 * (c4 * d1 - c1 * d4) + b4 * (c1 * d2 - c2 * d1));
	res.c2 = -invdet * (a1 * (c2 * d4 - c4 * d2) + a2 * (c4 * d1 - c1 * d4) + a4 * (c1 * d2 - c2 * d1));
	res.c3 = invdet  * (a1 * (b2 * d4 - b4 * d2) + a2 * (b4 * d1 - b1 * d4) + a4 * (b1 * d2 - b2 * d1));
	res.c4 = -invdet * (a1 * (b2 * c4 - b4 * c2) + a2 * (b4 * c1 - b1 * c4) + a4 * (b1 * c2 - b2 * c1));
	res.d1 = -invdet * (b1 * (c2 * d3 - c3 * d2) + b2 * (c3 * d1 - c1 * d3) + b3 * (c1 * d2 - c2 * d1));
	res.d2 = invdet  * (a1 * (c2 * d3 - c3 * d2) + a2 * (c3 * d1 - c1 * d3) + a3 * (c1 * d2 - c2 * d1));
	res.d3 = -invdet * (a1 * (b2 * d3 - b3 * d2) + a2 * (b3 * d1 - b1 * d3) + a3 * (b1 * d2 - b2 * d1));
	res.d4 = invdet  * (a1 * (b2 * c3 - b3 * c2) + a2 * (b3 * c1 - b1 * c3) + a3 * (b1 * c2 - b2 * c1)); 
	*this = res;

	return *this;
}

// ---------------------------------------------------------------------------
inline float* aiMatrix4x4::operator[](unsigned int p_iIndex)
{
	return &this->a1 + p_iIndex * 4;
}

// ---------------------------------------------------------------------------
inline const float* aiMatrix4x4::operator[](unsigned int p_iIndex) const
{
	return &this->a1 + p_iIndex * 4;
}
// ---------------------------------------------------------------------------
inline bool aiMatrix4x4::operator== (const aiMatrix4x4 m) const
{
	return (a1 == m.a1 && a2 == m.a2 && a3 == m.a3 && a4 == m.a4 &&
		b1 == m.b1 && b2 == m.b2 && b3 == m.b3 && b4 == m.b4 &&
		c1 == m.c1 && c2 == m.c2 && c3 == m.c3 && c4 == m.c4 &&
		d1 == m.d1 && d2 == m.d2 && d3 == m.d3 && d4 == m.d4);
}
// ---------------------------------------------------------------------------
inline bool aiMatrix4x4::operator!= (const aiMatrix4x4 m) const
{
	return !(*this == m);
}
// ---------------------------------------------------------------------------
inline void aiMatrix4x4::Decompose (aiVector3D& scaling, aiQuaternion& rotation,
	aiVector3D& position) const
{
	const aiMatrix4x4& _this = *this;

	// extract translation
	position.x = _this[0][3];
	position.y = _this[1][3];
	position.z = _this[2][3];

	// extract the rows of the matrix
	aiVector3D vRows[3] = {
		aiVector3D(_this[0][0],_this[1][0],_this[2][0]),
		aiVector3D(_this[0][1],_this[1][1],_this[2][1]),
		aiVector3D(_this[0][2],_this[1][2],_this[2][2])
	};

	// extract the scaling factors
	scaling.x = vRows[0].Length();
	scaling.y = vRows[1].Length();
	scaling.z = vRows[2].Length();

	// and remove all scaling from the matrix
	if(scaling.x)
	{
		vRows[0] /= scaling.x;
	}
	if(scaling.y)
	{
		vRows[1] /= scaling.y;
	}
	if(scaling.z)
	{
		vRows[2] /= scaling.z;
	}

	// build a 3x3 rotation matrix
	aiMatrix3x3 m(vRows[0].x,vRows[1].x,vRows[2].x,
		vRows[0].y,vRows[1].y,vRows[2].y,
		vRows[0].z,vRows[1].z,vRows[2].z);

	// and generate the rotation quaternion from it
	rotation = aiQuaternion(m);
}
// ---------------------------------------------------------------------------
inline void aiMatrix4x4::DecomposeNoScaling (aiQuaternion& rotation,
	aiVector3D& position) const
{
	const aiMatrix4x4& _this = *this;

	// extract translation
	position.x = _this[0][3];
	position.y = _this[1][3];
	position.z = _this[2][3];

	// extract rotation
	rotation = aiQuaternion((aiMatrix3x3)_this);
}
// ---------------------------------------------------------------------------
inline void aiMatrix4x4::FromEulerAnglesXYZ(const aiVector3D& blubb)
{
	FromEulerAnglesXYZ(blubb.x,blubb.y,blubb.z);
}
// ---------------------------------------------------------------------------
inline void aiMatrix4x4::FromEulerAnglesXYZ(float x, float y, float z)
{
	aiMatrix4x4& _this = *this;

	float cr = cos( x );
	float sr = sin( x );
	float cp = cos( y );
	float sp = sin( y );
	float cy = cos( z );
	float sy = sin( z );

	_this.a1 = cp*cy ;
	_this.a2 = cp*sy;
	_this.a3 = -sp ;

	float srsp = sr*sp;
	float crsp = cr*sp;

	_this.b1 = srsp*cy-cr*sy ;
	_this.b2 = srsp*sy+cr*cy ;
	_this.b3 = sr*cp ;

	_this.c1 =  crsp*cy+sr*sy ;
	_this.c2 =  crsp*sy-sr*cy ;
	_this.c3 = cr*cp ;

}
// ---------------------------------------------------------------------------
inline bool aiMatrix4x4::IsIdentity() const
{
	// Use a small epsilon to solve floating-point inaccuracies
	const static float epsilon = 10e-3f;

	return (a2 <= epsilon && a2 >= -epsilon &&
			a3 <= epsilon && a3 >= -epsilon &&
			a4 <= epsilon && a4 >= -epsilon &&
			b1 <= epsilon && b1 >= -epsilon &&
			b3 <= epsilon && b3 >= -epsilon &&
			b4 <= epsilon && b4 >= -epsilon &&
			c1 <= epsilon && c1 >= -epsilon &&
			c2 <= epsilon && c2 >= -epsilon &&
			c3 <= epsilon && c3 >= -epsilon &&
			d1 <= epsilon && d1 >= -epsilon &&
			d2 <= epsilon && d2 >= -epsilon &&
			d3 <= epsilon && d3 >= -epsilon &&
			a1 <= 1.f+epsilon && a1 >= 1.f-epsilon && 
			b2 <= 1.f+epsilon && b2 >= 1.f-epsilon && 
			c3 <= 1.f+epsilon && c3 >= 1.f-epsilon && 
			d4 <= 1.f+epsilon && d4 >= 1.f-epsilon);
}

// ---------------------------------------------------------------------------
inline aiMatrix4x4& aiMatrix4x4::RotationX(float a, aiMatrix4x4& out)
{
	/*
	     |  1  0       0       0 |
     M = |  0  cos(A) -sin(A)  0 |
         |  0  sin(A)  cos(A)  0 |
         |  0  0       0       1 |	*/
	out = aiMatrix4x4();
	out.b2 = out.c3 = cos(a);
	out.b3 = -(out.c2 = sin(a));
	return out;
}

// ---------------------------------------------------------------------------
inline aiMatrix4x4& aiMatrix4x4::RotationY(float a, aiMatrix4x4& out)
{
	/*
	     |  cos(A)  0   sin(A)  0 |
     M = |  0       1   0       0 |
         | -sin(A)  0   cos(A)  0 |
         |  0       0   0       1 |
		*/
	out = aiMatrix4x4();
	out.a1 = out.c3 = cos(a);
	out.c1 = -(out.a3 = sin(a));
	return out;
}

// ---------------------------------------------------------------------------
inline aiMatrix4x4& aiMatrix4x4::RotationZ(float a, aiMatrix4x4& out)
{
	/*
	     |  cos(A)  -sin(A)   0   0 |
     M = |  sin(A)   cos(A)   0   0 |
         |  0        0        1   0 |
         |  0        0        0   1 |	*/
	out = aiMatrix4x4();
	out.a1 = out.b2 = cos(a);
	out.a2 = -(out.b1 = sin(a));
	return out;
}

// ---------------------------------------------------------------------------
// Returns a rotation matrix for a rotation around an arbitrary axis.
inline aiMatrix4x4& aiMatrix4x4::Rotation( float a, const aiVector3D& axis, aiMatrix4x4& out)
{
  float c = cos( a), s = sin( a), t = 1 - c;
  float x = axis.x, y = axis.y, z = axis.z;

  // Many thanks to MathWorld and Wikipedia
  out.a1 = t*x*x + c;   out.a2 = t*x*y - s*z; out.a3 = t*x*z + s*y;
  out.b1 = t*x*y + s*z; out.b2 = t*y*y + c;   out.b3 = t*y*z - s*x;
  out.c1 = t*x*z - s*y; out.c2 = t*y*z + s*x; out.c3 = t*z*z + c;
  out.a4 = out.b4 = out.c4 = 0.0f;
  out.d1 = out.d2 = out.d3 = 0.0f;
  out.d4 = 1.0f;

  return out;
}

// ---------------------------------------------------------------------------
inline aiMatrix4x4& aiMatrix4x4::Translation( const aiVector3D& v, aiMatrix4x4& out)
{
	out = aiMatrix4x4();
	out.a4 = v.x;
	out.b4 = v.y;
	out.c4 = v.z;
	return out;
}

// ---------------------------------------------------------------------------
/** A function for creating a rotation matrix that rotates a vector called
 * "from" into another vector called "to".
 * Input : from[3], to[3] which both must be *normalized* non-zero vectors
 * Output: mtx[3][3] -- a 3x3 matrix in colum-major form
 * Authors: Tomas Möller, John Hughes
 *          "Efficiently Building a Matrix to Rotate One Vector to Another"
 *          Journal of Graphics Tools, 4(4):1-4, 1999
 */
// ---------------------------------------------------------------------------
inline aiMatrix4x4& aiMatrix4x4::FromToMatrix(const aiVector3D& from, 
	const aiVector3D& to, aiMatrix4x4& mtx)
{
	const aiVector3D v = from ^ to;
	const float e = from * to;
	const float f = (e < 0)? -e:e;

	if (f > 1.0 - 0.00001f)     /* "from" and "to"-vector almost parallel */
	{
		aiVector3D u,v;     /* temporary storage vectors */
		aiVector3D x;       /* vector most nearly orthogonal to "from" */

		x.x = (from.x > 0.0)? from.x : -from.x;
		x.y = (from.y > 0.0)? from.y : -from.y;
		x.z = (from.z > 0.0)? from.z : -from.z;

		if (x.x < x.y)
		{
			if (x.x < x.z)
			{
				x.x = 1.0; x.y = x.z = 0.0;
			}
			else
			{
				x.z = 1.0; x.y = x.z = 0.0;
			}
		}
		else
		{
			if (x.y < x.z)
			{
				x.y = 1.0; x.x = x.z = 0.0;
			}
			else
			{
				x.z = 1.0; x.x = x.y = 0.0;
			}
		}

		u.x = x.x - from.x; u.y = x.y - from.y; u.z = x.z - from.z;
		v.x = x.x - to.x;   v.y = x.y - to.y;   v.z = x.z - to.z;

		const float c1 = 2.0f / (u * u);
		const float c2 = 2.0f / (v * v);
		const float c3 = c1 * c2  * (u * v);

		for (unsigned int i = 0; i < 3; i++) 
		{
			for (unsigned int j = 0; j < 3; j++) 
			{
				mtx[i][j] =  - c1 * u[i] * u[j] - c2 * v[i] * v[j]
					+ c3 * v[i] * u[j];
			}
			mtx[i][i] += 1.0;
		}
	}
	else  /* the most common case, unless "from"="to", or "from"=-"to" */
	{
		/* ... use this hand optimized version (9 mults less) */
		const float h = 1.0f/(1.0f + e);      /* optimization by Gottfried Chen */
		const float hvx = h * v.x;
		const float hvz = h * v.z;
		const float hvxy = hvx * v.y;
		const float hvxz = hvx * v.z;
		const float hvyz = hvz * v.y;
		mtx[0][0] = e + hvx * v.x;
		mtx[0][1] = hvxy - v.z;
		mtx[0][2] = hvxz + v.y;

		mtx[1][0] = hvxy + v.z;
		mtx[1][1] = e + h * v.y * v.y;
		mtx[1][2] = hvyz - v.x;

		mtx[2][0] = hvxz - v.y;
		mtx[2][1] = hvyz + v.x;
		mtx[2][2] = e + hvz * v.z;
	}
	return mtx;
}

#endif // __cplusplus
#endif // AI_MATRIX4x4_INL_INC
