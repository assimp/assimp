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

/** @file Base class of all import post processing steps */
#ifndef AI_BASEPROCESS_H_INC
#define AI_BASEPROCESS_H_INC

struct aiScene;

namespace Assimp
{

// ---------------------------------------------------------------------------
/** The BaseProcess defines a common interface for all post processing steps.
 * A post processing step is run after a successful import if the caller
 * specified the corresponding flag when calling ReadFile(). Enum #aiPostProcessSteps
 * defines which flags are available. 
 * After a successful import the Importer iterates over its internal array of processes
 * and calls IsActive() on each process to evaluate if the step should be executed.
 * If the function returns true, the class' Execute() function is called subsequently.
 */
class BaseProcess
{
	friend class Importer;

public:
	/** Constructor to be privately used by Importer */
	BaseProcess();

	/** Destructor, private as well */
	virtual ~BaseProcess();

public:
	// -------------------------------------------------------------------
	/** Returns whether the processing step is present in the given flag field.
	 * @param pFlags The processing flags the importer was called with. A bitwise
	 *   combination of #aiPostProcessSteps.
	 * @return true if the process is present in this flag fields, false if not.
	*/
	virtual bool IsActive( unsigned int pFlags) const = 0;

	// -------------------------------------------------------------------
	/** Executes the post processing step on the given imported data.
	* At the moment a process is not supposed to fail.
	* @param pScene The imported data to work at.
	*/
	virtual void Execute( aiScene* pScene) = 0;
};

/** Constructor, dummy implementation to keep the compiler from complaining */
inline BaseProcess::BaseProcess()
{
}

/** Destructor */
inline BaseProcess::~BaseProcess()
{
}

} // end of namespace Assimp

#endif // AI_BASEPROCESS_H_INC