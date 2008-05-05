/** @file 3D vector structure, including operators when compiling in C++ */
#ifndef AI_VECTOR3D_H_INC
#define AI_VECTOR3D_H_INC

#include <math.h>

#include "aiAssert.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
/** Represents a three-dimensional vector. */
typedef struct aiVector3D
{
#ifdef __cplusplus
	aiVector3D () : x(0.0f), y(0.0f), z(0.0f) {}
	aiVector3D (float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
	aiVector3D (const aiVector3D& o) : x(o.x), y(o.y), z(o.z) {}

	void Set( float pX, float pY, float pZ) { x = pX; y = pY; z = pZ; }
	float SquareLength() const { return x*x + y*y + z*z; }
	float Length() const { return sqrt( SquareLength()); }
	aiVector3D& Normalize() { *this /= Length(); return *this; }
	const aiVector3D& operator += (const aiVector3D& o) { x += o.x; y += o.y; z += o.z; return *this; }
	const aiVector3D& operator -= (const aiVector3D& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
	const aiVector3D& operator *= (float f) { x *= f; y *= f; z *= f; return *this; }
	const aiVector3D& operator /= (float f) { x /= f; y /= f; z /= f; return *this; }

	inline float operator[](unsigned int i) const {return *(&x + i);}
	inline float& operator[](unsigned int i) {return *(&x + i);}
#endif // __cplusplus

	float x, y, z;	
} aiVector3D_t;

#ifdef __cplusplus
} // end extern "C"

// symmetric addition
inline aiVector3D operator + (const aiVector3D& v1, const aiVector3D& v2)
{
	return aiVector3D( v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

// symmetric subtraction
inline aiVector3D operator - (const aiVector3D& v1, const aiVector3D& v2)
{
	return aiVector3D( v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

// scalar product
inline float operator * (const aiVector3D& v1, const aiVector3D& v2)
{
	return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
}

// scalar multiplication
inline aiVector3D operator * ( float f, const aiVector3D& v)
{
	return aiVector3D( f*v.x, f*v.y, f*v.z);
}

// and the other way around
inline aiVector3D operator * ( const aiVector3D& v, float f)
{
	return aiVector3D( f*v.x, f*v.y, f*v.z);
}

// scalar division
inline aiVector3D operator / ( const aiVector3D& v, float f)
{
	//ai_assert(0.0f != f);
	return v * (1/f);
}

// vector division
inline aiVector3D operator / ( const aiVector3D& v, const aiVector3D& v2)
	{
	//ai_assert(0.0f != v2.x && 0.0f != v2.y && 0.0f != v2.z);
	return aiVector3D(v.x / v2.x,v.y / v2.y,v.z / v2.z);
	}

// cross product
inline aiVector3D operator ^ ( const aiVector3D& v1, const aiVector3D& v2)
{
	return aiVector3D( v1.y*v2.z - v1.z*v2.y, v1.z*v2.x - v1.x*v2.z, v1.x*v2.y - v1.y*v2.x);
}

// vector inversion
inline aiVector3D operator - ( const aiVector3D& v)
{
	return aiVector3D( -v.x, -v.y, -v.z);
}

#endif // __cplusplus

#endif // AI_VECTOR3D_H_INC