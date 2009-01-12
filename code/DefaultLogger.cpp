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

#include "AssimpPCH.h"
#include "DefaultIOSystem.h"

// Default log streams
#include "Win32DebugLogStream.h"
#include "StdOStreamLogStream.h"
#include "FileLogStream.h"

namespace Assimp	{

// ----------------------------------------------------------------------------------
NullLogger DefaultLogger::s_pNullLogger;
Logger *DefaultLogger::m_pLogger = &DefaultLogger::s_pNullLogger;

// ----------------------------------------------------------------------------------
//
struct LogStreamInfo
{
	unsigned int m_uiErrorSeverity;
	LogStream *m_pStream;

	// Constructor
	LogStreamInfo( unsigned int uiErrorSev, LogStream *pStream ) :
		m_uiErrorSeverity( uiErrorSev ),
		m_pStream( pStream )
	{
		// empty
	}
	
	// Destructor
	~LogStreamInfo()
	{
		// empty
	}
};

// ----------------------------------------------------------------------------------
// Construct a default log stream
LogStream* LogStream::createDefaultStream(DefaultLogStreams	streams,
	const std::string& name /*= "AssimpLog.txt"*/,
	IOSystem* io		    /*= NULL*/)
{
	switch (streams)	
	{
		// This is a platform-specific feature
	case DLS_DEBUGGER:
#ifdef WIN32
		return new Win32DebugLogStream();
#else
		return NULL;
#endif

		// Platform-independent default streams
	case DLS_CERR:
		return new StdOStreamLogStream(std::cerr);
	case DLS_COUT:
		return new StdOStreamLogStream(std::cout);
	case DLS_FILE:
		return (name.size() ? new FileLogStream(name,io) : NULL);
	default:
		// We don't know this default log stream, so raise an assertion
		ai_assert(false);

	};

	// For compilers without dead code path detection
	return NULL;
}

// ----------------------------------------------------------------------------------
//	Creates the only singleton instance
Logger *DefaultLogger::create(const std::string &name /*= "AssimpLog.txt"*/,
	LogSeverity severity    /*= NORMAL*/,
	unsigned int defStreams /*= DLS_DEBUGGER | DLS_FILE*/,
	IOSystem* io		    /*= NULL*/)
{
	if (m_pLogger && !isNullLogger() )
		delete m_pLogger;

	m_pLogger = new DefaultLogger( severity );

	// Attach default log streams
	// Stream the log to the MSVC debugger?
	if (defStreams & DLS_DEBUGGER)
		m_pLogger->attachStream( LogStream::createDefaultStream(DLS_DEBUGGER));

	// Stream the log to COUT?
	if (defStreams & DLS_COUT)
		m_pLogger->attachStream( LogStream::createDefaultStream(DLS_COUT));

	// Stream the log to CERR?
	if (defStreams & DLS_CERR)
		 m_pLogger->attachStream( LogStream::createDefaultStream(DLS_CERR));
	
	// Stream the log to a file
	if (defStreams & DLS_FILE && !name.empty())
		m_pLogger->attachStream( LogStream::createDefaultStream(DLS_FILE,name,io));

	return m_pLogger;
}

// ----------------------------------------------------------------------------------
void DefaultLogger::set( Logger *logger )
{
	if (!logger)logger = &s_pNullLogger;
	if (m_pLogger && !isNullLogger() )
		delete m_pLogger;

	DefaultLogger::m_pLogger = logger;
}

// ----------------------------------------------------------------------------------
bool DefaultLogger::isNullLogger()
{
	return m_pLogger == &s_pNullLogger;
}

// ----------------------------------------------------------------------------------
//	Singleton getter
Logger *DefaultLogger::get()
{
	return m_pLogger;
}

// ----------------------------------------------------------------------------------
//	Kills the only instance
void DefaultLogger::kill()
{
	if (m_pLogger != &s_pNullLogger)return;
	delete m_pLogger;
	m_pLogger = &s_pNullLogger;
}

// ----------------------------------------------------------------------------------
//	Debug message
void DefaultLogger::debug( const std::string &message )
{
	if ( m_Severity == Logger::NORMAL )
		return;

	const std::string msg( "Debug, T" + getThreadID() + ": " + message );
	writeToStreams( msg, Logger::DEBUGGING );
}

// ----------------------------------------------------------------------------------
//	Logs an info
void DefaultLogger::info( const std::string &message )
{
	const std::string msg( "Info,  T" + getThreadID() + ": " + message );
	writeToStreams( msg , Logger::INFO );
}

// ----------------------------------------------------------------------------------
//	Logs a warning
void DefaultLogger::warn( const std::string &message )
{
	const std::string msg( "Warn,  T" + getThreadID() + ": "+ message );
	writeToStreams( msg, Logger::WARN );
}

// ----------------------------------------------------------------------------------
//	Logs an error
void DefaultLogger::error( const std::string &message )
{
	const std::string msg( "Error, T"+ getThreadID() + ": " + message );
	writeToStreams( msg, Logger::ERR );
}

// ----------------------------------------------------------------------------------
//	Severity setter
void DefaultLogger::setLogSeverity( LogSeverity log_severity )
{
	m_Severity = log_severity;
}

// ----------------------------------------------------------------------------------
//	Attachs a new stream
void DefaultLogger::attachStream( LogStream *pStream, unsigned int severity )
{
	if (!pStream)
		return;

	// fix (Aramis)
	if (0 == severity)
	{
		severity = Logger::INFO | Logger::ERR | Logger::WARN | Logger::DEBUGGING;
	}

	for ( StreamIt it = m_StreamArray.begin();
		it != m_StreamArray.end();
		++it )
	{
		if ( (*it)->m_pStream == pStream )
		{
			(*it)->m_uiErrorSeverity |= severity;
			return;
		}
	}
	
	LogStreamInfo *pInfo = new LogStreamInfo( severity, pStream );
	m_StreamArray.push_back( pInfo );
}

// ----------------------------------------------------------------------------------
//	Detatch a stream
void DefaultLogger::detatchStream( LogStream *pStream, unsigned int severity )
{
	if (!pStream)
		return;

	// fix (Aramis)
	if (0 == severity)
	{
		severity = Logger::INFO | Logger::ERR | Logger::WARN | Logger::DEBUGGING;
	}
	
	for ( StreamIt it = m_StreamArray.begin();
		it != m_StreamArray.end();
		++it )
	{
		if ( (*it)->m_pStream == pStream )
		{
			(*it)->m_uiErrorSeverity &= ~severity;
			if ( (*it)->m_uiErrorSeverity == 0 )
			{
				m_StreamArray.erase( it );
				break;
			}
		}
	}
}

// ----------------------------------------------------------------------------------
//	Constructor
DefaultLogger::DefaultLogger(LogSeverity severity) 

	:	m_Severity	( severity )
	,	noRepeatMsg	(false)
{}

// ----------------------------------------------------------------------------------
//	Destructor
DefaultLogger::~DefaultLogger()
{
	for ( StreamIt it = m_StreamArray.begin(); it != m_StreamArray.end(); ++it )
		delete *it;
}

// ----------------------------------------------------------------------------------
//	Writes message to stream
void DefaultLogger::writeToStreams(const std::string &message, 
	ErrorSeverity ErrorSev )
{
	if ( message.empty() )
		return;

	std::string s;

	// Check whether this is a repeated message
	if (message == lastMsg)
	{
		if (!noRepeatMsg)
		{
			noRepeatMsg = true;
			s = "Skipping one or more lines with the same contents\n";
		}
		else return;
	}
	else
	{
		lastMsg = s = message;
		noRepeatMsg = false;

		s.append("\n");
	}
	for ( ConstStreamIt it = m_StreamArray.begin();
		it != m_StreamArray.end();
		++it)
	{
		if ( ErrorSev & (*it)->m_uiErrorSeverity )
			(*it)->m_pStream->write( s);
	}
}

// ----------------------------------------------------------------------------------
//	Returns thread id, if not supported only a zero will be returned.
std::string DefaultLogger::getThreadID()
{
	std::string thread_id( "0" );
#ifdef WIN32
	HANDLE hThread = ::GetCurrentThread();
	if ( hThread )
	{
		std::stringstream thread_msg;
		thread_msg << ::GetCurrentThreadId() /*<< " "*/;
		return thread_msg.str();
	}
	else
		return thread_id;
#else
	return thread_id;
#endif
}

// ----------------------------------------------------------------------------------

} // !namespace Assimp
