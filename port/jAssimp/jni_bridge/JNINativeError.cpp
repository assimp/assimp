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

#include "JNIEnvironment.h"
#include "JNILogger.h"

using namespace Assimp;

namespace Assimp	{
namespace JNIBridge		{

// ------------------------------------------------------------------------------------------------
JNIEnvironment::_assimp::_NativeException::Initialize()
{
	// get a handle to the JNI context for this thread
	JNIEnv* pc = JJNIEnvironment::Get()->GetThread()->m_pcEnv;

	// load a handle to the class
	if(!(this->Class = pc->FindClass("assimp.NativeException")))
	{
		// if this fails we have no exception class we could use ...
		// throw an java.lang.Error instance
		this->Class = pc->FindClass("java.lang.Exception");
		pc->ThrowNew(this->Class,"Unable to load class assimp.NativeException (severe failure!)");
		this->Class = NULL;
	}
}


// ------------------------------------------------------------------------------------------------
void JNIEnvironment::ThrowNativeError(const char* msg /*= NULL*/)
{
	// get a handle to the JNI context for this thread
	JNIEnv* pc = this->GetThread()->m_pcEnv;

	// throw a new NativeException
	pc->ThrowNew(this->assimp.NativeException.Class,
		msg ? msg 
		: "An unspecified error occured in the native interface to Assimp.");
}