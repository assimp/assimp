/** @file aiMatrix3x3.inl
 *  @brief Inline implementation of the 3x3 matrix operators
 */
#ifndef AI_MATRIX3x3_INL_INC
#define AI_MATRIX3x3_INL_INC

#include "aiMatrix3x3.h"

#ifdef __cplusplus
#include "aiMatrix4x4.h"
#include <algorithm>
#include <limits>

// ------------------------------------------------------------------------------------------------
// Construction from a 4x4 matrix. The remaining parts of the matrix are ignored.
inline aiMatrix3x3::aiMatrix3x3( const aiMatrix4x4& pMatrix)
{
	a1 = pMatrix.a1; a2 = pMatrix.a2; a3 = pMatrix.a3;
	b1 = pMatrix.b1; b2 = pMatrix.b2; b3 = pMatrix.b3;
	c1 = pMatrix.c1; c2 = pMatrix.c2; c3 = pMatrix.c3;
}

// ------------------------------------------------------------------------------------------------
inline aiMatrix3x3& aiMatrix3x3::operator *= (const aiMatrix3x3& m)
{
	*this = aiMatrix3x3(m.a1 * a1 + m.b1 * a2 + m.c1 * a3,
		m.a2 * a1 + m.b2 * a2 + m.c2 * a3,
		m.a3 * a1 + m.b3 * a2 + m.c3 * a3,
		m.a1 * b1 + m.b1 * b2 + m.c1 * b3,
		m.a2 * b1 + m.b2 * b2 + m.c2 * b3,
		m.a3 * b1 + m.b3 * b2 + m.c3 * b3,
		m.a1 * c1 + m.b1 * c2 + m.c1 * c3,
		m.a2 * c1 + m.b2 * c2 + m.c2 * c3,
		m.a3 * c1 + m.b3 * c2 + m.c3 * c3);
	return *this;
}

// ------------------------------------------------------------------------------------------------
inline aiMatrix3x3 aiMatrix3x3::operator* (const aiMatrix3x3& m) const
{
	aiMatrix3x3 temp( *this);
	temp *= m;
	return temp;
}

// ------------------------------------------------------------------------------------------------
inline float* aiMatrix3x3::operator[] (unsigned int p_iIndex)
{
	return &this->a1 + p_iIndex * 3;
}

// ------------------------------------------------------------------------------------------------
inline const float* aiMatrix3x3::operator[] (unsigned int p_iIndex) const
{
	return &this->a1 + p_iIndex * 3;
}

// ------------------------------------------------------------------------------------------------
inline bool aiMatrix3x3::operator== (const aiMatrix4x4 m) const
{
	return a1 == m.a1 && a2 == m.a2 && a3 == m.a3 &&
		   b1 == m.b1 && b2 == m.b2 && b3 == m.b3 &&
		   c1 == m.c1 && c2 == m.c2 && c3 == m.c3;
}

// ------------------------------------------------------------------------------------------------
inline bool aiMatrix3x3::operator!= (const aiMatrix4x4 m) const
{
	return !(*this == m);
}

// ------------------------------------------------------------------------------------------------
inline aiMatrix3x3& aiMatrix3x3::Transpose()
{
	// (float&) don't remove, GCC complains cause of packed fields
	std::swap( (float&)a2, (float&)b1);
	std::swap( (float&)a3, (float&)c1);
	std::swap( (float&)b3, (float&)c2);
	return *this;
}

// ----------------------------------------------------------------------------------------
inline float aiMatrix3x3::Determinant() const
{
	return a1*b2*c3 - a1*b3*c2 + a2*b3*c1 - a2*b1*c3 + a3*b1*c2 - a3*b2*c1;
}

// ----------------------------------------------------------------------------------------
inline aiMatrix3x3& aiMatrix3x3::Inverse()
{
	// Compute the reciprocal determinant
	float det = Determinant();
	if(det == 0.0f) 
	{
		// Matrix not invertible. Setting all elements to nan is not really
		// correct in a mathematical sense but it is easy to debug for the
		// programmer.
		const float nan = std::numeric_limits<float>::quiet_NaN();
		*this = aiMatrix3x3( nan,nan,nan,nan,nan,nan,nan,nan,nan);

		return *this;
	}

	float invdet = 1.0f / det;

	aiMatrix3x3 res;
	res.a1 = invdet  * (b2 * c3 - b3 * c2);
	res.a2 = -invdet * (a2 * c3 - a3 * c2);
	res.a3 = invdet  * (a2 * b3 - a3 * b2);
	res.b1 = -invdet * (b1 * c3 - b3 * c1);
	res.b2 = invdet  * (a1 * c3 - a3 * c1);
	res.b3 = -invdet * (a1 * b3 - a3 * b1);
	res.c1 = invdet  * (b1 * c2 - b2 * c1);
	res.c2 = -invdet * (a1 * c2 - a2 * c1);
	res.c3 = invdet  * (a1 * b2 - a2 * b1);
	*this = res;

	return *this;
}

// ------------------------------------------------------------------------------------------------
inline aiMatrix3x3& aiMatrix3x3::RotationZ(float a, aiMatrix3x3& out)
{
	out.a1 = out.b2 = ::cos(a);
	out.b1 = ::sin(a);
	out.a2 = - out.b1;

	out.a3 = out.b3 = out.c1 = out.c2 = 0.f;
	out.c3 = 1.f;

	return out;
}

// ------------------------------------------------------------------------------------------------
// Returns a rotation matrix for a rotation around an arbitrary axis.
inline aiMatrix3x3& aiMatrix3x3::Rotation( float a, const aiVector3D& axis, aiMatrix3x3& out)
{
  float c = cos( a), s = sin( a), t = 1 - c;
  float x = axis.x, y = axis.y, z = axis.z;

  // Many thanks to MathWorld and Wikipedia
  out.a1 = t*x*x + c;   out.a2 = t*x*y - s*z; out.a3 = t*x*z + s*y;
  out.b1 = t*x*y + s*z; out.b2 = t*y*y + c;   out.b3 = t*y*z - s*x;
  out.c1 = t*x*z - s*y; out.c2 = t*y*z + s*x; out.c3 = t*z*z + c;

  return out;
}

// ------------------------------------------------------------------------------------------------
inline aiMatrix3x3& aiMatrix3x3::Translation( const aiVector2D& v, aiMatrix3x3& out)
{
	out = aiMatrix3x3();
	out.a3 = v.x;
	out.b3 = v.y;
	return out;
}

// ----------------------------------------------------------------------------------------
/** A function for creating a rotation matrix that rotates a vector called
 * "from" into another vector called "to".
 * Input : from[3], to[3] which both must be *normalized* non-zero vectors
 * Output: mtx[3][3] -- a 3x3 matrix in colum-major form
 * Authors: Tomas Möller, John Hughes
 *          "Efficiently Building a Matrix to Rotate One Vector to Another"
 *          Journal of Graphics Tools, 4(4):1-4, 1999
 */
// ----------------------------------------------------------------------------------------
inline aiMatrix3x3& aiMatrix3x3::FromToMatrix(const aiVector3D& from, 
	const aiVector3D& to, aiMatrix3x3& mtx)
{
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
		const aiVector3D v = from ^ to;
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
#endif // AI_MATRIX3x3_INL_INC
