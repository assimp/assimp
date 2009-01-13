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

/** @file LogStream.h
 *  @brief Abstract base class 'LogStream', representing an output log stream.
 */

#ifndef INCLUDED_AI_LOGSTREAM_H
#define INCLUDED_AI_LOGSTREAM_H

#include "aiTypes.h"

namespace Assimp	{
class IOSystem;

// ------------------------------------------------------------------------------------
/** @enum  DefaultLogStreams
 *  @brief Enumerates default log streams supported by DefaultLogger
 *
 *  These streams can be allocated using LogStream::createDefaultStream.
 */
enum DefaultLogStreams	
{
	// Stream the log to a file
	DLS_FILE = 0x1,

	// Stream the log to std::cout
	DLS_COUT = 0x2,

	// Stream the log to std::cerr
	DLS_CERR = 0x4,

	// MSVC only: Stream the log the the debugger
	DLS_DEBUGGER = 0x8
};

// ------------------------------------------------------------------------------------
/** @class	LogStream
 *	 @brief	Abstract interface for log stream implementations.
 *
 *  Several default implementations are provided, see DefaultLogStreams for more
 *  details. In most cases it shouldn't be necessary to write a custom log stream.
 */
class ASSIMP_API LogStream : public Intern::AllocateFromAssimpHeap
{
protected:
	/** @brief	Default constructor	*/
	LogStream();

public:
	/** @brief	Virtual destructor	*/
	virtual ~LogStream();

	/** @brief	Overwrite this for your own output methods
	 *  @param  message Message to be written
  	 */
	virtual void write(const std::string &message) = 0;

	/** @brief Creates a default log stream
	 *  @param streams Type of the default stream
	 *  @param name For DLS_FILE: name of the output file
	 *  @param  io  For DLS_FILE: IOSystem to be used to open the output file.
	 *              Pass NULL for the default implementation.
	 *  @return New LogStream instance - you're responsible for it's destruction!
	 */
	static LogStream* createDefaultStream(DefaultLogStreams	streams,
		const std::string& name = "AssimpLog.txt",
		IOSystem* io			= NULL);
};

// ------------------------------------------------------------------------------------
//	Default constructor
inline LogStream::LogStream()
{
	// empty
}

// ------------------------------------------------------------------------------------
//	Virtual destructor
inline LogStream::~LogStream()
{
	// empty
}

// ------------------------------------------------------------------------------------
} // Namespace Assimp

#endif
