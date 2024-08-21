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
#include "TestIOSystem.h"

#include <assimp/Base64.hpp>

using namespace std;
using namespace Assimp;

class Base64Test : public ::testing::Test {};

static const std::vector<uint8_t> assimpStringBinary = { 97, 115, 115, 105, 109, 112 };
static const std::string assimpStringEncoded = "YXNzaW1w";

TEST_F( Base64Test, encodeTest) {
    EXPECT_EQ( "", Base64::Encode(std::vector<uint8_t>{}) );
    EXPECT_EQ( "Vg==", Base64::Encode(std::vector<uint8_t>{ 86 }) );
    EXPECT_EQ( assimpStringEncoded, Base64::Encode(assimpStringBinary) );
}

TEST_F( Base64Test, encodeTestWithNullptr ) {
    std::string out;
    Base64::Encode(nullptr, 100u, out);
    EXPECT_TRUE(out.empty());

    Base64::Encode(&assimpStringBinary[0], 0u, out);
    EXPECT_TRUE(out.empty());
}

TEST_F( Base64Test, decodeTest) {
    EXPECT_EQ( std::vector<uint8_t> {}, Base64::Decode("") );
    EXPECT_EQ( std::vector<uint8_t> { 86 }, Base64::Decode("Vg==") );
    EXPECT_EQ( assimpStringBinary, Base64::Decode(assimpStringEncoded) );
}

TEST_F(Base64Test, decodeTestWithNullptr) {
    uint8_t *out = nullptr;
    size_t size = Base64::Decode(nullptr, 100u, out);
    EXPECT_EQ(nullptr, out);
    EXPECT_EQ(0u, size);
}
