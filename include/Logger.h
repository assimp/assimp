#ifndef AI_LOGGER_H_INC
#define AI_LOGGER_H_INC

#include <string>

namespace Assimp
{

class LogStream;

// ---------------------------------------------------------------------------
/**	@class	Logger
 *	@brief	Abstract interface for logger implementations.
 */
class Logger
{
public:
	/**	@enum	LogSeverity
	 *	@brief	Log severity to descripe granuality of logging.
	 */
	enum LogSeverity
	{
		NORMAL,		//!< Normal granlality of logging
		VERBOSE		//!< Debug infos will be logged, too
	};

	/**	@enum	ErrorSeverity
	 *	@brief	Description for severity of a log message
	 */
	enum ErrorSeverity
	{
		DEBUGGING	= 1,	//!< Debug log message
		INFO		= 2, 	//!< Info log message
		WARN		= 4,	//!< Warn log message
		ERR			= 8		//!< Error log message
	};

public:
	/**	@brief	Virtual destructor */
	virtual ~Logger();

	/**	@brief	Writes a debug message
	 *	@param	message		Debug message
	 */
	virtual void debug(const std::string &message)= 0;

	/**	@brief	Writes a info message
	 *	@param	message		Info message
	 */
	virtual void info(const std::string &message) = 0;

	/**	@brief	Writes a warning message
	 *	@param	message		Warn message
	 */
	virtual void warn(const std::string &message) = 0;

	/**	@brief	Writes an error message
	 *	@param	message		Error message
	 */
	virtual void error(const std::string &message) = 0;

	/**	@brief	Set a new log severity.
	 *	@param	log_severity	New severity for logging
	 */
	virtual void setLogSeverity(LogSeverity log_severity) = 0;

	/**	@brief	Attach a new logstream
	 *	@param	pStream		Logstream to attach
	 */
	virtual void attachStream(LogStream *pStream, unsigned int severity) = 0;

	/**	@brief	Detach a still attached stream from logger
	 *	@param	pStream		Logstream instance for detatching
	 */
	virtual void detatchStream(LogStream *pStream, unsigned int severity) = 0;

protected:
	/**	@brief	Default constructor	*/
	Logger();
};
// ---------------------------------------------------------------------------
//	Default constructor
inline Logger::Logger()
{
	//	empty
}

// ---------------------------------------------------------------------------
//	Virtual destructor
inline  Logger::~Logger()
{
	// empty
}
// ---------------------------------------------------------------------------

} // Namespace Assimp

#endif
