/** @file Inline implementation of vector3D operators */
#ifndef AI_VECTOR3D_INL_INC
#define AI_VECTOR3D_INL_INC

#include "aiVector3D.h"

#ifdef __cplusplus
#include "aiMatrix3x3.h"
#include "aiMatrix4x4.h"

/** Transformation of a vector by a 3x3 matrix */
inline aiVector3D operator * (const aiMatrix3x3& pMatrix, const aiVector3D& pVector)
{
	aiVector3D res;
	res.x = pMatrix.a1 * pVector.x + pMatrix.a2 * pVector.y + pMatrix.a3 * pVector.z;
	res.y = pMatrix.b1 * pVector.x + pMatrix.b2 * pVector.y + pMatrix.b3 * pVector.z;
	res.z = pMatrix.c1 * pVector.x + pMatrix.c2 * pVector.y + pMatrix.c3 * pVector.z;
	return res;
}

/** Transformation of a vector by a 4x4 matrix */
inline aiVector3D operator * (const aiMatrix4x4& pMatrix, const aiVector3D& pVector)
{
	aiVector3D res;
	res.x = pMatrix.a1 * pVector.x + pMatrix.a2 * pVector.y + pMatrix.a3 * pVector.z + pMatrix.a4;
	res.y = pMatrix.b1 * pVector.x + pMatrix.b2 * pVector.y + pMatrix.b3 * pVector.z + pMatrix.b4;
	res.z = pMatrix.c1 * pVector.x + pMatrix.c2 * pVector.y + pMatrix.c3 * pVector.z + pMatrix.c4;
	return res;
}


#endif // __cplusplus
#endif // AI_VECTOR3D_INL_INC
