
/* --------------------------------------------------------------------------------
 *
 * Open Asset Import Library (ASSIMP) (http://assimp.sourceforge.net)
 * Assimp2Java bridge 
 *
 * Copyright (c) 2006-2009, ASSIMP Development Team
 * All rights reserved. See the LICENSE file for more information.
 *
 * --------------------------------------------------------------------------------
 */

#include "jbridge_pch.h"
using namespace Assimp;

// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_assimp::_NodeAnim::Initialize()
{
	AIJ_LOAD_CLASS();

	AIJ_LOAD_FIELD(mName);
	AIJ_LOAD_FIELD(mPosKeys);
	AIJ_LOAD_FIELD(mScalingKeys);
	AIJ_LOAD_FIELD(mQuatKeys);

	VectorKey.Initialize();
	QuatKey.Initialize();
}
// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_assimp::_NodeAnim::Fill(jobject obj,const aiNodeAnim* pcSrc)
{
	jobjectArray ja;
	JNIEnv* pc = JNIEnvironment::Get()->GetThread()->m_pcEnv;	
	pc->SetObjectField(obj,mName,JNU_NewStringNative(pc,pcSrc->mNodeName.data));

	// copy position keys
	if (pcSrc->mNumPositionKeys)	{
		JNU_CopyObjectArrayToVM(pc,(const void**)pcSrc->mPositionKeys,pcSrc->mNumPositionKeys,
			AIJ_GET_HANDLE(assimp.NodeAnim.VectorKey),ja);
		pc->SetObjectField(obj,mPosKeys,ja);
	}

	// copy scaling keys
	if (pcSrc->mNumScalingKeys)	{
		JNU_CopyObjectArrayToVM(pc,(const void**)pcSrc->mScalingKeys,pcSrc->mNumScalingKeys,
			AIJ_GET_HANDLE(assimp.NodeAnim.VectorKey),ja);
		pc->SetObjectField(obj,mScalingKeys,ja);
	}

	// copy rotation keys
	if (pcSrc->mNumRotationKeys)	{
		JNU_CopyObjectArrayToVM(pc,(const void**)pcSrc->mRotationKeys,pcSrc->mNumRotationKeys,
			AIJ_GET_HANDLE(assimp.NodeAnim.QuatKey),ja);
		pc->SetObjectField(obj,mQuatKeys,ja);
	}
}
