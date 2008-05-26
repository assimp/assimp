/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

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

----------------------------------------------------------------------
*/

#if (!defined AI_JNIENVIRONMENT_H_INCLUDED)
#define AI_JNIENVIRONMENT_H_INCLUDED


#include <boost/thread/thread.hpp>
#include <boost/thread/tss.hpp>

#include <vector>
#include <jni.h>

#include "../../include/aiAssert.h"

namespace Assimp	{
namespace JNIBridge		{


// ---------------------------------------------------------------------------
/**	@class	JNIThreadData
 *	@brief	Manages a list of JNI data structures that are
 *  private to a thread.
 */
struct JNIThreadData
{
	//! Default constructor
	JNIThreadData() : m_pcEnv(NULL), m_iNumRef(1) {}

	//! Construction from an existing JNIEnv
	JNIThreadData(JNIEnv* env) : m_pcEnv(env), m_iNumRef(1) {}

	//! JNI environment, is attached to the thread
	JNIEnv* m_pcEnv;

	//! Number of Importer instances that have been
	//! created by this thread
	unsigned int m_iNumRef;
};


// ---------------------------------------------------------------------------
/**	@class	JNIEnvironment
 *	@brief	Helper class to manage the JNI environment for multithreaded
 *  use of the library.
 */
class JNIEnvironment
{
private:

	JNIEnvironment() : m_iRefCnt(1) {}

public:

	//! Create the JNI environment class
	//! refcnt = 1
	static JNIEnvironment* Create()
	{
		if (NULL == s_pcEnv)
		{
			s_pcEnv = new JNIEnvironment();
		}
		else s_pcEnv->AddRef();
		return s_pcEnv;
	}

	//! static getter for the singleton instance
	//! doesn't hange the reference counter
	static JNIEnvironment* Get()
	{
		ai_assert(NULL != s_pcEnv);
		return s_pcEnv;
	}

	//! COM-style reference counting mechanism
	unsigned int AddRef()
	{
		return ++this->m_iRefCnt;
	}

	//! COM-style reference counting mechanism
	unsigned int Release()
	{
		unsigned int iNew = --this->m_iRefCnt;
		if (0 == iNew)delete this;
		return iNew;
	}

	//! Attach to the current thread
	bool AttachToCurrentThread (JNIEnv* pcEnv);

	//! Detach from the current thread
	bool DetachFromCurrentThread ();

	//! Get the thread local data of the current thread
	JNIThreadData* GetThread();

public:

	//! Handle to the java.lang.String class
	static jclass Class_java_lang_String;

	//! Handle to the java.lang.String.getBytes() class
	static jmethodID MID_String_getBytes;

	//! Handle to the java.lang.String.String(byte[]) c'tor
	static jmethodID MID_String_init;

private:

	//! Singleton instance
	static JNIEnvironment* s_pcEnv;

	//! TLS data 
	boost::thread_specific_ptr<JNIThreadData> ptr;

	//! Reference counter of the whole class
	unsigned int m_iRefCnt;
};

};};


// ---------------------------------------------------------------------------
/** @brief Helper function to create a java.lang.String from
 *  a native char*.
 *
 * This function has been taken from
 * http://java.sun.com/docs/books/jni/html/other.html#26021
 * I am not sure whether it is really necessary, but I trust the source
 */
jstring JNU_NewStringNative(JNIEnv *env, const char *str);


// ---------------------------------------------------------------------------
/** @brief Helper function to create a char* from
 *  a managed jstring
 *
 * This function has been taken from
 * http://java.sun.com/docs/books/jni/html/other.html#26021
 * I am not sure whether it is really necessary, but I trust the source
 */
char* JNU_GetStringNativeChars(JNIEnv *env, jstring jstr);

#endif //! AI_JNIENVIRONMENT_H_INCLUDED


