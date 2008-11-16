/** @file Inline implementation of the 3x3 matrix operators */
#ifndef AI_MATRIX3x3_INL_INC
#define AI_MATRIX3x3_INL_INC

#include "aiMatrix3x3.h"

#ifdef __cplusplus
#include "aiMatrix4x4.h"
#include <algorithm>

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
	*this = aiMatrix3x3(
		m.a1 * a1 + m.b1 * a2 + m.c1 * a3,
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
inline aiMatrix3x3& aiMatrix3x3::Transpose()
{
	// (float&) don't remove, GCC complains cause of packed fields
	std::swap( (float&)a2, (float&)b1);
	std::swap( (float&)a3, (float&)c1);
	std::swap( (float&)b3, (float&)c2);
	return *this;
}

// ------------------------------------------------------------------------------------------------
inline aiMatrix3x3& aiMatrix3x3::Rotation(float a, aiMatrix3x3& out)
{
	out.a1 = out.b2 = ::cos(a);
	out.b1 = ::sin(a);
	out.a2 = - out.b1;

	out.a3 = out.b3 = out.c1 = out.c2 = 0.f;
	out.c3 = 1.f;

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


#endif // __cplusplus
#endif // AI_MATRIX3x3_INL_INC
