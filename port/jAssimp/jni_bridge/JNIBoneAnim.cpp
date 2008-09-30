/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

All rights reserved.
See the disclaimer in JNIEnvironment.h for licensing and distribution 
conditions.
---------------------------------------------------------------------------
*/

/** @file Implementation of the JNI API for jAssimp */

#include "JNIEnvironment.h"
#include "JNILogger.h"

using namespace Assimp;

namespace Assimp	{
namespace JNIBridge		{

// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_assimp::_BoneAnim::Initialize()
{
	JNIEnv* pc = JNIEnvironment::Get()->GetThread()->m_pcEnv;

	// load a handle to the class
	if(!(Class = pc->FindClass("assimp.BoneAnim")))
		JNIEnvironment::Get()->ThrowNativeError("Unable to load class assimp.BoneAnim");

	DefaultCtor = pc->GetMethodID(Class,"<init>","");

	// load all fields of the class
	mName			= pc->GetFieldID(Class,"mName","Lassimp.Node;");
	mPosKeys		= pc->GetFieldID(Class,"mPosKeys",	"[Lassimp.BoneAnim.Keyframe<[F>;");
	mScalingKeys	= pc->GetFieldID(Class,"mScalingKeys","[Lassimp.BoneAnim.Keyframe<[F>;");
	mQuatKeys		= pc->GetFieldID(Class,"mQuatKeys","[Lassimp.BoneAnim.Keyframe<Lassimp.Quaternion;>;");



	// check whether all fields have been loaded properly
	if (!mName || !mPosKeys || !mScalingKeys || !mQuatKeys)
		JNIEnvironment::Get()->ThrowNativeError("Unable to load all fields of class assimp.BoneAnim");
}
// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_assimp::_BoneAnim::Fill(jobject obj,const aiNodeAnim* pcSrc)
{
	JNIEnv* pc = JNIEnvironment::Get()->GetThread()->m_pcEnv;
	
}
}}