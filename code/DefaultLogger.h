#include "Logger.h"
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
class DefaultLogger :
	public Logger
{
public:
	/**	@brief	Creates the only logging instance
	 *	@param	name		Name for logfile
	 *	@param	severity	Log severity, VERBOSE will activate debug messages
	 */
	static Logger *create(const std::string &name, LogSeverity severity);
	
	/**	@brief	Getter for singleton instance
	 *	@return	Only instance
	 */
	static Logger *get();
	
	/**	@brief	Will kill the singleton instance	*/
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

private:
	//	Aliases for stream container
	typedef std::vector<LogStreamInfo*>	StreamArray;
	typedef std::vector<LogStreamInfo*>::iterator StreamIt;
	typedef std::vector<LogStreamInfo*>::const_iterator ConstStreamIt;

	//!	only logging instance
	static DefaultLogger *m_pLogger;
	//!	Logger severity
	LogSeverity m_Severity;
	//!	Attached streams
	StreamArray	m_StreamArray;
	//!	Array with default streams
	std::vector<LogStream*> m_Streams;
};
// ---------------------------------------------------------------------------

} // Namespace Assimp
