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
void JNIEnvironment::_assimp::_NativeException::Initialize()
{
	// get a handle to the JNI context for this thread
	JNIEnv* pc = JNIEnvironment::Get()->GetThread()->m_pcEnv;

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
}}