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
void JNIEnvironment::_assimp::_Bone::Initialize()
{
	JNIEnv* pc = JNIEnvironment::Get()->GetThread()->m_pcEnv;

	// load a handle to the class
	if(!(Class = pc->FindClass("assimp.Bone")))
		JNIEnvironment::Get()->ThrowNativeError("Unable to load class assimp.Bone");

	DefaultCtor = pc->GetMethodID(Class,"<init>","");

	// load all fields of the class
	name			= pc->GetFieldID(Class,"name",		"Ljava.lang.String;");
	weights			= pc->GetFieldID(Class,"weights",	"[Lassimp.Bone.Weight;");

	// check whether all fields have been loaded properly
	if (!name || !weights)
		JNIEnvironment::Get()->ThrowNativeError("Unable to load all fields of class assimp.Bone");

	Weight.Initialize();
}
// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_assimp::_Bone::Fill(jobject obj,const aiBone* pcSrc)
{
	JNIEnv* pc = JNIEnvironment::Get()->GetThread()->m_pcEnv;	
}


// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_assimp::_Bone::_Weight::Initialize()
{
	JNIEnv* pc = JNIEnvironment::Get()->GetThread()->m_pcEnv;

	// load a handle to the class
	if(!(Class = pc->FindClass("assimp.Bone.Weight")))
		JNIEnvironment::Get()->ThrowNativeError("Unable to load class assimp.Bone.Weights");

	DefaultCtor = pc->GetMethodID(Class,"<init>","");

	// load all fields of the class
	index			= pc->GetFieldID(Class,"index",		"I");
	weight			= pc->GetFieldID(Class,"weight",	"F");

	// check whether all fields have been loaded properly
	if (!index || !weight)
		JNIEnvironment::Get()->ThrowNativeError("Unable to load all fields of class assimp.Bone.Weight");
}
// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_assimp::_Bone::_Weight::Fill(jobject obj,const aiVertexWeight* pcSrc)
{
	JNIEnv* pc = JNIEnvironment::Get()->GetThread()->m_pcEnv;	

	pc->SetIntField(obj,index,(jint)pcSrc->mVertexId);
	pc->SetFloatField(obj,index,(jfloat)pcSrc->mWeight);
}


}}