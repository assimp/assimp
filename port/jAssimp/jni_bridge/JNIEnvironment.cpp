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

/** @file Implementation of the JNI API for jAssimp */

#if (defined ASSIMP_JNI_EXPORT)

#include "JNIEnvironment.h"
#include "JNILogger.h"

using namespace Assimp;

namespace Assimp	{
namespace JNIBridge		{


/*static*/ jclass JNIEnvironment::Class_java_lang_String = 0;
/*static*/ jmethodID JNIEnvironment::MID_String_getBytes = 0;
/*static*/ jmethodID JNIEnvironment::MID_String_init = 0;


// ------------------------------------------------------------------------------------------------
bool JNIEnvironment::AttachToCurrentThread (JNIEnv* pcEnv)
{
	ai_assert(NULL != pcEnv);

	// first initialize some members
	if (0 == Class_java_lang_String)
	{
		if( 0 == (Class_java_lang_String = pcEnv->FindClass("java.lang.String")))
			return false;
	}
	if (0 == MID_String_getBytes)
	{
		if( 0 == (MID_String_getBytes = pcEnv->GetStaticMethodID(
			Class_java_lang_String,"getBytes","()[byte")))
			return false;
	}
	if (0 == MID_String_init)
	{
		if( 0 == (MID_String_init = pcEnv->GetStaticMethodID(
			Class_java_lang_String,"String","([byte)V")))
			return false;
	}
	
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
jstring JNU_NewStringNative(JNIEnv *env, const char *str)
{
	jstring result;
	jbyteArray bytes = 0;
	int len;
	if (env->EnsureLocalCapacity( 2) < 0) 
	{
		return NULL; /* out of memory error */
	}
	len = strlen(str);
	bytes = env->NewByteArray(len);
	if (bytes != NULL) 
	{
		env->SetByteArrayRegion(bytes, 0, len,
			(jbyte *)str);
		result = (jstring)env->NewObject(JNIEnvironment::Class_java_lang_String,
			JNIEnvironment::MID_String_init, bytes);
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

	bytes = (jbyteArray)env->CallObjectMethod(jstr,JNIEnvironment::MID_String_getBytes);
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

};};

#endif // ! JNI only
