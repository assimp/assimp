/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

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

----------------------------------------------------------------------
*/

/** @file Definition of a helper class that processes texture transformations */
#ifndef AI_TEXTURE_TRANSFORM_H_INCLUDED
#define AI_TEXTURE_TRANSFORM_H_INCLUDED

#include "BaseImporter.h"
#include "../include/aiTypes.h"
#include "../include/aiMaterial.h"
#include "../include/aiMesh.h"


struct aiNode;
#include "3DSHelper.h"

namespace Assimp
{

using namespace Assimp::Dot3DS;

// ---------------------------------------------------------------------------
/** Helper class representing texture coordinate transformations
*/
struct STransformVecInfo 
{
	//! Construction. The resulting matrix is the identity
	STransformVecInfo ()
		: 
		fScaleU(1.0f),fScaleV(1.0f),
		fOffsetU(0.0f),fOffsetV(0.0f),
		fRotation(0.0f),
		iUVIndex(0)
	{}

	//! Texture coordinate scaling in the x-direction 
	float fScaleU;
	//! Texture coordinate scaling in the y-direction 
	float fScaleV;
	//! Texture coordinate offset in the x-direction
	float fOffsetU;
	//! Texture coordinate offset in the y-direction
	float fOffsetV;
	//! Texture coordinate rotation, clockwise, in radians
	float fRotation;

	//! Source texture coordinate index
	unsigned int iUVIndex;


	//! List of all textures that use this texture
	//! coordinate transformations
	std::vector<Dot3DS::Texture*> pcTextures; 


	// -------------------------------------------------------------------
	/** Returns whether this is an untransformed texture coordinate set
	*/
	inline bool IsUntransformed() const
	{
		return 1.0f == fScaleU && 1.0f == fScaleV &&
			!fOffsetU && !fOffsetV && !fRotation;
	}

	// -------------------------------------------------------------------
	/** Build a 3x3 matrix from the transformations
	*/
	inline void GetMatrix(aiMatrix3x3& mOut)
	{
		mOut = aiMatrix3x3();

		if (1.0f != this->fScaleU || 1.0f != this->fScaleV)
		{
			aiMatrix3x3 mScale;
			mScale.a1 = this->fScaleU;
			mScale.b2 = this->fScaleV;
			mOut = mScale;
		}
		if (this->fRotation)
		{
			aiMatrix3x3 mRot; 
			mRot.a1 = mRot.b2 = cosf(this->fRotation);
			mRot.a2 = mRot.b1 = sinf(this->fRotation);
			mRot.a2 = -mRot.a2;
			mOut *= mRot;
		}
		if (this->fOffsetU || this->fOffsetV)
		{
			aiMatrix3x3 mTrans; 
			mTrans.a3 = this->fOffsetU;
			mTrans.b3 = this->fOffsetV;
			mOut *= mTrans;
		}
	}
};


// ---------------------------------------------------------------------------
/** Helper class used by the ASE/ASK and 3DS loaders to handle texture
 *  coordinate transformations correctly (such as offsets, scaling)
*/
class ASSIMP_API TextureTransform
{
	//! Constructor, it is not possible to create instances of this class
	TextureTransform() {}
public:


	// -------------------------------------------------------------------
	/** Returns true if a texture requires UV transformations
	 * \param rcIn Input texture
	*/
	inline static bool HasUVTransform(
		const Dot3DS::Texture& rcIn)
	{
		return (rcIn.mOffsetU || rcIn.mOffsetV ||
			1.0f != rcIn.mScaleU  ||  1.0f != rcIn.mScaleV || rcIn.mRotation);
	}

	// -------------------------------------------------------------------
	/** Must be called before HasUVTransform(rcIn) is called 
	 * \param rcIn Input texture
	*/
	static void PreProcessUVTransform(
		Dot3DS::Texture& rcIn);

	// -------------------------------------------------------------------
	/** Check whether the texture coordinate transformation of
	 *  a texture is already contained in a given list
	 * \param rasVec List of transformations
	 * \param pcTex Pointer to the texture
	*/
	static void AddToList(std::vector<STransformVecInfo>& rasVec,
		Dot3DS::Texture* pcTex);

	// -------------------------------------------------------------------
	/** Get a full list of all texture coordinate offsets required
	 *  for a material
	 * \param materials List of materials to be processed
	*/
	static void ApplyScaleNOffset(std::vector<Dot3DS::Material>& materials);

	// -------------------------------------------------------------------
	/** Get a full list of all texture coordinate offsets required
	 *  for a material
	 * \param material Material to be processed
	*/
	static void ApplyScaleNOffset(Dot3DS::Material& material);

	// -------------------------------------------------------------------
	/** Precompute as many texture coordinate transformations as possible
	 * \param pcMesh Mesh containing the texture coordinate data
	 * \param pcSrc Input material. Must have been passed to
	 * ApplyScaleNOffset
	*/
	static void BakeScaleNOffset(aiMesh* pcMesh, Dot3DS::Material* pcSrc);

	
	// -------------------------------------------------------------------
	/** Setup the correct UV source for a material
	 * \param pcMat Final material to be changed
	 * \param pcMatIn Input material, unconverted
	*/
	static void SetupMatUVSrc (aiMaterial* pcMat, 
		const Dot3DS::Material* pcMatIn);
};

};

#endif //! AI_TEXTURE_TRANSFORM_H_INCLUDED