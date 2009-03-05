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

/** @file DefaultLogger.h
*/

#ifndef INCLUDED_AI_DEFAULTLOGGER
#define INCLUDED_AI_DEFAULTLOGGER

#include "Logger.h"
#include "LogStream.h"
#include "NullLogger.h"
#include <vector>

namespace Assimp	{
// ------------------------------------------------------------------------------------
class IOStream;
struct LogStreamInfo;

//! Default log file name
#define ASSIMP_DEFAULT_LOG_NAME "AssimpLog.txt"

// ------------------------------------------------------------------------------------
/**  @class	DefaultLogger
 *	 @brief	Default logging implementation. 
 *
 *  todo .... move static stuff to Logger where it belongs to.
 */
class ASSIMP_API DefaultLogger :
	public Logger
{
public:

	/** @brief	Creates a default logging instance (DefaultLogger)
	 *	@param	name		Name for log file. Only valid in combination
	 *                      with the DLS_FILE flag. 
	 *	@param	severity	Log severity, VERBOSE will activate debug messages
	 *  @param  defStreams  Default log streams to be attached. Bitwise
	 *                      combination of the DefaultLogStreams enumerated
	 *                      values. If DLS_FILE is specified, but an empty
	 *                      string is passed for 'name' no log file is created.
	 *  @param  io          IOSystem to be used to open external files (such as the 
	 *                      log file). Pass NULL for the default implementation.
	 *
	 * This replaces the default NullLogger with a DefaultLogger instance.
	 */
	static Logger *create(const char* name = ASSIMP_DEFAULT_LOG_NAME,
		LogSeverity severity    = NORMAL,
		unsigned int defStreams = DLS_DEBUGGER | DLS_FILE,
		IOSystem* io		    = NULL);

	/** @brief	Setup a custom implementation of the Logger interface as
	 *  default logger. 
	 *
	 *  Use this if the provided DefaultLogger class doesn't fit into
	 *  your needs. If the provided message formatting is OK for you,
	 *  it is easier to use create() to create a DefaultLogger and to attach
	 *  your own custom output streams to it than using this method.
	 *  @param logger Pass NULL to setup a default NullLogger
	 */
	static void set (Logger *logger);
	
	/** @brief	Getter for singleton instance
	 *	 @return	Only instance. This is never null, but it could be a 
	 *  NullLogger. Use isNullLogger to check this.
	 */
	static Logger *get();

	/** @brief  Return whether a default NullLogger is currently active
	 *  @return true if the current logger is a NullLogger.
	 *  Use create() or set() to setup a custom logger.
	 */
	static bool isNullLogger();
	
	/** @brief	Will kill the singleton instance and setup a NullLogger as logger */
	static void kill();
	
	/**	@brief	Attach a stream to the logger. */
	/* override */ bool attachStream(LogStream *pStream,
		unsigned int severity);

	/**	@brief	Detach a still attached stream from logger */
	/* override */ bool detatchStream(LogStream *pStream, 
		unsigned int severity);

private:

	/**	@brief	Logs debug infos, only been written when severity level VERBOSE is set */
	/* override */ void OnDebug(const char* message);

	/**	@brief	Logs an info message */
	/* override */ void OnInfo(const char*  message);

	/**	@brief	Logs a warning message */
	/* override */ void OnWarn(const char*  message);
	
	/**	@brief	Logs an error message */
	/* override */ void OnError(const char* message);


	/** @brief	Private construction for internal use by create().
	 *  @param severity Logging granularity
	 */
	DefaultLogger(LogSeverity severity);
	
	/**	@brief	Destructor	*/
	~DefaultLogger();

	/**	@brief Writes a message to all streams */
	void WriteToStreams(const char* message, ErrorSeverity ErrorSev );

	/** @brief	Returns the thread id.
	 *	@remark	This is an OS specific feature, if not supported, a zero will be returned.
	 */
	unsigned int GetThreadID();

private:
	//	Aliases for stream container
	typedef std::vector<LogStreamInfo*>	StreamArray;
	typedef std::vector<LogStreamInfo*>::iterator StreamIt;
	typedef std::vector<LogStreamInfo*>::const_iterator ConstStreamIt;

	//!	only logging instance
	static Logger *m_pLogger;
	static NullLogger s_pNullLogger;

	//!	Attached streams
	StreamArray	m_StreamArray;

	bool noRepeatMsg;
	char lastMsg[MAX_LOG_MESSAGE_LENGTH*2];
	size_t lastLen;
};
// ------------------------------------------------------------------------------------

} // Namespace Assimp

#endif // !! INCLUDED_AI_DEFAULTLOGGER
