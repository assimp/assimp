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
	aiVector3D_t mValue; ///< The value of this key
};

/** A time-value pair specifying a rotation for the given time. For joint animations
 * the rotation is usually expressed using a quaternion.
 */
struct aiQuatKey
{
	double mTime;      ///< The time of this key
	aiQuaternion_t mValue; ///< The value of this key
};

/** Describes the animation of a single bone. The name specifies the bone which is affected by this
 * animation channel. The keyframes are given in three separate series of values, one each for
 * position, rotation and scaling.
 */
struct aiBoneAnim
{
	/** The name of the bone affected by this animation. */
	aiString mBoneName;

	/** The number of position keys */
	unsigned int mNumPositionKeys;
	/** The position keys of this animation channel. Positions are specified as 3D vector. 
	* The array is mNumPositionKeys in size.
	*/
	aiVectorKey* mPositionKeys;

	/** The number of rotation keys */
	unsigned int mNumRotationKeys;
	/** The rotation keys of this animation channel. Rotations are given as quaternions, 
	* which are 4D vectors. The array is mNumRotationKeys in size.
	*/
	aiQuatKey* mRotationKeys;

	/** The number of scaling keys */
	unsigned int mNumScalingKeys;
	/** The scaling keys of this animation channel. Scalings are specified as 3D vector. 
	* The array is mNumScalingKeys in size.
	*/
	aiVectorKey* mScalingKeys;

#ifdef __cplusplus
	aiBoneAnim()
	{
		mNumPositionKeys = 0; mPositionKeys = NULL; 
		mNumRotationKeys= 0; mRotationKeys = NULL; 
		mNumScalingKeys = 0; mScalingKeys = NULL; 
	}

	~aiBoneAnim()
	{
		delete [] mPositionKeys;
		delete [] mRotationKeys;
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
	aiString mName;

	/** Duration of the animation in ticks. */
	double mDuration;
	/** Ticks per second. 0 if not specified in the imported file */
	double mTicksPerSecond;

	/** The number of bone animation channels. Each channel affects a single bone. */
	unsigned int mNumBones;
	/** The bone animation channels. Each channel affects a single bone. The array
	* is mNumBones in size.
	*/
	aiBoneAnim** mBones;

#ifdef __cplusplus
	aiAnimation()
	{
		mDuration = 0;
		mTicksPerSecond = 0;
		mNumBones = 0; mBones = NULL;
	}

	~aiAnimation()
	{
		for( unsigned int a = 0; a < mNumBones; a++)
			delete mBones[a];
		delete [] mBones;
	}
#endif // __cplusplus
};

#ifdef __cplusplus
}
#endif

#endif // AI_ANIM_H_INC
