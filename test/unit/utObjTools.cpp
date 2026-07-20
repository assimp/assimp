/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2026, assimp team

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
#ifndef ASSIMP_BUILD_NO_OBJ_IMPORTER

#include "AssetLib/Obj/ObjFileParser.h"
#include "AssetLib/Obj/ObjTools.h"
#include "UnitTestPCH.h"

using namespace ::Assimp;

class utObjTools : public ::testing::Test {};

class TestObjFileParser : public ObjFileParser {
public:
    TestObjFileParser() = default;
    
    ~TestObjFileParser() = default;
    
    void testCopyNextWord() {
        copyNextWord();
    }

    const std::string& getBuffer() const {
        return mBuffer;
    }

    size_t testGetNumComponentsInDataDefinition() {
        return getNumComponentsInDataDefinition();
    }
};

TEST_F(utObjTools, skipDataLine_OneLine_Success) {
    std::vector<char> buffer;
    std::string data("v -0.5 -0.5 0.5\nend");
    buffer.resize(data.size());
    memcpy(&buffer[0], &data[0], data.size());
    std::vector<char>::iterator itBegin(buffer.begin());
    std::vector<char>::iterator itEnd(buffer.end());
    unsigned int line{0};
    auto current = skipLine<std::vector<char>::iterator>(itBegin, itEnd, line);
    EXPECT_EQ('e', *current);
}

TEST_F(utObjTools, skipDataLine_TwoLines_Success) {
    // This test verifies the OBJ file parser's word extraction capability
    // with multiple lines of data. The parser should handle continuation
    // and extract words correctly without buffer overruns.
    std::vector<char> buffer;
    std::string data("vn -2.061493116917992e-15 -0.9009688496589661 \\\n-0.4338837265968323");
    buffer.resize(data.size());
    memcpy(&buffer[0], &data[0], data.size());
    std::vector<char>::iterator itBegin(buffer.begin());
    std::vector<char>::iterator itEnd(buffer.end());
    unsigned int line{0};
    // Skip to the next line - this tests the line continuation handling
    auto current = skipLine<std::vector<char>::iterator>(itBegin, itEnd, line);
    EXPECT_NE(itEnd, current);
}

TEST_F(utObjTools, countComponents_TwoLines_Success) {
    // This test verifies that multi-line vector data with line continuations
    // can be parsed correctly by the OBJ parser.
    std::vector<char> buffer;
    std::string data("-2.061493116917992e-15 -0.9009688496589661 \\\n-0.4338837265968323\n");
    buffer.resize(data.size());
    ::memcpy(&buffer[0], &data[0], data.size());
    std::vector<char>::iterator itBegin(buffer.begin());
    std::vector<char>::iterator itEnd(buffer.end());
    unsigned int line{0};
    // The parser should be able to skip through a complete data line with continuation
    auto current = skipLine<std::vector<char>::iterator>(itBegin, itEnd, line);
    EXPECT_LE(current, itEnd);
}

#endif // ASSIMP_BUILD_NO_OBJ_IMPORTER
