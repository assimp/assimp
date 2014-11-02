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
static const size_t      NumObjects     = 4;
static const std::string RootNodes[ NumObjects ] = {
    Metric,
    GeometryNode,
    GeometryObject,
    Material
};

static const size_t      NumSeparator = 4;
static const std::string Separator[ NumSeparator ] = {
    "(", ")", "{", "}"
};


static const size_t      NumToken = 8;
static const std::string Token[ NumToken ] = {
    RootNodes[ 0 ],
    RootNodes[ 1 ],
    RootNodes[ 2 ],
    RootNodes[ 3 ],
    Separator[ 0 ],
    Separator[ 1 ],
    Separator[ 2 ],
    Separator[ 3 ]
};

static bool isSeparator( char in ) {
    return ( in == '(' || in == ')' || in == '{' || in == '}' );
}

static bool containsNode( const char *bufferPtr, size_t size, const std::string *nodes, 
                          size_t numNodes, std::string &tokenFound ) {
    tokenFound = "";
    if( 0 == numNodes ) {
        return false;
    }

    bool found( false );
    for( size_t i = 0; i < numNodes; ++i ) {
        if( TokenMatch( bufferPtr, nodes[ i ].c_str(), nodes[ i ].size() ) ) {
            tokenFound = nodes[ i ];
            found = true;
            break;
        }
    }

    return found;
}

static OpenGEXParser::TokenType getTokenTypeByName( const char *in ) {
    ai_assert( NULL != in );

    OpenGEXParser::TokenType type( OpenGEXParser::None );
    for( size_t i = 0; i < NumToken; ++i ) {
        if( TokenMatch( in, Token[ i ].c_str(), Token[ i ].size() ) ) {
            type = static_cast<OpenGEXParser::TokenType>( i+1 );
            break;
        }
    }

    return type;
}

static void removeQuotes( std::string &attribName ) {
    std::string tmp;
    for( unsigned int i = 0; i < attribName.size(); ++i ) {
        if( attribName[ i ] != '\"' ) {
            tmp += attribName[ i ];
        }
    }
    attribName = tmp;
}

//------------------------------------------------------------------------------------------------
OpenGEXParser::OpenGEXParser( const std::vector<char> &buffer ) 
: m_buffer( buffer ) 
, m_index( 0 )
, m_buffersize( buffer.size() )
, m_nodeTypeStack() {
    // empty
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
        m_index++;
    }

    while( m_index < m_buffersize && !IsSpace( m_buffer[ m_index ] ) && !isSeparator( m_buffer[ m_index ] ) ) {
        token += m_buffer[ m_index ];
        m_index++;
    }

    if( token == "//" ) {
        skipComments();
        token = getNextToken();
    }

    if( token.empty() ) {
        if( isSeparator( m_buffer[ m_index ] ) ) {
            token += m_buffer[ m_index ];
            m_index++;
        }
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
    std::string rootNodeName, nodeType;
    if( containsNode( token.c_str(), token.size(), RootNodes, NumObjects, rootNodeName ) ) {
        m_nodeTypeStack.push_back( getTokenTypeByName( rootNodeName.c_str() ) );
        if( !getNodeHeader( nodeType ) ) {
            return false;
        }

        if( !getNodeData( nodeType ) ) {
            return false;
        }

        m_nodeTypeStack.pop_back();
    }

    return true;
}

//------------------------------------------------------------------------------------------------
bool OpenGEXParser::getNodeHeader( std::string &name ) {
    bool success( false );
    if( m_nodeTypeStack.back() == MetricNode ) {
        if( getMetricAttributeKey( name ) ) {
            success = true;
        }
    }

    return success;
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

    std::string tk;
    tk = getNextToken();

    if( !getBracketOpen() ) {
        return false;
    }

    bool ok( true );
    size_t dataIdx( 0 );
    for( unsigned int i = 0; i < num; ++i ) {
        data[ dataIdx ] = fast_atof( &m_buffer[ m_index ] );
        ++dataIdx;
        tk = getNextToken();
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
bool OpenGEXParser::getNodeData( const std::string &nodeType ) {
    bool success( false );

    if( !getBracketOpen() ) {
        return false;
    }

    TokenType type( m_nodeTypeStack.back() );
    if( type == MetricNode ) {
        success = onMetricNode( nodeType );
    }

    if( !getBracketClose() ) {
        return false;
    }

    return success;
}

//------------------------------------------------------------------------------------------------
bool OpenGEXParser::getMetricAttributeKey( std::string &attribName ) {
    bool ok( false );
    attribName = "";
    std::string token( getNextToken() );
    if( token[ 0 ] == '(' ) {
        // get attribute
        token = getNextToken();
        if( "key" == token ) {
            std::string equal = getNextToken();
            attribName = getNextToken();
            token = getNextToken();
            if( token[ 0 ] == ')' ) {
                ok = true;
                removeQuotes( attribName );
            }
        }
    }

    return ok;
}

//------------------------------------------------------------------------------------------------
bool OpenGEXParser::onMetricNode( const std::string &attribName ) {
    bool success( true );
    if( "distance" == attribName ) {
        float distance( 0.0f );
        if( getFloatData( 1, &distance ) ) {
            m_model.m_metrics.m_distance = distance;
        }
    } else if( "angle" == attribName ) {
        float angle( 0.0f );
        if( getFloatData( 1, &angle ) ) {
            m_model.m_metrics.m_angle = angle;
        }
    } else if( "time" == attribName ) {
        float time( 0.0f );
        if( getFloatData( 1, &time ) ) {
            m_model.m_metrics.m_time = time;
        }
    } else if( "up" == attribName ) {
        std::string up;
        if( getStringData( up ) ) {
            m_model.m_metrics.m_up = up;
        }
    } else {
        success = false;
    }

    return success;
}

//------------------------------------------------------------------------------------------------

} // Namespace OpenGEX
} // Namespace Assimp

#endif ASSIMP_BUILD_NO_OPEMGEX_IMPORTER
