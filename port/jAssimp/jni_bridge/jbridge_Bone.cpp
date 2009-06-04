
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
void JNIEnvironment::_assimp::_Bone::Initialize()
{
	AIJ_LOAD_CLASS();

	AIJ_LOAD_FIELD(name);
	AIJ_LOAD_FIELD(weights);

	Weight.Initialize();
}
// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_assimp::_Bone::Fill(jobject obj,const aiBone* pcSrc)
{
	JNIEnv* pc = JNIEnvironment::Get()->GetThread()->m_pcEnv;	

	// copy bone weights
	if (pcSrc->mNumWeights)	{

		jobjectArray jarr =  pc->NewObjectArray(pcSrc->mNumWeights,Weight.Class,0);
		for (unsigned int i = 0; i < pcSrc->mNumWeights;++i)	{
			jobject jobj = pc->NewObject(Weight.Class,Weight.DefaultCtor);

			Weight.Fill(jobj,&pcSrc->mWeights[i]);
			pc->SetObjectArrayElement(jarr,i,jobj);
		}
		pc->SetObjectField(obj,weights,jarr);
	}
	pc->SetObjectField(obj,name,JNU_NewStringNative(pc,pcSrc->mName.data));

	jobject matrix = pc->NewObject(AIJ_GET_CLASS_HANDLE(assimp.Matrix4x4),AIJ_GET_DEFAULT_CTOR_HANDLE(assimp.Matrix4x4));
	AIJ_GET_HANDLE(assimp.Matrix4x4).Fill(matrix,&pcSrc->mOffsetMatrix);
	pc->SetObjectField(obj,name,matrix);
}

// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_assimp::_Bone::_Weight::Initialize()
{
	AIJ_LOAD_CLASS();
	AIJ_LOAD_FIELD(index);
	AIJ_LOAD_FIELD(weight);
}

// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_assimp::_Bone::_Weight::Fill(jobject obj,const aiVertexWeight* pcSrc)
{
	JNIEnv* pc = JNIEnvironment::Get()->GetThread()->m_pcEnv;	

	pc->SetIntField(obj,index,(jint)pcSrc->mVertexId);
	pc->SetFloatField(obj,weight,(jfloat)pcSrc->mWeight);
}
