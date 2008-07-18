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

/** @file Defines the data structures in which the imported animations are returned. */
#ifndef AI_ANIM_H_INC
#define AI_ANIM_H_INC

#include "aiTypes.h"
#include "aiQuaternion.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A time-value pair specifying a certain 3D vector for the given time. */
struct aiVectorKey
{
	double mTime;      ///< The time of this key
	C_STRUCT aiVector3D mValue; ///< The value of this key
};

/** A time-value pair specifying a rotation for the given time. For joint animations
 * the rotation is usually expressed using a quaternion.
 */
struct aiQuatKey
{
	double mTime;      ///< The time of this key
	C_STRUCT aiQuaternion mValue; ///< The value of this key
};

/** Describes the animation of a single node. The name specifies the bone/node which is affected by this
 * animation channel. The keyframes are given in three separate series of values, one each for
 * position, rotation and scaling.
 * <br>
 * NOTE: The name "BoneAnim" is misleading. This structure is also used to describe
 * the animation of regular nodes on the node graph. They needn't be nodes.
 */
struct aiBoneAnim
{
	/** The name of the bone affected by this animation. */
	C_STRUCT aiString mBoneName;

	/** The number of position keys */
	unsigned int mNumPositionKeys;
	/** The position keys of this animation channel. Positions are specified as 3D vector. 
	* The array is mNumPositionKeys in size.
	*/
	C_STRUCT aiVectorKey* mPositionKeys;

	/** The number of rotation keys */
	unsigned int mNumRotationKeys;
	/** The rotation keys of this animation channel. Rotations are given as quaternions, 
	* which are 4D vectors. The array is mNumRotationKeys in size.
	*/
	C_STRUCT aiQuatKey* mRotationKeys;

	/** The number of scaling keys */
	unsigned int mNumScalingKeys;
	/** The scaling keys of this animation channel. Scalings are specified as 3D vector. 
	* The array is mNumScalingKeys in size.
	*/
	C_STRUCT aiVectorKey* mScalingKeys;

#ifdef __cplusplus
	aiBoneAnim()
	{
		mNumPositionKeys = 0; mPositionKeys = NULL; 
		mNumRotationKeys= 0; mRotationKeys = NULL; 
		mNumScalingKeys = 0; mScalingKeys = NULL; 
	}

	~aiBoneAnim()
	{
		if (mNumPositionKeys)
			delete [] mPositionKeys;
		if (mNumRotationKeys)
			delete [] mRotationKeys;
		if (mNumScalingKeys)
			delete [] mScalingKeys;
	}
#endif // __cplusplus
};

/** An animation consists of keyframe data for a number of bones. For each bone affected by the animation
 * a separate series of data is given.
 */
struct aiAnimation
{
	/** The name of the animation. If the modelling package this data was exported from does support 
	* only a single animation channel, this name is usually empty (length is zero).
	*/
	C_STRUCT aiString mName;

	/** Duration of the animation in ticks. */
	double mDuration;
	/** Ticks per second. 0 if not specified in the imported file */
	double mTicksPerSecond;

	/** The number of bone animation channels. Each channel affects a single bone. */
	unsigned int mNumBones;
	/** The bone animation channels. Each channel affects a single bone. The array
	* is mNumBones in size.
	*/
	C_STRUCT aiBoneAnim** mBones;

#ifdef __cplusplus
	aiAnimation()
	{
		mDuration = 0;
		mTicksPerSecond = 0;
		mNumBones = 0; mBones = NULL;
	}

	~aiAnimation()
	{
		if (mNumBones)
		{
			for( unsigned int a = 0; a < mNumBones; a++)
				delete mBones[a];
			delete [] mBones;
		}
	}
#endif // __cplusplus
};

#ifdef __cplusplus
}
#endif

#endif // AI_ANIM_H_INC
