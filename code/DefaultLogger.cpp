#include "DefaultLogger.h"
#include "aiAssert.h"
#include "DefaultIOSystem.h"
#include "Win32DebugLogStream.h"
#include "IOStream.h"
#include "FileLogStream.h"

#include <iostream>

namespace Assimp
{
// ---------------------------------------------------------------------------
DefaultLogger *DefaultLogger::m_pLogger = NULL;

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
};
// ---------------------------------------------------------------------------
//	Creates the only singleton instance
Logger *DefaultLogger::create(const std::string &name, LogSeverity severity)
{
	ai_assert (NULL == m_pLogger);
	m_pLogger = new DefaultLogger( name, severity );
	
	return m_pLogger;
}

// ---------------------------------------------------------------------------
//	Singleton getter
Logger *DefaultLogger::get()
{
	ai_assert (NULL != m_pLogger);
	return m_pLogger;
}

// ---------------------------------------------------------------------------
//	Kills the only instance
void DefaultLogger::kill()
{
	ai_assert (NULL != m_pLogger);
	delete m_pLogger;
	m_pLogger = NULL;
}

// ---------------------------------------------------------------------------
//	Debug message
void DefaultLogger::debug(const std::string &message)
{
	if ( m_Severity == Logger::NORMAL )
		return;

	const std::string msg( "Debug: " + message );
	writeToStreams( msg, Logger::DEBUGGING );
}

// ---------------------------------------------------------------------------
//	Logs an info
void DefaultLogger::info(const std::string &message)
{
	const std::string msg( "Info: " + message );
	writeToStreams( msg , Logger::INFO );
}

// ---------------------------------------------------------------------------
//	Logs a warning
void DefaultLogger::warn( const std::string &message )
{
	const std::string msg( "Warn:  " + message );
	writeToStreams( msg, Logger::WARN );
}

// ---------------------------------------------------------------------------
//	Logs an error
void DefaultLogger::error( const std::string &message )
{
	const std::string msg( "Error:  " + message );
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
	
	LogStreamInfo *pInfo = new LogStreamInfo( severity, pStream );
	m_StreamArray.push_back( pInfo );
}

// ---------------------------------------------------------------------------
//	Detatch a stream
void DefaultLogger::detatchStream( LogStream *pStream, unsigned int severity )
{
	ai_assert ( NULL != pStream );
	
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
	if (name.empty())
		return;
	
	m_Streams.push_back( new FileLogStream( name ) );
	m_Streams.push_back( new Win32DebugLogStream() );
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

	for ( ConstStreamIt it = this->m_StreamArray.begin();
		it != m_StreamArray.end();
		++it)
	{
		if ( ErrorSev & (*it)->m_uiErrorSeverity )
		{
			(*it)->m_pStream->write( message );
		}
	}
	for (std::vector<LogStream*>::iterator it = m_Streams.begin();
		it != m_Streams.end();
		++it)
	{
		(*it)->write( message + std::string("\n"));
	}
}

// ---------------------------------------------------------------------------

} // Namespace Assimp
