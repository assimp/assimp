/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2016, assimp team

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
#include "IOStreamBuffer.h"
#include "TestIOStream.h"

class IOStreamBufferTest : public ::testing::Test {
    // empty
};

using namespace Assimp;

TEST_F( IOStreamBufferTest, creationTest ) {
    bool ok( true );
    try {
        IOStreamBuffer<char> myBuffer;
    } catch ( ... ) {
        ok = false;
    }
    EXPECT_TRUE( ok );
}

TEST_F( IOStreamBufferTest, accessCacheSizeTest ) {
    IOStreamBuffer<char> myBuffer1;
    EXPECT_NE( 0U, myBuffer1.cacheSize() );

    IOStreamBuffer<char> myBuffer2( 100 );
    EXPECT_EQ( 100U, myBuffer2.cacheSize() );
}

TEST_F( IOStreamBufferTest, open_close_Test ) {
    IOStreamBuffer<char> myBuffer;

    EXPECT_FALSE( myBuffer.open( nullptr ) );
    EXPECT_FALSE( myBuffer.close() );

    char buffer[ L_tmpnam ];
    tmpnam( buffer );
    std::FILE *fs( std::fopen( buffer, "w+" ) );
    size_t written( std::fwrite( buffer, 1, sizeof( char ) * L_tmpnam, fs ) );
    EXPECT_NE( 0U, written );
    std::fflush( fs );

    TestDefaultIOStream myStream( fs, buffer );

    EXPECT_TRUE( myBuffer.open( &myStream ) );
    EXPECT_FALSE( myBuffer.open( &myStream ) );
    EXPECT_TRUE( myBuffer.close() );
}

TEST_F( IOStreamBufferTest, readlineTest ) {
    char buffer[ L_tmpnam ];
    tmpnam( buffer );
    std::FILE *fs( std::fopen( buffer, "w+" ) );
    size_t written( std::fwrite( buffer, 1, sizeof( char ) * L_tmpnam, fs ) );
    EXPECT_NE( 0U, written );
    std::fflush( fs );

    IOStreamBuffer<char> myBuffer( 26 );
    EXPECT_EQ( 26U, myBuffer.cacheSize() );

    TestDefaultIOStream myStream( fs, buffer );
    size_t size( myStream.FileSize() );
    size_t numBlocks( size / myBuffer.cacheSize() );
    if ( size % myBuffer.cacheSize() > 0 ) {
        numBlocks++;
    }
    EXPECT_TRUE( myBuffer.open( &myStream ) );
    EXPECT_EQ( numBlocks, myBuffer.getNumBlocks() );
    EXPECT_TRUE( myBuffer.close() );
}

TEST_F( IOStreamBufferTest, accessBlockIndexTest ) {

}

