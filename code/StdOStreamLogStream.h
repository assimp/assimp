#ifndef AI_STROSTREAMLOGSTREAM_H_INC
#define AI_STROSTREAMLOGSTREAM_H_INC

#include "../include/assimp/LogStream.hpp"
#include <ostream>

namespace Assimp    {

// ---------------------------------------------------------------------------
/** @class  StdOStreamLogStream
 *  @brief  Logs into a std::ostream
 */
class StdOStreamLogStream : public LogStream
{
public:
    /** @brief  Construction from an existing std::ostream
     *  @param _ostream Output stream to be used
    */
    explicit StdOStreamLogStream(std::ostream& _ostream);

    /** @brief  Destructor  */
    ~StdOStreamLogStream();

    /** @brief  Writer  */
    void write(const char* message);
private:
    std::ostream& ostream;
};

// ---------------------------------------------------------------------------
//  Default constructor
inline StdOStreamLogStream::StdOStreamLogStream(std::ostream& _ostream)
    : ostream   (_ostream)
{}

// ---------------------------------------------------------------------------
//  Default constructor
inline StdOStreamLogStream::~StdOStreamLogStream()
{}

// ---------------------------------------------------------------------------
//  Write method
inline void StdOStreamLogStream::write(const char* message)
{
    ostream << message;
    ostream.flush();
}

// ---------------------------------------------------------------------------
}   // Namespace Assimp

#endif // guard
