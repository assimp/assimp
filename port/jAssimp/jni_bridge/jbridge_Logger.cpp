
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
bool JNILogDispatcher::OnAttachToCurrentThread(JNIThreadData* pcData)
{
	ai_assert(NULL != pcData);
	//this->AddRef(); - done at another location

	// There is much error handling code in this function. We do it to be absolutely sure that the
	// interface is of the same version on both sides.
	JNIEnv* jvmenv = AIJ_GET_JNIENV();

	// get a handle to the assimp.DefaultLogger class
	if (!m_pcClass)	{
		if( NULL == (m_pcClass = jvmenv->FindClass("assimp.DefaultLogger")))	{
			return false;
		}
	}
	// get handles to the logging functions
	if (!m_pcMethodError)	{
		if( !(m_pcMethodError = jvmenv->GetStaticMethodID(m_pcClass,"_NativeCallWriteError","(Ljava/lang/String;)V")))	{
			return false;
		}
	}
	if (!m_pcMethodWarn)	{
		if( !(m_pcMethodWarn = jvmenv->GetStaticMethodID(m_pcClass,"_NativeCallWriteWarn","(Ljava/lang/String;)V")))	{
			return false;
		}
	}
	if (!m_pcMethodInfo)	{
		if( !(m_pcMethodInfo = jvmenv->GetStaticMethodID(m_pcClass,"_NativeCallWriteInfo","(Ljava/lang/String;)V")))	{
			return false;
		}
	}
	if (!m_pcMethodDebug)	{
		if( !(m_pcMethodDebug = jvmenv->GetStaticMethodID(m_pcClass,"_NativeCallWriteDebug","(Ljava/lang/String;)V")))	{
			return false;
		}
	}
	return true;
}

// ------------------------------------------------------------------------------------------------
bool JNILogDispatcher::OnDetachFromCurrentThread(JNIThreadData* pcData)
{
	Release();
	return true;
}

// ------------------------------------------------------------------------------------------------
void JNILogDispatcher::debug(const std::string &message)
{
	JNIEnv* jvmenv = AIJ_GET_JNIENV();
	jstring jstr = JNU_NewStringNative(jvmenv,message.c_str());

	jvmenv->CallStaticVoidMethod(m_pcClass,m_pcMethodDebug,jstr);
	jvmenv->DeleteLocalRef(jstr);
}

// ------------------------------------------------------------------------------------------------
void JNILogDispatcher::info(const std::string &message)
{
	JNIEnv* jvmenv = AIJ_GET_JNIENV();
	jstring jstr = JNU_NewStringNative(jvmenv,message.c_str());

	jvmenv->CallStaticVoidMethod(m_pcClass,m_pcMethodInfo,jstr);
	jvmenv->DeleteLocalRef(jstr);
}

// ------------------------------------------------------------------------------------------------
void JNILogDispatcher::warn(const std::string &message)
{
	JNIEnv* jvmenv = AIJ_GET_JNIENV();
	jstring jstr = JNU_NewStringNative(jvmenv,message.c_str());

	jvmenv->CallStaticVoidMethod(m_pcClass,m_pcMethodWarn,jstr);
	jvmenv->DeleteLocalRef(jstr);
}	

// ------------------------------------------------------------------------------------------------
void JNILogDispatcher::error(const std::string &message)
{
	JNIEnv* jvmenv = AIJ_GET_JNIENV();
	jstring jstr = JNU_NewStringNative(jvmenv,message.c_str());

	jvmenv->CallStaticVoidMethod(m_pcClass,m_pcMethodError,jstr);
	jvmenv->DeleteLocalRef(jstr);
}
