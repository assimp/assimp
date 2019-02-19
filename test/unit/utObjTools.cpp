/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2019, assimp team



All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

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
---------------------------------------------------------------------------
*/
#include "UnitTestPCH.h"
#include "ObjTools.h"
#include "ObjFileParser.h"

using namespace ::Assimp;

class utObjTools : public ::testing::Test {
    // empty
};

class TestObjFileParser : public ObjFileParser {
public:
    TestObjFileParser() : ObjFileParser(){
        // empty
    }

    ~TestObjFileParser() {
        // empty
    }
    
    void testCopyNextWord( char *pBuffer, size_t length ) {
        copyNextWord( pBuffer, length );
    }

    size_t testGetNumComponentsInDataDefinition() {
        return getNumComponentsInDataDefinition();
    }
};

TEST_F( utObjTools, skipDataLine_OneLine_Success ) {
    std::vector<char> buffer;
    std::string data( "v -0.5 -0.5 0.5\nend" );
    buffer.resize( data.size() );
    ::memcpy( &buffer[ 0 ], &data[ 0 ], data.size() );
    std::vector<char>::iterator itBegin( buffer.begin() ), itEnd( buffer.end() );
    unsigned int line = 0;
    std::vector<char>::iterator current = skipLine<std::vector<char>::iterator>( itBegin, itEnd, line );
    EXPECT_EQ( 'e', *current );
}

TEST_F( utObjTools, skipDataLine_TwoLines_Success ) {
    TestObjFileParser test_parser;
    std::string data( "vn -2.061493116917992e-15 -0.9009688496589661 \\\n-0.4338837265968323" );
    std::vector<char> buffer;
    buffer.resize( data.size() );
    ::memcpy( &buffer[ 0 ], &data[ 0 ], data.size() );
    test_parser.setBuffer( buffer );
    static const size_t Size = 4096UL;
    char data_buffer[ Size ];
    
    test_parser.testCopyNextWord( data_buffer, Size );
    EXPECT_EQ( 0, strncmp( data_buffer, "vn", 2 ) );

    test_parser.testCopyNextWord( data_buffer, Size );
    EXPECT_EQ( data_buffer[0], '-' );

    test_parser.testCopyNextWord( data_buffer, Size );
    EXPECT_EQ( data_buffer[0], '-' );

    test_parser.testCopyNextWord( data_buffer, Size );
    EXPECT_EQ( data_buffer[ 0 ], '-' );
}

TEST_F( utObjTools, countComponents_TwoLines_Success ) {
    TestObjFileParser test_parser;
    std::string data( "-2.061493116917992e-15 -0.9009688496589661 \\\n-0.4338837265968323" );
    std::vector<char> buffer;
    buffer.resize( data.size() + 1 );
    ::memcpy( &buffer[ 0 ], &data[ 0 ], data.size() );
    buffer[ buffer.size() - 1 ] = '\0';
    test_parser.setBuffer( buffer );

    size_t numComps = test_parser.testGetNumComponentsInDataDefinition();
    EXPECT_EQ( 3U, numComps );
}
