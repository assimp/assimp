
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
void JNIEnvironment::_assimp::_Animation::Initialize()
{
	AIJ_LOAD_CLASS();

	AIJ_LOAD_FIELD(name);
	AIJ_LOAD_FIELD(mDuration);
	AIJ_LOAD_FIELD(mTicksPerSecond);
	AIJ_LOAD_FIELD(nodeAnims);
}
// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_assimp::_Animation::Fill(jobject obj,const aiAnimation* pcSrc)
{
	jobjectArray ja;
	JNIEnv* pc = JNIEnvironment::Get()->GetThread()->m_pcEnv;	
	pc->SetObjectField(obj,name,JNU_NewStringNative(pc,pcSrc->mName.data));
	pc->SetDoubleField(obj,mDuration,pcSrc->mDuration);
	pc->SetDoubleField(obj,mTicksPerSecond,pcSrc->mTicksPerSecond);

	// copy node animations
	if (pcSrc->mNumChannels)	{
		JNU_CopyObjectArrayToVM(pc,(const void**)pcSrc->mChannels,pcSrc->mNumChannels,
			AIJ_GET_HANDLE(assimp.NodeAnim),ja);
		pc->SetObjectField(obj,nodeAnims,ja);
	}
}
