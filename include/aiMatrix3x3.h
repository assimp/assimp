/** @file Definition of a 3x3 matrix, including operators when compiling in C++ */
#ifndef AI_MATRIX3x3_H_INC
#define AI_MATRIX3x3_H_INC

#ifdef __cplusplus
extern "C" {
#endif

struct aiMatrix4x4;

// ---------------------------------------------------------------------------
/** Represents a column-major 3x3 matrix 
*/
// ---------------------------------------------------------------------------
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

	/** Construction from a 4x4 matrix. The remaining parts of the matrix are ignored. */
	explicit aiMatrix3x3( const aiMatrix4x4& pMatrix);

	aiMatrix3x3& operator *= (const aiMatrix3x3& m);
	aiMatrix3x3 operator* (const aiMatrix3x3& m) const;
	aiMatrix3x3& Transpose();

#endif // __cplusplus


	float a1, a2, a3;
	float b1, b2, b3;
	float c1, c2, c3;
};

#ifdef __cplusplus
} // end of extern C
#endif

#endif // AI_MATRIX3x3_H_INC