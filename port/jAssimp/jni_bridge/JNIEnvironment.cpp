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
bool JNIEnvironment::AttachToCurrentThread (JNIEnv* pcEnv)
{
	ai_assert(NULL != pcEnv);

	// now initialize the thread-local storage
	if (NULL == this->ptr.get())
	{
		// attach to the current thread
		JavaVM* vm;
		pcEnv->GetJavaVM(&vm);
		vm->AttachCurrentThread((void **) &pcEnv, NULL);

		this->ptr.reset(new JNIThreadData(pcEnv));
	}
	// increase the reference counter
	else this->ptr->m_iNumRef++;

	// attach the logger
	((JNILogDispatcher*)DefaultLogger::get())->OnAttachToCurrentThread(this->ptr.get());


	// get handles to all methods/fields/classes
	this->Initialize();

	return true;
}
// ------------------------------------------------------------------------------------------------
bool JNIEnvironment::DetachFromCurrentThread ()
{
	ai_assert(NULL != pcEnv);

	// detach the logger
	((JNILogDispatcher*)DefaultLogger::get())->OnDetachFromCurrentThread(this->ptr.get());

	// decrease the reference counter
	if (NULL != this->ptr.get())
	{
		this->ptr->m_iNumRef--;
		if (0 == this->ptr->m_iNumRef)
		{
			JavaVM* vm;
			this->ptr->m_pcEnv->GetJavaVM(&vm);
			vm->DetachCurrentThread();
		}
	}
	return true;
}
// ------------------------------------------------------------------------------------------------
JNIThreadData* JNIEnvironment::GetThread()
{
	ai_assert(NULL != this->ptr.get());
	return this->ptr.get();
}
// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_java::_lang::_String::Initialize()
{
	JNIEnv* pcEnv = JNIEnvironment::Get()->GetThread()->m_pcEnv;

	// first initialize some members
	if( !(this->Class = pcEnv->FindClass("java.lang.String")))
		JNIEnvironment::Get()->ThrowNativeError("Unable to get handle of class java.lang.String");
	
	if( !(this->getBytes = pcEnv->GetMethodID(this->Class,"getBytes","()[byte")))
		JNIEnvironment::Get()->ThrowNativeError("Unable to get handle of class java.lang.String");

	if( !(this->constructor_ByteArray = pcEnv->GetStaticMethodID(
		this->Class,"<init>","([byte)V")))
		JNIEnvironment::Get()->ThrowNativeError("Unable to get handle of class java.lang.String");
}
// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_java::_lang::_Array::Initialize()
{
	JNIEnv* pcEnv = JNIEnvironment::Get()->GetThread()->m_pcEnv;

	if( !(this->FloatArray_Class = pcEnv->FindClass("[F")))
		JNIEnvironment::Get()->ThrowNativeError("Unable to get handle of class float[]");

	if( !(this->IntArray_Class = pcEnv->FindClass("[I")))
		JNIEnvironment::Get()->ThrowNativeError("Unable to get handle of class int[]");
}
// ------------------------------------------------------------------------------------------------
jstring JNU_NewStringNative(JNIEnv *env, const char *str)
{
	jstring result;
	jbyteArray bytes = 0;
	int len;
	if (env->EnsureLocalCapacity( 2) < 0) 
	{
		return NULL; /* out of memory error */
	}
	len = (int)::strlen(str);
	bytes = env->NewByteArray(len);
	if (bytes != NULL) 
	{
		env->SetByteArrayRegion(bytes, 0, len,
			(jbyte *)str);
		result = (jstring)env->NewObject(AIJ_GET_HANDLE(java.lang.String.Class),
			AIJ_GET_HANDLE(java.lang.String.constructor_ByteArray), bytes);
		env->DeleteLocalRef(bytes);
		return result;
	} /* else fall through */
	return NULL;
}
// ------------------------------------------------------------------------------------------------
char *JNU_GetStringNativeChars(JNIEnv *env, jstring jstr)
{
	jbyteArray bytes = 0;
	jthrowable exc;
	char *result = 0;
	if (env->EnsureLocalCapacity(2) < 0) 
	{
		return 0; /* out of memory error */
	}

	bytes = (jbyteArray)env->CallObjectMethod(jstr,AIJ_GET_HANDLE(java.lang.String.getBytes));
	exc = env->ExceptionOccurred();
	if (!exc)
	{
		jint len = env->GetArrayLength(bytes);
		result = (char *)malloc(len + 1);
		if (result == 0) 
		{
			env->DeleteLocalRef(bytes);
			return 0;
		}
		env->GetByteArrayRegion(bytes, 0, len,
			(jbyte *)result);
		result[len] = 0; /* NULL-terminate */
	}
	else 
	{
		env->DeleteLocalRef(exc);
	}
	env->DeleteLocalRef(bytes);
	return result;
}
// ------------------------------------------------------------------------------------------------
void JNU_CopyDataToArray(JNIEnv* pc, jarray jfl, void* data, unsigned int size)
{
	void* pf;
	jboolean iscopy = FALSE;

	// lock the array and get direct access to its buffer
	if(!(pf = pc->GetPrimitiveArrayCritical(jfl,&iscopy)))
		JNIEnvironment::Get()->ThrowNativeError("Unable to lock array");

	// copy the data to the array
	::memcpy(pf,data,size);

	// release our reference to the array
	pc->ReleasePrimitiveArrayCritical(jfl,pf,0);
}
// ------------------------------------------------------------------------------------------------
void JNU_CopyObjectArrayToVM(JNIEnv* pc, const void**  in, unsigned int num, 
	 JNIEnvironment::_Base& type, jobjectArray& out)
{
	jobjectArray jarr =  pc->NewObjectArray(num,type.Class,0);
	for (unsigned int i = 0; i < num;++i)
	{
		jobject jobj = pc->NewObject(type.Class,type.DefaultCtor);

		type.Fill(jobj,in[i]);
		pc->SetObjectArrayElement(jarr,i,jobj);
	}
	out = jarr;
}

};};


