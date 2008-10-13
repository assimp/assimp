/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

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

/** @file Defines the aiLight data structure
 */

#ifndef AI_TEXTURE_H_INC
#define AI_TEXTURE_H_INC

#include "aiTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
/** Enumerates all supported types of light sources.
 */
enum aiLightSourceType
{
	aiLightSource_UNDEFINED     = 0x0,
	aiLightSource_DIRECTIONAL   = 0x1,
	aiLightSource_POINT         = 0x2,
	aiLightSource_SPOT          = 0x3
};

// ---------------------------------------------------------------------------
/** Helper structure to describe a light source.
 *
 *  Assimp supports multiple sorts of light sources, including
 *  directional, point and spot lights. All of them are defined with just
 *  a single structure. 
*/
struct aiLight
{
	/** The name of the light sources.
	 *
	 *  By this name it is referenced by a node in the scene graph.
	 */
	aiString mName;

	/** The type of the light source.
	 */
	aiLightSourceType mType;

	aiMatrix4x4 mLocalTransform;

	float mAttenuationConstant;
	float mAttenuationLinear;
	float mAttenuationQuadratic;

	aiColor3D mColorDiffuse;
	aiColor3D mColorSpecular;
	aiColor3D mColorAmbient;

	float mAngleOuterCone;
	float mAngleInnerCone;

#ifdef __cplusplus

	aiLight()
		:	mType                 (aiLightSource_UNDEFINED)
		,	mAttenuationConstant  (0.f)
		,   mAttenuationLinear    (1.f)
		,   mAttenuationQuadratic (0.f)
		,	mAngleOuterCone       (AI_MATH_TWO_PI)
		,	mAngleInnerCone       (AI_MATH_TWO_PI)
	{
	}

#endif
};

#ifdef __cplusplus
}
#endif


#endif