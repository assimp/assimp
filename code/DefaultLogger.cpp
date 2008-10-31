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
#include "Win32DebugLogStream.h"
#include "FileLogStream.h"

namespace Assimp
{
// ---------------------------------------------------------------------------
NullLogger DefaultLogger::s_pNullLogger;
Logger *DefaultLogger::m_pLogger = &DefaultLogger::s_pNullLogger;

// ---------------------------------------------------------------------------
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
// ---------------------------------------------------------------------------
//	Creates the only singleton instance
Logger *DefaultLogger::create(const std::string &name, LogSeverity severity)
{
	if (m_pLogger && !isNullLogger() )
		delete m_pLogger;
	m_pLogger = new DefaultLogger( name, severity );
	
	return m_pLogger;
}
// ---------------------------------------------------------------------------
void DefaultLogger::set( Logger *logger )
{
	if (!logger)logger = &s_pNullLogger;
	if (m_pLogger && !isNullLogger() )
		delete m_pLogger;

	DefaultLogger::m_pLogger = logger;
}
// ---------------------------------------------------------------------------
bool DefaultLogger::isNullLogger()
{
	return m_pLogger == &s_pNullLogger;
}
// ---------------------------------------------------------------------------
//	Singleton getter
Logger *DefaultLogger::get()
{
	return m_pLogger;
}

// ---------------------------------------------------------------------------
//	Kills the only instance
void DefaultLogger::kill()
{
	if (m_pLogger != &s_pNullLogger)return;
	delete m_pLogger;
	m_pLogger = &s_pNullLogger;
}

// ---------------------------------------------------------------------------
//	Debug message
void DefaultLogger::debug( const std::string &message )
{
	if ( m_Severity == Logger::NORMAL )
		return;

	const std::string msg( "Debug, T" + getThreadID() + ": " + message );
	writeToStreams( msg, Logger::DEBUGGING );
}

// ---------------------------------------------------------------------------
//	Logs an info
void DefaultLogger::info( const std::string &message )
{
	const std::string msg( "Info,  T" + getThreadID() + ": " + message );
	writeToStreams( msg , Logger::INFO );
}

// ---------------------------------------------------------------------------
//	Logs a warning
void DefaultLogger::warn( const std::string &message )
{
	const std::string msg( "Warn,  T" + getThreadID() + ": "+ message );
	writeToStreams( msg, Logger::WARN );
}

// ---------------------------------------------------------------------------
//	Logs an error
void DefaultLogger::error( const std::string &message )
{
	const std::string msg( "Error, T"+ getThreadID() + ": " + message );
	writeToStreams( msg, Logger::ERR );
}

// ---------------------------------------------------------------------------
//	Severity setter
void DefaultLogger::setLogSeverity( LogSeverity log_severity )
{
	m_Severity = log_severity;
}

// ---------------------------------------------------------------------------
//	Attachs a new stream
void DefaultLogger::attachStream( LogStream *pStream, unsigned int severity )
{
	ai_assert ( NULL != pStream );

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

// ---------------------------------------------------------------------------
//	Detatch a stream
void DefaultLogger::detatchStream( LogStream *pStream, unsigned int severity )
{
	ai_assert ( NULL != pStream );

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
			unsigned int uiSev = (*it)->m_uiErrorSeverity;
			if ( severity & Logger::INFO ) 
				uiSev &= ( ~Logger::INFO );
			if ( severity & Logger::WARN ) 
				uiSev &= ( ~Logger::WARN );
			if ( severity & Logger::ERR ) 
				uiSev &= ( ~Logger::ERR );
			// fix (Aramis)
			if ( severity & Logger::DEBUGGING ) 
				uiSev &= ( ~Logger::DEBUGGING );

			(*it)->m_uiErrorSeverity = uiSev;
			
			if ( (*it)->m_uiErrorSeverity == 0 )
			{
				it = m_StreamArray.erase( it );
				break;
			}
		}
	}
}

// ---------------------------------------------------------------------------
//	Constructor
DefaultLogger::DefaultLogger( const std::string &name, LogSeverity severity ) :
	m_Severity( severity )
{
#ifdef WIN32
	m_Streams.push_back( new Win32DebugLogStream() );
#endif
	
	if (name.empty())
		return;
	m_Streams.push_back( new FileLogStream( name ) );

	noRepeatMsg = false;
}

// ---------------------------------------------------------------------------
//	Destructor
DefaultLogger::~DefaultLogger()
{
	for ( StreamIt it = m_StreamArray.begin(); 
		it != m_StreamArray.end();
		++it )
	{
		delete *it;
	}

	for (std::vector<LogStream*>::iterator it = m_Streams.begin();
		it != m_Streams.end();
		++it)
	{
		delete *it;
	}
	m_Streams.clear();
}

// ---------------------------------------------------------------------------
//	Writes message to stream
void DefaultLogger::writeToStreams(const std::string &message, 
								   ErrorSeverity ErrorSev )
{
	if ( message.empty() )
		return;

	std::string s;
	if (message == lastMsg)
	{
		if (!noRepeatMsg)
		{
			noRepeatMsg = true;
			s = "Skipping one or more lines with the same contents";
		}
		else return;
	}
	else
	{
		lastMsg = s = message;
		noRepeatMsg = false;
	}

	for ( ConstStreamIt it = m_StreamArray.begin();
		it != m_StreamArray.end();
		++it)
	{
		if ( ErrorSev & (*it)->m_uiErrorSeverity )
		{
			(*it)->m_pStream->write( s);
		}
	}
	for (std::vector<LogStream*>::iterator it = m_Streams.begin();
		it != m_Streams.end();
		++it)
	{
		(*it)->write( s + std::string("\n"));
	}
}

// ---------------------------------------------------------------------------
//	Returns thread id, if not supported only a zero will be returned.
std::string DefaultLogger::getThreadID()
{
	std::string thread_id( "0" );
#ifdef WIN32
	HANDLE hThread = GetCurrentThread();
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

// ---------------------------------------------------------------------------

} // Namespace Assimp
