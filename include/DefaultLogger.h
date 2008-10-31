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

#if (!defined AI_DEFAULTLOGGER_H_INCLUDED)
#define AI_DEFAULTLOGGER_H_INCLUDED

#include "Logger.h"
#include "NullLogger.h"
#include <vector>

namespace Assimp
{
// ---------------------------------------------------------------------------
class IOStream;
struct LogStreamInfo;


// ---------------------------------------------------------------------------
/**	@class	DefaultLogger
 *	@brief	Default logging implementation. The logger writes into a file. 
 *	The name can be set by creating the logger. If no filename was specified 
 *	the logger will use the standard out and error streams.
 */
class ASSIMP_API DefaultLogger :
	public Logger
{
public:
	/**	@brief	Creates a custom logging instance (DefaultLogger)
	 *	@param	name		Name for logfile
	 *	@param	severity	Log severity, VERBOSE will activate debug messages
	 *
	 * This replaces the default NullLogger with a DefaultLogger instance.
	 */
	static Logger *create(const std::string &name, LogSeverity severity);

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
	
	/**	@brief	Getter for singleton instance
	 *	@return	Only instance. This is never null, but it could be a 
	 *  NullLogger. Use isNullLogger to check this.
	 */
	static Logger *get();

	/** @brief  Return whether a default NullLogger is currently active
	 *  @return true if the current logger id a NullLogger.
	 *  Use create() or set() to setup a custom logger.
	 */
	static bool isNullLogger();
	
	/**	@brief	Will kill the singleton instance and setup a NullLogger as
        logger	*/
	static void kill();

	/**	@brief	Logs debug infos, only been written when severity level VERBOSE is set */
	void debug(const std::string &message);

	/**	@brief	Logs an info message */
	void info(const std::string &message);

	/**	@brief	Logs a warning message */
	void warn(const std::string &message);
	
	/**	@brief	Logs an error message */
	void error(const std::string &message);

	/**	@drief	Severity setter	*/
	void setLogSeverity(LogSeverity log_severity);
	
	/**	@brief	Detach a still attached stream from logger */
	void attachStream(LogStream *pStream, unsigned int severity);

	/**	@brief	Detach a still attached stream from logger */
	void detatchStream(LogStream *pStream, unsigned int severity);

private:
	/**	@brief	Constructor
	 *	@param	name		Name for logfile, keep this empty to use std::cout and std::cerr
	 *	@param	severity	Severity of logger
	 */
	DefaultLogger(const std::string &name, LogSeverity severity);
	
	/**	@brief	Destructor	*/
	~DefaultLogger();

	/**	@brief	Writes message into a file	*/
	void writeToStreams(const std::string &message, ErrorSeverity ErrorSev );

	/**	@brief	Returns the thread id.
	 *	@remark	This is an OS specific feature, if not supported, a zero will be returned.
	 */
	std::string getThreadID();

private:
	//	Aliases for stream container
	typedef std::vector<LogStreamInfo*>	StreamArray;
	typedef std::vector<LogStreamInfo*>::iterator StreamIt;
	typedef std::vector<LogStreamInfo*>::const_iterator ConstStreamIt;

	//!	only logging instance
	static Logger *m_pLogger;
	static NullLogger s_pNullLogger;

	//!	Logger severity
	LogSeverity m_Severity;
	//!	Attached streams
	StreamArray	m_StreamArray;
	//!	Array with default streams
	std::vector<LogStream*> m_Streams;

	bool noRepeatMsg;
	std::string lastMsg;
};
// ---------------------------------------------------------------------------

} // Namespace Assimp

#endif // !! AI_DEFAULTLOGGER_H_INCLUDED
