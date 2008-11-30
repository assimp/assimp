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

/** @file Defines the data structures in which the imported animations
   are returned. */
#ifndef AI_ANIM_H_INC
#define AI_ANIM_H_INC

#include "aiTypes.h"
#include "aiQuaternion.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
/** A time-value pair specifying a certain 3D vector for the given time. */
struct aiVectorKey
{
	double mTime;      ///< The time of this key
	C_STRUCT aiVector3D mValue; ///< The value of this key

#ifdef __cplusplus

	// time is not compared
	bool operator == (const aiVectorKey& o) const
		{return o.mValue == this->mValue;}

	bool operator != (const aiVectorKey& o) const
		{return o.mValue != this->mValue;}



	// Only time is compared. This operator is defined
	// for use with std::sort
	bool operator < (const aiVectorKey& o) const
		{return mTime < o.mTime;}

	bool operator > (const aiVectorKey& o) const
		{return mTime > o.mTime;}


#endif
};


// ---------------------------------------------------------------------------
/** A time-value pair specifying a rotation for the given time. For joint 
 *  animations the rotation is usually expressed using a quaternion.
 */
struct aiQuatKey
{
	double mTime;      ///< The time of this key
	C_STRUCT aiQuaternion mValue; ///< The value of this key

#ifdef __cplusplus

	// time is not compared
	bool operator == (const aiQuatKey& o) const
		{return o.mValue == this->mValue;}

	bool operator != (const aiQuatKey& o) const
		{return o.mValue != this->mValue;}


	// Only time is compared. This operator is defined
	// for use with std::sort
	bool operator < (const aiQuatKey& o) const
		{return mTime < o.mTime;}

	bool operator > (const aiQuatKey& o) const
		{return mTime < o.mTime;}


#endif
};

// ---------------------------------------------------------------------------
/** Defines how an animation channel behaves outside the defined time
 *  range. This corresponds to aiNodeAnim::mPreState and 
 *  aiNodeAnim::mPostState.
 */
enum aiAnimBehaviour
{
	/** The value from the default node transformation is taken
	 */
	aiAnimBehaviour_DEFAULT  = 0x0,  

	/** The nearest key is used
	 */
	aiAnimBehaviour_CONSTANT = 0x1,

	/** The value of the nearest two keys is linearly
	 *  extrapolated for the current time value.
	 */
	aiAnimBehaviour_LINEAR   = 0x2,

	/** The animation is repeated.
	 *
	 *  If the animation key go from n to m and the current
	 *  time is t, use the value at (t-n) % (|m-n|).
	 */
	aiAnimBehaviour_REPEAT   = 0x3,



	/** This value is not used, it is just here to force the
	 *  the compiler to map this enum to a 32 Bit integer 
	 */
	_aiAnimBehaviour_Force32Bit = 0x8fffffff
};

// ---------------------------------------------------------------------------
/** Describes the animation of a single node. The name specifies the 
 *  bone/node which is affected by this animation channel. The keyframes
 *  are given in three separate series of values, one each for position, 
 *  rotation and scaling. The transformation matrix computed from these
 *  values replaces the node's original transformation matrix at a
 *  spefific time. The order in which the transformations are applied is
 *  - as usual - scaling, rotation, translation.
 *
 *  @note All keys are returned in their correct, chronological order.
 *  Duplicate keys don't pass the validation step. Most likely there
 *  will be no negative time keys, but they are not forbidden ...
 */
struct aiNodeAnim
{
	/** The name of the node affected by this animation. The node 
	 *  must exist and it must be unique.
	 */
	C_STRUCT aiString mNodeName;

	/** The number of position keys */
	unsigned int mNumPositionKeys;

	/** The position keys of this animation channel. Positions are 
	 * specified as 3D vector. The array is mNumPositionKeys in size.
	 *
	 * If there are position keys, there will also be at least one
	 * scaling and one rotation key.
	 */
	C_STRUCT aiVectorKey* mPositionKeys;

	/** The number of rotation keys */
	unsigned int mNumRotationKeys;

	/** The rotation keys of this animation channel. Rotations are 
	 *  given as quaternions,  which are 4D vectors. The array is 
	 *  mNumRotationKeys in size.
	 *
	 * If there are rotation keys, there will also be at least one
	 * scaling and one position key.
	 */
	C_STRUCT aiQuatKey* mRotationKeys;


	/** The number of scaling keys */
	unsigned int mNumScalingKeys;

	/** The scaling keys of this animation channel. Scalings are 
	 *  specified as 3D vector. The array is mNumScalingKeys in size.
	 *
	 * If there are scaling keys, there will also be at least one
	 * position and one rotation key.
	 */
	C_STRUCT aiVectorKey* mScalingKeys;


	/** Defines how the animation behaves before the first
	 *  key is encountered.
	 *
	 *  The default value is aiAnimBehaviour_DEFAULT (the original
	 *  transformation matrix of the affacted node is taken).
	 */
	aiAnimBehaviour mPreState;

	/** Defines how the animation behaves after the last 
	 *  kway was encountered.
	 *
	 *  The default value is aiAnimBehaviour_DEFAULT (the original
	 *  transformation matrix of the affacted node is taken).
	 */
	aiAnimBehaviour mPostState;

#ifdef __cplusplus
	aiNodeAnim()
	{
		mNumPositionKeys = 0; mPositionKeys = NULL; 
		mNumRotationKeys= 0; mRotationKeys = NULL; 
		mNumScalingKeys = 0; mScalingKeys = NULL; 

		mPreState = mPostState = aiAnimBehaviour_DEFAULT;
	}

	~aiNodeAnim()
	{
		delete [] mPositionKeys;
		delete [] mRotationKeys;
		delete [] mScalingKeys;
	}
#endif // __cplusplus
};

// ---------------------------------------------------------------------------
/** An animation consists of keyframe data for a number of nodes. For 
 *  each node affected by the animation a separate series of data is given.
 */
struct aiAnimation
{
	/** The name of the animation. If the modelling package this data was 
	 *  exported from does support only a single animation channel, this 
	 *  name is usually empty (length is zero).
	 */
	C_STRUCT aiString mName;

	/** Duration of the animation in ticks. 
	 */
	double mDuration;

	/** Ticks per second. 0 if not specified in the imported file 
	 */
	double mTicksPerSecond;

	/** The number of bone animation channels. Each channel affects
	 *  a single node.
	 */
	unsigned int mNumChannels;

	/** The node animation channels. Each channel affects a single node. 
	 *  The array is mNumChannels in size.
	 */
	C_STRUCT aiNodeAnim** mChannels;

#ifdef __cplusplus
	aiAnimation()
	{
		mDuration = -1.;
		mTicksPerSecond = 0;
		mNumChannels = 0; mChannels = NULL;
	}

	~aiAnimation()
	{
		// DO NOT REMOVE THIS ADDITIONAL CHECK
		if (mNumChannels && mChannels)
		{
			for( unsigned int a = 0; a < mNumChannels; a++)
				delete mChannels[a];

		delete [] mChannels;
		}
	}
#endif // __cplusplus
};

#ifdef __cplusplus
}
#endif

#endif // AI_ANIM_H_INC
