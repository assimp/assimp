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
#include "AssimpPCH.h"
#ifndef ASSIMP_BUILD_NO_OPEMGEX_IMPORTER

#include "OpenGEXParser.h"
#include "OpenGEXStructs.h"
#include "ParsingUtils.h"
#include "fast_atof.h"

#include <vector>

namespace Assimp {
namespace OpenGEX {

//------------------------------------------------------------------------------------------------
static const std::string Metric         = "Metric";
static const std::string GeometryNode   = "GeometryNode";
static const std::string GeometryObject = "GeometryObject";
static const std::string Material       = "Material";
static const size_t NumObjects = 4;

static const std::string RootNodes[ NumObjects ] = {
    Metric,
    GeometryNode,
    GeometryObject,
    Material
};

static bool containsNode( const char *bufferPtr, size_t size, const std::string *nodes, size_t numNodes,
                          const std::string *tokenFound ) {
    tokenFound = NULL;
    if( 0 == numNodes ) {
        return false;
    }

    bool found( false );
    for( size_t i = 0; i < numNodes; ++i ) {
        if( TokenMatch( bufferPtr, nodes[ i ].c_str(), nodes[ i ].size() ) ) {
            tokenFound = &nodes[ i ];
            found = true;
            break;
        }
    }

    return found;
}

//------------------------------------------------------------------------------------------------
OpenGEXParser::OpenGEXParser( const std::vector<char> &buffer ) 
: m_buffer( buffer ) 
, m_index( 0 )
, m_buffersize( buffer.size() ) {

}

//------------------------------------------------------------------------------------------------
OpenGEXParser::~OpenGEXParser() {

}

//------------------------------------------------------------------------------------------------
void OpenGEXParser::parse() {
    while( parseNextNode() ) {

    }
}

//------------------------------------------------------------------------------------------------
std::string OpenGEXParser::getNextToken() {
    std::string token;
    while( m_index < m_buffersize && IsSpace( m_buffer[ m_index ] ) ) {
        ++m_index;
    }

    while( m_index < m_buffersize && !IsSpace( m_buffer[ m_index ] ) ) {
        token += m_buffer[ m_index ];
        m_index++;
    }

    if( token == "//" ) {
        skipComments();
        token = getNextToken();
    }

    return token;
}

//------------------------------------------------------------------------------------------------
bool OpenGEXParser::skipComments() {
    bool skipped( false );
    if( strncmp( &m_buffer[ m_index ], "//", 2 ) == 0) {
        while( !IsLineEnd( m_buffer[ m_index ] ) ) {
            ++m_index;
        }
        skipped = true;
    }

    return skipped;
}

//------------------------------------------------------------------------------------------------
bool OpenGEXParser::parseNextNode() {
    std::string token( getNextToken() );
    std::string rootNodeName;
    if( containsNode( token.c_str(), token.size(), RootNodes, NumObjects, &rootNodeName ) ) {
        if( !getNodeHeader( rootNodeName ) ) {
            return false;
        }

        if( !getNodeData() ) {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------------------------
bool OpenGEXParser::getNodeHeader( const std::string &name ) {
    if( name == Metric ) {
        std::string token( getNextToken() );

    }

    return false;
}

//------------------------------------------------------------------------------------------------
bool OpenGEXParser::getBracketOpen() {
    const std::string token( getNextToken() );
    if( "{" == token ) {
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
bool OpenGEXParser::getBracketClose() {
    const std::string token( getNextToken() );
    if( "}" == token ) {
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
bool OpenGEXParser::getStringData( std::string &data ) {
    if( !getBracketOpen() ) {
        return false;
    }

    if( !getBracketClose() ) {
        return false;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
bool OpenGEXParser::getFloatData( size_t num, float *data ) {
    ai_assert( NULL != data );

    if( !getBracketOpen() ) {
        return false;
    }

    bool ok( true );
    size_t dataIdx( 0 );
    for( unsigned int i = 0; i < num; ++i ) {
        data[ dataIdx ] = fast_atof( &m_buffer[ m_index ] );
        ++dataIdx;
        std::string tk = getNextToken();
        if( tk == "," ) {
            if( i >= ( num - 1 ) ) {
                ok = false;
                break;
            }
        }
    }

    if( !getBracketClose() ) {
        return false;
    }

    return ok;
}

//------------------------------------------------------------------------------------------------
bool OpenGEXParser::getNodeData() {
    if( !getBracketOpen() ) {
        return false;
    }

    if( !onMetricNode() ) {
        return false;
    }

    if( !getBracketClose() ) {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
bool OpenGEXParser::getMetricAttribute( std::string &attribName ) {
    return false;
}

//------------------------------------------------------------------------------------------------
bool OpenGEXParser::onMetricNode() {
    std::string attribName;
    if( !getMetricAttribute( attribName ) ) {
        return false;
    }

    if( "distance" == attribName ) {
        float distance( 0.0f );
        getFloatData( 1, &distance );
    } else if( "angle" == attribName ) {
        float angle( 0.0f );
        getFloatData( 1, &angle );
    } else if( "time" == attribName ) {
        float time( 0.0f );
        getFloatData( 1, &time );
    } else if( "up" == attribName ) {
        std::string up;
        getStringData( up );
    } else {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------

} // Namespace openGEX
} // Namespace Assimp

#endif ASSIMP_BUILD_NO_OPEMGEX_IMPORTER
