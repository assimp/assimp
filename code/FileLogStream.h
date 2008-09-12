#ifndef ASSIMP_FILELOGSTREAM_H_INC
#define ASSIMP_FILELOGSTREAM_H_INC

#include "../include/LogStream.h"
#include "../include/IOStream.h"

namespace Assimp
{

// ---------------------------------------------------------------------------
/**	@class	FileLogStream
 *	@brief	Logstream to write into a file.
 */
class FileLogStream :
	public LogStream
{
public:
	FileLogStream( const std::string &strFileName );
	~FileLogStream();
	void write( const std::string &message );

private:
	IOStream *m_pStream;
};
// ---------------------------------------------------------------------------
//	Constructor
inline FileLogStream::FileLogStream( const std::string &strFileName ) :
	m_pStream(NULL)
{
	if ( strFileName.empty() )
		return;
	
	DefaultIOSystem FileSystem;
	const std::string mode = "w";
	m_pStream = FileSystem.Open( strFileName, mode );
}

// ---------------------------------------------------------------------------
//	Destructor
inline FileLogStream::~FileLogStream()
{
	if (NULL != m_pStream)
	{
		DefaultIOSystem FileSystem;
		FileSystem.Close( m_pStream );
	}
}

// ---------------------------------------------------------------------------
//	Write method
inline void FileLogStream::write( const std::string &message )
{
	if (m_pStream != NULL)
	{
		m_pStream->Write(message.c_str(), sizeof(char), 
			message.size());
		/*int i=0;
		i++;*/
	}
}

// ---------------------------------------------------------------------------

} // Namespace Assimp

#endif
