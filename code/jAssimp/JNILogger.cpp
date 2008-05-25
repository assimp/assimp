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


// include assimp
#include "../../include/aiTypes.h"
#include "../../include/aiMesh.h"
#include "../../include/aiAnim.h"
#include "../../include/aiScene.h"
#include "../../include/aiAssert.h"
#include "../../include/aiPostProcess.h"
#include "../../include/assimp.hpp"

#include "../DefaultLogger.h"

#include "JNILogger.h"

using namespace Assimp;


namespace Assimp	{
namespace JNIBridge		{


// ------------------------------------------------------------------------------------------------
void JNILogDispatcher::SetJNIEnvironment(JNIEnv* ptr) 
{
	// there is much error handling code in this function.
	// However, it is not impossible that the jAssimp package 
	// loaded by the JVM is incomplete ...

	jclass java_lang_Exception = this->GetJNIEnv()->FindClass("java.lang.Exception");

	// get a handle to the assimp.DefaultLogger class
	this->m_pcJNIEnv = ptr;
	if( NULL == (this->m_pcClass = this->GetJNIEnv()->FindClass("assimp.DefaultLogger")))
	{
		this->GetJNIEnv()->ThrowNew(java_lang_Exception,
			"Unable to get class handle to assimp.DefaultLogger");
		return;
	}
	// get handles to the logging functions
	if( NULL == (this->m_pcMethodError = this->GetJNIEnv()->GetStaticMethodID(
		this->m_pcClass,"_NativeCallWriteError","(Ljava/lang/String;)V")))
	{
		this->GetJNIEnv()->ThrowNew(java_lang_Exception,
			"Unable to get class handle to assimp.DefaultLogger._NativeCallWriteError()");
		return;
	}
	if( NULL == (this->m_pcMethodWarn = this->GetJNIEnv()->GetStaticMethodID(
		this->m_pcClass,"_NativeCallWriteWarn","(Ljava/lang/String;)V")))
	{
		this->GetJNIEnv()->ThrowNew(java_lang_Exception,
			"Unable to get class handle to assimp.DefaultLogger._NativeCallWriteWarn()");
		return;
	}
	if( NULL == (this->m_pcMethodInfo = this->GetJNIEnv()->GetStaticMethodID(
		this->m_pcClass,"_NativeCallWriteInfo","(Ljava/lang/String;)V")))
	{
		this->GetJNIEnv()->ThrowNew(java_lang_Exception,
			"Unable to get class handle to assimp.DefaultLogger._NativeCallWriteInfo()");
		return;
	}
	if( NULL == (this->m_pcMethodDebug = this->GetJNIEnv()->GetStaticMethodID(
		this->m_pcClass,"_NativeCallWriteDebug","(Ljava/lang/String;)V")))
	{
		this->GetJNIEnv()->ThrowNew(java_lang_Exception,
			"Unable to get class handle to assimp.DefaultLogger._NativeCallWriteDebug()");
	}
}
// ------------------------------------------------------------------------------------------------
void JNILogDispatcher::debug(const std::string &message)
{
	
}
// ------------------------------------------------------------------------------------------------
void JNILogDispatcher::info(const std::string &message)
{
}
// ------------------------------------------------------------------------------------------------
void JNILogDispatcher::warn(const std::string &message)
{
}	
// ------------------------------------------------------------------------------------------------
void JNILogDispatcher::error(const std::string &message)
{
}


};};

#endif // jni