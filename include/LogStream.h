#ifndef AI_LOGSTREAM_H_INC
#define AI_LOGSTREAM_H_INC

#include <string>

namespace Assimp
{
// ---------------------------------------------------------------------------
/**	@class	LogStream
 *	@brief	Abstract interface for log stream implementations.
 */
class ASSIMP_API LogStream
{
protected:
	/**	@brief	Default constructor	*/
	LogStream();

public:
	/**	@brief	Virtual destructor	*/
	virtual ~LogStream();

	/**	@brief	Overwrite this for your own output methods	*/
	virtual void write(const std::string &message) = 0;
};

// ---------------------------------------------------------------------------
//	Default constructor
inline LogStream::LogStream()
{
	// empty
}

// ---------------------------------------------------------------------------
//	Virtual destructor
inline LogStream::~LogStream()
{
	// empty
}

// ---------------------------------------------------------------------------

} // Namespace Assimp

#endif
