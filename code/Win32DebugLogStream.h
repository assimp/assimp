#ifndef AI_WIN32DEBUGLOGSTREAM_H_INC
#define AI_WIN32DEBUGLOGSTREAM_H_INC

#include "../include/LogStream.h"

//#ifdef _MSC_VER
#ifdef WIN32
#include "Windows.h"
#endif

namespace Assimp
{
//#ifdef _MSC_VER
#ifdef WIN32

// ---------------------------------------------------------------------------
/**	@class	Win32DebugLogStream
 *	@brief	Logs into the debug stream from win32.
 */
class Win32DebugLogStream :
	public LogStream
{
public:
	/**	@brief	Default constructor	*/
	Win32DebugLogStream();

	/**	@brief	Destructor	*/
	~Win32DebugLogStream();
	
	/**	@brief	Writer	*/
	void write(const std::string &messgae);
};

// ---------------------------------------------------------------------------
//	Default constructor
inline Win32DebugLogStream::Win32DebugLogStream()
{
	// empty
}

// ---------------------------------------------------------------------------
//	Default constructor
inline Win32DebugLogStream::~Win32DebugLogStream()
{
	// empty
}

// ---------------------------------------------------------------------------
//	Write method
inline void Win32DebugLogStream::write(const std::string &message)
{
	OutputDebugString( message.c_str() );
}

// ---------------------------------------------------------------------------

#endif

}	// Namespace Assimp

#endif
