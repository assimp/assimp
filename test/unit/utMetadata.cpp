/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2017, assimp team


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

#include <assimp/metadata.h>

using namespace ::Assimp;

class utMetadata: public ::testing::Test {
protected:
    aiMetadata *m_data;

    virtual void SetUp() {
        m_data = nullptr;
    }

    virtual void TearDown() {
        aiMetadata::Dealloc( m_data );
    }

};

TEST_F( utMetadata, creationTest ) {
    bool ok( true );
    try {
        aiMetadata data;
    } catch ( ... ) {
        ok = false;
    }
    EXPECT_TRUE( ok );
}

TEST_F( utMetadata, allocTest ) {
    aiMetadata *data = aiMetadata::Alloc( 0 );
    EXPECT_EQ( nullptr, data );

    data = aiMetadata::Alloc( 1 );
    EXPECT_NE( nullptr, data );
    EXPECT_EQ( 1U, data->mNumProperties );
    EXPECT_NE( nullptr, data->mKeys );
    EXPECT_NE( nullptr, data->mValues );
}

TEST_F( utMetadata, get_set_pod_Test ) {
    m_data = aiMetadata::Alloc( 5 );

    // int, 32 bit
    unsigned int index( 0 );
    bool success( false );
    const std::string key_int = "test_int";
    success = m_data->Set( index, key_int, 1 );
    EXPECT_TRUE( success );
    success = m_data->Set( index + 10, key_int, 1 );
    EXPECT_FALSE( success );

    // unsigned int, 64 bit
    index++;
    const std::string key_uint = "test_uint";
    success = m_data->Set<uint64_t>( index, key_uint, 1UL );
    EXPECT_TRUE( success );
    uint64_t result_uint( 0 );
    success = m_data->Get( key_uint, result_uint );
    EXPECT_TRUE( success );
    EXPECT_EQ( 1UL, result_uint );

    // bool
    index++;
    const std::string key_bool = "test_bool";
    success = m_data->Set( index, key_bool, true );
    EXPECT_TRUE( success );
    bool result_bool( false );
    success = m_data->Get( key_bool, result_bool );
    EXPECT_TRUE( success );
    EXPECT_EQ( true, result_bool );

    // float
    index++;
    const std::string key_float = "test_float";
    float fVal = 2.0f;
    success = m_data->Set( index, key_float, fVal );
    EXPECT_TRUE( success );
    float result_float( 0.0f );
    success = m_data->Get( key_float, result_float );
    EXPECT_TRUE( success );
    EXPECT_FLOAT_EQ( 2.0f, result_float );

    // double
    index++;
    const std::string key_double = "test_double";
    double dVal = 3.0;
    success = m_data->Set( index, key_double, dVal );
    EXPECT_TRUE( success );
    double result_double( 0.0 );
    success = m_data->Get( key_double, result_double );
    EXPECT_TRUE( success );
    EXPECT_DOUBLE_EQ( 3.0, result_double );

    // error
    int result;
    success = m_data->Get( "bla", result );
    EXPECT_FALSE( success );
}

TEST_F( utMetadata, get_set_string_Test ) {
    m_data = aiMetadata::Alloc( 1 );

    unsigned int index( 0 );
    bool success( false );
    const std::string key = "test";
    success = m_data->Set( index, key, aiString( std::string( "test" ) ) );
    EXPECT_TRUE( success );

    success = m_data->Set( index+10, key, aiString( std::string( "test" ) ) );
    EXPECT_FALSE( success );

    aiString result;
    success = m_data->Get( key, result );
    EXPECT_EQ( aiString( std::string( "test" ) ), result );
    EXPECT_TRUE( success );

    success = m_data->Get( "bla", result );
    EXPECT_FALSE( success );
}

TEST_F( utMetadata, get_set_aiVector3D_Test ) {
    m_data = aiMetadata::Alloc( 1 );

    unsigned int index( 0 );
    bool success( false );
    const std::string key = "test";
    aiVector3D vec( 1, 2, 3 );

    success = m_data->Set( index, key, vec );
    EXPECT_TRUE( success );

    aiVector3D result( 0, 0, 0 );
    success = m_data->Get( key, result );
    EXPECT_EQ( vec, result );
    EXPECT_TRUE( success );
}

