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
		aiVector3D(_this[0][0],_this[1][1],_this[2][0]),
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
		vRows[0].x /= scaling.x;
		vRows[0].y /= scaling.x;
		vRows[0].z /= scaling.x;
	}
	if(scaling.y)
	{
		vRows[1].x /= scaling.y;
		vRows[1].y /= scaling.y;
		vRows[1].z /= scaling.y;
	}
	if(scaling.z)
	{
		vRows[2].x /= scaling.z;
		vRows[2].y /= scaling.z;
		vRows[2].z /= scaling.z;
	}

	// build a 3x3 rotation matrix
	aiMatrix3x3 m(vRows[0].x,vRows[0].y,vRows[0].z,
		vRows[1].x,vRows[1].y,vRows[1].z,
		vRows[2].x,vRows[2].y,vRows[2].z);

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
inline void aiMatrix4x4::FromEulerAngles(float x, float y, float z)
{
	aiMatrix4x4& _this = *this;

	const float A       = ::cosf(x);
    const float B       = ::sinf(x);
    const float C       = ::cosf(y);
    const float D       = ::sinf(y);
    const float E       = ::cosf(z);
    const float F       = ::sinf(z);
    const float AD      =   A * D;
    const float BD      =   B * D;
    _this.a1  =   C * E;
    _this.a2  =  -C * F;
    _this.a3  =   D;
    _this.b1  =  BD * E + A * F;
    _this.b2  = -BD * F + A * E;
    _this.b3  =  -B * C;
    _this.c1  = -AD * E + B * F;
    _this.c2  =  AD * F + B * E;
    _this.c3 =   A * C;
    _this.a4 = _this.b4 = _this.c4 = _this.d1 = _this.d2 = _this.d3 =  0.0f;
    _this.d4 = 1.0f;
}
// ---------------------------------------------------------------------------
inline bool aiMatrix4x4::IsIdentity() const
{
	return !(a1 != 1.0f || a2 || a3 || a4 ||
		b1 || b2 != 1.0f || b3 || b4 ||
		c1 || c2 || c3 != 1.0f || a4 ||
		d1 || d2 || d3 || d4 != 1.0f);
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
inline aiMatrix4x4& aiMatrix4x4::Translation(aiVector3D v, aiMatrix4x4& out)
{
	out = aiMatrix4x4();
	out.d1 = v.x;
	out.d2 = v.y;
	out.d3 = v.z;
	return out;
}


#endif // __cplusplus
#endif // AI_MATRIX4x4_INL_INC
