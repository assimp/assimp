/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

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

    void SetUp() override {
        m_data = nullptr;
    }

    void TearDown() override {
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
    aiMetadata::Dealloc( data );
}

TEST_F( utMetadata, get_set_pod_Test ) {
    m_data = aiMetadata::Alloc( 7 );

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

    // int64_t
    index++;
    const std::string key_int64 = "test_int64";
    int64_t val_int64 = 64;
    success = m_data->Set(index, key_int64, val_int64);
    EXPECT_TRUE(success);
    int64_t result_int64(0);
    success = m_data->Get(key_int64, result_int64);
    EXPECT_TRUE(success);
    EXPECT_EQ(result_int64, val_int64);

    // uint32
    index++;
    const std::string key_uint32 = "test_uint32";
    int64_t val_uint32 = 32;
    success = m_data->Set(index, key_uint32, val_uint32);
    EXPECT_TRUE(success);
    int64_t result_uint32(0);
    success = m_data->Get(key_uint32, result_uint32);
    EXPECT_TRUE(success);
    EXPECT_EQ(result_uint32, val_uint32);

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


TEST_F( utMetadata, copy_test ) {
    m_data = aiMetadata::Alloc( AI_META_MAX );
    bool bv = true;
    m_data->Set( 0, "bool", bv );
    int32_t i32v = -10;
    m_data->Set( 1, "int32", i32v );
    uint64_t ui64v = static_cast<uint64_t>( 10 );
    m_data->Set( 2, "uint64", ui64v );
    float fv = 1.0f;
    m_data->Set( 3, "float", fv );
    double dv = 2.0;
    m_data->Set( 4, "double", dv );
    const aiString strVal( std::string( "test" ) );
    m_data->Set( 5, "aiString", strVal );
    aiVector3D vecVal( 1, 2, 3 );
    m_data->Set( 6, "aiVector3D", vecVal );
    aiMetadata metaVal;
    m_data->Set( 7, "aiMetadata", metaVal );
    int64_t i64 = 64;
    m_data->Set(8, "int64_t", i64);
    uint32_t ui32 = 32;
    m_data->Set(9, "uint32_t", ui32);
    aiMetadata copy(*m_data);
    EXPECT_EQ( 10u, copy.mNumProperties );

    // bool test
    {
        bool v = true;
        EXPECT_TRUE( copy.Get( "bool", v ) );
        EXPECT_EQ( bv, v );
    }

    // int32_t test
    {
        int32_t v = 127;
        bool ok = copy.Get( "int32", v );
        EXPECT_TRUE( ok );
        EXPECT_EQ( i32v, v );
    }

    // uint32_t test
    {
        uint32_t v = 0;
        bool ok = copy.Get("uint32_t", v);
        EXPECT_TRUE(ok);
        EXPECT_EQ( ui32, v );
    }

    // int64_t test
    {
        int64_t v = -1;
        bool ok = copy.Get("int64_t", v);
        EXPECT_TRUE(ok);
        EXPECT_EQ( i64, v );
    }

    // uint64_t test
    {
        uint64_t v = 255;
        bool ok = copy.Get( "uint64", v );
        EXPECT_TRUE( ok );
        EXPECT_EQ( ui64v, v );
    }

    // float test
    {
        float v = -9.9999f;
        EXPECT_TRUE( copy.Get( "float", v ) );
        EXPECT_EQ( fv, v );
    }

    // double test
    {
        double v = -99.99;
        EXPECT_TRUE( copy.Get( "double", v ) );
        EXPECT_EQ( dv, v );
    }

    // string test
    {
        aiString v;
        EXPECT_TRUE( copy.Get( "aiString", v ) );
        EXPECT_EQ( strVal, v );
    }

    // vector test
    {
        aiVector3D v;
        EXPECT_TRUE( copy.Get( "aiVector3D", v ) );
        EXPECT_EQ( vecVal, v );
    }

    // metadata test
    {
        aiMetadata v;
        EXPECT_TRUE( copy.Get( "aiMetadata", v ) );
        EXPECT_EQ( metaVal, v );
    }
}

TEST_F( utMetadata, set_test ) {
    aiMetadata v;
    const std::string key_bool = "test_bool";
    v.Set(1, key_bool, true);
    v.Set(1, key_bool, true);
    v.Set(1, key_bool, true);
    v.Set(1, key_bool, true);
}
