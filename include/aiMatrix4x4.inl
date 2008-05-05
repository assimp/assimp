/** @file Inline implementation of the 4x4 matrix operators */
#ifndef AI_MATRIX4x4_INL_INC
#define AI_MATRIX4x4_INL_INC

#include "aiMatrix4x4.h"

#ifdef __cplusplus
#include "aiMatrix3x3.h"

#include <algorithm>
#include <limits>
#include <math.h>

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
		*this = aiMatrix4x4(
			std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::quiet_NaN(),
			std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::quiet_NaN(),
			std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::quiet_NaN(),
			std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::quiet_NaN(),
			std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::quiet_NaN(),
			std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::quiet_NaN(),
			std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::quiet_NaN(),
			std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::quiet_NaN());
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

#endif // __cplusplus
#endif // AI_MATRIX4x4_INL_INC
