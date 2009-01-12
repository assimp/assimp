#ifndef ASSIMP_FILELOGSTREAM_H_INC
#define ASSIMP_FILELOGSTREAM_H_INC

#include "../include/LogStream.h"
#include "../include/IOStream.h"

namespace Assimp	{


// ----------------------------------------------------------------------------------
/**	@class	FileLogStream
 *	@brief	Logstream to write into a file.
 */
class FileLogStream :
	public LogStream
{
public:
	FileLogStream( const std::string &strFileName, IOSystem* io = NULL );
	~FileLogStream();
	void write( const std::string &message );

private:
	IOStream *m_pStream;
};


// ----------------------------------------------------------------------------------
//	Constructor
inline FileLogStream::FileLogStream( const std::string &strFileName, IOSystem* io ) :
	m_pStream(NULL)
{
	if ( strFileName.empty() )
		return;

	const static std::string mode = "wt";

	// If no IOSystem is specified: take a default one
	if (!io)
	{
		DefaultIOSystem FileSystem;
		m_pStream = FileSystem.Open( strFileName, mode );
	}
	else m_pStream = io->Open( strFileName, mode );
}

// ----------------------------------------------------------------------------------
//	Destructor
inline FileLogStream::~FileLogStream()
{
	// The virtual d'tor should destroy the underlying file
	delete m_pStream;
}


// ----------------------------------------------------------------------------------
//	Write method
inline void FileLogStream::write( const std::string &message )
{
	if (m_pStream != NULL)
	{
		m_pStream->Write(message.c_str(), sizeof(char), message.size());
		m_pStream->Flush();
	}
}


// ----------------------------------------------------------------------------------

} // !Namespace Assimp

#endif // !! ASSIMP_FILELOGSTREAM_H_INC
