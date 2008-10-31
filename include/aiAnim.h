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

#endif
};

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

#endif
};

enum aiAnimBehaviour
{
	// --- Wert aus Node-Transformation wird übernommen
	aiAnimBehaviour_DEFAULT  = 0x0,  

	// -- Nächster Key wird verwendet
	aiAnimBehaviour_CONSTANT = 0x1,

	// -- Nächste beiden Keys werden linear extrapoliert
	aiAnimBehaviour_LINEAR   = 0x2,

	// -- Animation wird wiederholt
	// Und das solange bis die Animationszeit (aiAnimation::mDuration)
	// abgelaufen ist. Ist diese 0 läuft das ganze ewig.
	aiAnimBehaviour_REPEAT   = 0x3
};

/** Describes the animation of a single node. The name specifies the 
 *  bone/node which is affected by this animation channel. The keyframes
 *  are given in three separate series of values, one each for position, 
 *  rotation and scaling. The transformation matrix computed from these
 *  values replaces the node's original transformation matrix at a
 *  spefific time. 
 */
struct aiNodeAnim
{
	/** The name of the node affected by this animation. The node 
	 *  must exist anf it must be unique.
	 */
	C_STRUCT aiString mNodeName;

	/** The number of position keys */
	unsigned int mNumPositionKeys;

	/** The position keys of this animation channel. Positions are 
	 * specified as 3D vector. The array is mNumPositionKeys in size.
	 *
	 *  If there are rotation or scaling keys, but no position keys,
	 *  a constant position of 0|0|0 should be assumed.
	 */
	C_STRUCT aiVectorKey* mPositionKeys;

	/** The number of rotation keys */
	unsigned int mNumRotationKeys;

	/** The rotation keys of this animation channel. Rotations are 
	 *  given as quaternions,  which are 4D vectors. The array is 
	 *  mNumRotationKeys in size.
	 *
	 *  If there are position or scaling keys, but no rotation keys,
	 *  a constant rotation of 0|0|0 should be assumed. 
	 */
	C_STRUCT aiQuatKey* mRotationKeys;


	/** The number of scaling keys */
	unsigned int mNumScalingKeys;

	/** The scaling keys of this animation channel. Scalings are 
	 *  specified as 3D vector. The array is mNumScalingKeys in size.
	 *
	 *  If there are position or rotation keys, but no scaling keys,
	 *  a constant scaling of 1|1|1 should be assumed. 
	 */
	C_STRUCT aiVectorKey* mScalingKeys;


	aiAnimBehaviour mPrePostState;

#ifdef __cplusplus
	aiNodeAnim()
	{
		mNumPositionKeys = 0; mPositionKeys = NULL; 
		mNumRotationKeys= 0; mRotationKeys = NULL; 
		mNumScalingKeys = 0; mScalingKeys = NULL; 
	}

	~aiNodeAnim()
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
		mDuration = 0;
		mTicksPerSecond = 0;
		mNumChannels = 0; mChannels = NULL;
	}

	~aiAnimation()
	{
		if (mNumChannels)
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
