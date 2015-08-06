#ifndef ASSIMP_FILELOGSTREAM_H_INC
#define ASSIMP_FILELOGSTREAM_H_INC

#include "../include/assimp/LogStream.hpp"
#include "../include/assimp/IOStream.hpp"
#include "DefaultIOSystem.h"

namespace Assimp    {

// ----------------------------------------------------------------------------------
/** @class  FileLogStream
 *  @brief  Logstream to write into a file.
 */
class FileLogStream :
    public LogStream
{
public:
    FileLogStream( const char* file, IOSystem* io = NULL );
    ~FileLogStream();
    void write( const char* message );

private:
    IOStream *m_pStream;
};

// ----------------------------------------------------------------------------------
//  Constructor
inline FileLogStream::FileLogStream( const char* file, IOSystem* io ) :
    m_pStream(NULL)
{
    if ( !file || 0 == *file )
        return;

    // If no IOSystem is specified: take a default one
    if (!io)
    {
        DefaultIOSystem FileSystem;
        m_pStream = FileSystem.Open( file, "wt");
    }
    else m_pStream = io->Open( file, "wt" );
}

// ----------------------------------------------------------------------------------
//  Destructor
inline FileLogStream::~FileLogStream()
{
    // The virtual d'tor should destroy the underlying file
    delete m_pStream;
}

// ----------------------------------------------------------------------------------
//  Write method
inline void FileLogStream::write( const char* message )
{
    if (m_pStream != NULL)
    {
        m_pStream->Write(message, sizeof(char), ::strlen(message));
        m_pStream->Flush();
    }
}

// ----------------------------------------------------------------------------------
} // !Namespace Assimp

#endif // !! ASSIMP_FILELOGSTREAM_H_INC
