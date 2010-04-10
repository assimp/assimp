/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team

All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the following 
conditions are met:

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
---------------------------------------------------------------------------
*/

/** @file  aiColor4D.inl
 *  @brief Inline implementation of aiColor4D operators
 */
#ifndef AI_COLOR4D_INL_INC
#define AI_COLOR4D_INL_INC

#include "aiColor4D.h"
#ifdef __cplusplus

// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE const aiColor4D& aiColor4D::operator += (const aiColor4D& o) {
	r += o.r; g += o.g; b += o.b; a += o.a; return *this; 
}
// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE const aiColor4D& aiColor4D::operator -= (const aiColor4D& o) {
	r -= o.r; g -= o.g; b -= o.b; a -= o.a; return *this;
}
// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE const aiColor4D& aiColor4D::operator *= (float f) {
	r *= f; g *= f; b *= f; a *= f; return *this; 
}
// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE const aiColor4D& aiColor4D::operator /= (float f) {
	r /= f; g /= f; b /= f; a /= f; return *this; 
}
// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE float aiColor4D::operator[](unsigned int i) const {
	return *(&r + i);
}
// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE float& aiColor4D::operator[](unsigned int i) {
	return *(&r + i);
}
// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE bool aiColor4D::operator== (const aiColor4D& other) const {
	return r == other.r && g == other.g && b == other.b && a == other.a;
}
// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE bool aiColor4D::operator!= (const aiColor4D& other) const {
	return r != other.r || g != other.g || b != other.b || a != other.a;
}
// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE aiColor4D operator + (const aiColor4D& v1, const aiColor4D& v2)	{
	return aiColor4D( v1.r + v2.r, v1.g + v2.g, v1.b + v2.b, v1.a + v2.a);
}
// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE aiColor4D operator - (const aiColor4D& v1, const aiColor4D& v2)	{
	return aiColor4D( v1.r - v2.r, v1.g - v2.g, v1.b - v2.b, v1.a - v2.a);
}
// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE aiColor4D operator * (const aiColor4D& v1, const aiColor4D& v2)	{
	return aiColor4D( v1.r * v2.r, v1.g * v2.g, v1.b * v2.b, v1.a * v2.a);
}
// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE aiColor4D operator / (const aiColor4D& v1, const aiColor4D& v2)	{
	return aiColor4D( v1.r / v2.r, v1.g / v2.g, v1.b / v2.b, v1.a / v2.a);
}
// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE aiColor4D operator * ( float f, const aiColor4D& v)	{
	return aiColor4D( f*v.r, f*v.g, f*v.b, f*v.a);
}
// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE  aiColor4D operator * ( const aiColor4D& v, float f)	{
	return aiColor4D( f*v.r, f*v.g, f*v.b, f*v.a);
}
// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE  aiColor4D operator / ( const aiColor4D& v, float f)	{
	return v * (1/f);
}
// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE  aiColor4D operator / ( float f,const aiColor4D& v)	{
	return aiColor4D(f,f,f,f)/v;
}
// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE  aiColor4D operator + ( const aiColor4D& v, float f)	{
	return aiColor4D( f+v.r, f+v.g, f+v.b, f+v.a);
}
// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE  aiColor4D operator - ( const aiColor4D& v, float f)	{
	return aiColor4D( v.r-f, v.g-f, v.b-f, v.a-f);
}
// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE  aiColor4D operator + ( float f, const aiColor4D& v)	{
	return aiColor4D( f+v.r, f+v.g, f+v.b, f+v.a);
}
// ------------------------------------------------------------------------------------------------
AI_FORCE_INLINE  aiColor4D operator - ( float f, const aiColor4D& v)	{
	return aiColor4D( f-v.r, f-v.g, f-v.b, f-v.a);
}

// ------------------------------------------------------------------------------------------------
inline bool aiColor4D :: IsBlack() const	{
	// The alpha component doesn't care here. black is black.
	static const float epsilon = 10e-3f;
	return fabs( r ) < epsilon && fabs( g ) < epsilon && fabs( b ) < epsilon;
}

#endif // __cplusplus
#endif // AI_VECTOR3D_INL_INC
