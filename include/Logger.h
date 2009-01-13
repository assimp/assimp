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

/** @file Logger.h
 *  @brief Abstract base class 'Logger', base of the logging system. 
 */

#ifndef INCLUDED_AI_LOGGER_H
#define INCLUDED_AI_LOGGER_H

#include "aiTypes.h"
namespace Assimp	{

class LogStream;

// ----------------------------------------------------------------------------------
/**	@class	Logger
 *	@brief	Abstract interface for logger implementations.
 *  Assimp provides a default implementation ('DefaultLogger').
 */
class ASSIMP_API Logger : public Intern::AllocateFromAssimpHeap
{
public:
	/**	@enum	LogSeverity
	 *	@brief	Log severity to describe the granularity of logging.
	 *
	 *  This is a general property of a Logger instance, NORMAL means
	 *  that debug messages are rejected immediately.
	 */
	enum LogSeverity
	{
		NORMAL,		//!< Normal granularity of logging
		VERBOSE		//!< Debug infos will be logged, too
	};

	/**	@enum	ErrorSeverity
	 *	@brief	Description for severity of a log message.
	 *
	 *  Every LogStream has a bitwise combination of these flags.
	 *  A LogStream doesn't receive any messages of a specific type
	 *  if it doesn't specify the corresponding ErrorSeverity flag.
	 */
	enum ErrorSeverity
	{
		DEBUGGING	= 1,	//!< Debug log message
		INFO		= 2, 	//!< Info log message
		WARN		= 4,	//!< Warn log message
		ERR			= 8		//!< Error log message
	};

public:
	/** @brief	Virtual destructor */
	virtual ~Logger();

	/** @brief	Writes a debug message
	 *	 @param	message		Debug message
	 */
	virtual void debug(const std::string &message)= 0;

	/** @brief	Writes a info message
	 *	 @param	message		Info message
	 */
	virtual void info(const std::string &message) = 0;

	/** @brief	Writes a warning message
	 *	 @param	message		Warn message
	 */
	virtual void warn(const std::string &message) = 0;

	/** @brief	Writes an error message
	 *	 @param	message		Error message
	 */
	virtual void error(const std::string &message) = 0;

	/** @brief	Set a new log severity.
	 *	 @param	log_severity	New severity for logging
	 */
	virtual void setLogSeverity(LogSeverity log_severity) = 0;

	/** @brief	Attach a new logstream
	 *
	 *  The logger takes ownership of the stream and is responsible
	 *  for its destruction (which is done when the logger itself 
	 *  is destroyed). Call detachStream to detach a stream and to
	 *  gain ownership of it again.
	 *	 @param	pStream	 Logstream to attach
	 *  @param severity  Message filter, specified which types of log
	 *    messages are dispatched to the stream. Provide a bitwise
	 *    combination of the ErrorSeverity flags.
	 */
	virtual void attachStream(LogStream *pStream, 
		unsigned int severity = DEBUGGING | ERR | WARN | INFO) = 0;

	/** @brief	Detach a still attached stream from the logger (or 
	 *          modify the filter flags bits)
	 *	 @param	pStream	Logstream instance for detaching
	 *  @param severity Provide a bitwise combination of the ErrorSeverity
	 *    flags. This value is &~ed with the current flags of the stream,
	 *    if the result is 0 the stream is detached from the Logger and
	 *    the caller retakes the possession of the stream.
	 */
	virtual void detatchStream(LogStream *pStream, 
		unsigned int severity = DEBUGGING | ERR | WARN | INFO) = 0;

protected:
	/**	@brief	Default constructor	*/
	Logger();
};

// ----------------------------------------------------------------------------------
//	Default constructor
inline Logger::Logger()
{
	//	empty
}

// ----------------------------------------------------------------------------------
//	Virtual destructor
inline  Logger::~Logger()
{
	// empty
}
// ----------------------------------------------------------------------------------

} // Namespace Assimp

#endif // !! INCLUDED_AI_LOGGER_H
