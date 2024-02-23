/*-------------------------------------------------------------------------
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
-------------------------------------------------------------------------*/
#include "UnitTestPCH.h"
#include <assimp/XmlParser.h>
#include <assimp/DefaultIOStream.h>
#include <assimp/DefaultIOSystem.h>

using namespace Assimp;

class utXmlParser : public ::testing::Test {
public:
    utXmlParser() :
            Test(),
            mIoSystem() {
        // empty
    }

protected:
    DefaultIOSystem mIoSystem;
};

TEST_F(utXmlParser, parse_xml_test) {
    XmlParser parser;
    std::string filename = ASSIMP_TEST_MODELS_DIR "/X3D/ComputerKeyboard.x3d";
    std::unique_ptr<IOStream> stream(mIoSystem.Open(filename.c_str(), "rb"));
    EXPECT_NE(stream.get(), nullptr);
    bool result = parser.parse(stream.get());
    EXPECT_TRUE(result);
}

TEST_F(utXmlParser, parse_xml_and_traverse_test) {
    XmlParser parser;
    std::string filename = ASSIMP_TEST_MODELS_DIR "/X3D/ComputerKeyboard.x3d";
    std::unique_ptr<IOStream> stream(mIoSystem.Open(filename.c_str(), "rb"));
    EXPECT_NE(stream.get(), nullptr);
    bool result = parser.parse(stream.get());
    EXPECT_TRUE(result);
    XmlNode root = parser.getRootNode();

    XmlNodeIterator nodeIt(root, XmlNodeIterator::PreOrderMode);
    const size_t numNodes = nodeIt.size();
    bool empty = nodeIt.isEmpty();
    EXPECT_FALSE(empty);
    EXPECT_NE(numNodes, 0U);
    XmlNode node;
    while (nodeIt.getNext(node)) {
        const std::string nodeName = node.name();
        EXPECT_FALSE(nodeName.empty());
    }
}
