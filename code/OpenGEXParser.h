/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2014, assimp team
All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the
following conditions are met:

* Redistributions of source code must retain the above
copyright notice, this list of conditions and the
following disclaimer.

* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the
following disclaimer in the documentation and/or other
materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
contributors may be used to endorse or promote products
derived from this software without specific prior
written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/
#ifndef ASSIMP_OPENGEX_OPENGEXPARSER_H_INC
#define ASSIMP_OPENGEX_OPENGEXPARSER_H_INC

#ifndef ASSIMP_BUILD_NO_OPEMGEX_IMPORTER
#include <vector>

namespace Assimp {
namespace OpenGEX {

class OpenGEXParser {
public:
    OpenGEXParser( const std::vector<char> &buffer );
    ~OpenGEXParser();
    void parse();

protected:
    std::string getNextToken();
    bool skipComments();
    bool parseNextNode();
    bool getNodeHeader( const std::string &name );
    bool getBracketOpen();
    bool getBracketClose();
    bool getStringData( std::string &data );
    bool getFloatData( size_t num, float *data );
    bool getNodeData();
    bool getMetricAttribute( std::string &attribName );
    bool onMetricNode();

private:
    OpenGEXParser( const OpenGEXParser & );
    OpenGEXParser &operator = ( const OpenGEXParser & );

private:
    const std::vector<char> &m_buffer;
    size_t m_index;
    size_t m_buffersize;
};

} // Namespace openGEX
} // Namespace Assimp

#endif // ASSIMP_BUILD_NO_OPEMGEX_IMPORTER

#endif // ASSIMP_OPENGEX_OPENGEXPARSER_H_INC
