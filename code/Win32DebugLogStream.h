#ifndef AI_WIN32DEBUGLOGSTREAM_H_INC
#define AI_WIN32DEBUGLOGSTREAM_H_INC

#ifdef WIN32

#include <assimp/LogStream.hpp>
#include "windows.h"

namespace Assimp    {

// ---------------------------------------------------------------------------
/** @class  Win32DebugLogStream
 *  @brief  Logs into the debug stream from win32.
 */
class Win32DebugLogStream :
    public LogStream
{
public:
    /** @brief  Default constructor */
    Win32DebugLogStream();

    /** @brief  Destructor  */
    ~Win32DebugLogStream();

    /** @brief  Writer  */
    void write(const char* messgae);
};

// ---------------------------------------------------------------------------
//  Default constructor
inline Win32DebugLogStream::Win32DebugLogStream()
{}

// ---------------------------------------------------------------------------
//  Default constructor
inline Win32DebugLogStream::~Win32DebugLogStream()
{}

// ---------------------------------------------------------------------------
//  Write method
inline void Win32DebugLogStream::write(const char* message)
{
    OutputDebugStringA( message);
}

// ---------------------------------------------------------------------------
}   // Namespace Assimp

#endif // ! WIN32
#endif // guard
