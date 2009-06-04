
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
void JNIEnvironment::_assimp::_NativeException::Initialize()
{
	// get a handle to the JNI context for this thread
	JNIEnv* pc = JNIEnvironment::Get()->GetThread()->m_pcEnv;

	// and load a handle to the class
	if(!(Class = pc->FindClass("assimp.NativeException")))	{
		pc->ThrowNew(pc->FindClass("java.lang.Exception"),"Unable to load class assimp.NativeException"); // :-)
	}
}

// ------------------------------------------------------------------------------------------------
void JNIEnvironment::ThrowNativeError(const std::string& msg)
{
	// get a handle to the JNI context for this thread ...
	JNIEnv* pc = GetThread()->m_pcEnv;

	// and throw a new assimp.NativeException
	pc->ThrowNew(assimp.NativeException.Class,msg.c_str());
}
