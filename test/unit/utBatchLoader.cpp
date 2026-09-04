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
#include "UnitTestPCH.h"
#include "Common/Importer.h"
#include "TestIOSystem.h"
#include <assimp/DefaultIOSystem.h>
#include <assimp/MemoryIOWrapper.h>
#include <assimp/scene.h>

using namespace ::Assimp;

class BatchLoaderTest : public ::testing::Test {
public:
    virtual void SetUp() {
        m_io = new TestIOSystem();
    }

    virtual void TearDown() {
        delete m_io;
    }

protected:
    TestIOSystem* m_io;
};

TEST_F( BatchLoaderTest, createTest ) {
    bool ok( true );
    try {
        BatchLoader loader( m_io );
    } catch ( ... ) {
        ok = false;
    }
    EXPECT_TRUE( ok );
}

TEST_F( BatchLoaderTest, validateAccessTest ) {
    BatchLoader loader1( m_io );
    EXPECT_FALSE( loader1.getValidation() );
    loader1.setValidation( true );
    EXPECT_TRUE( loader1.getValidation() );

    BatchLoader loader2( m_io, true );
    EXPECT_TRUE( loader2.getValidation() );
}

TEST_F(BatchLoaderTest, polledSceneOwnershipIsTransferred) {
    static const char obj[] =
            "o triangle\n"
            "v 0 0 0\n"
            "v 1 0 0\n"
            "v 0 1 0\n"
            "f 1 2 3\n";

    DefaultIOSystem fileSystem;
    MemoryIOSystem memoryIO(reinterpret_cast<const uint8_t *>(obj), sizeof(obj) - 1, &fileSystem);

    BatchLoader loader(&memoryIO);
    const std::string path = AI_MEMORYIO_MAGIC_FILENAME ".obj";
    const unsigned int request = loader.AddLoadRequest(path);
    EXPECT_EQ(request, loader.AddLoadRequest(path));

    loader.LoadAll();
    aiScene *scene = loader.GetImport(request);
    ASSERT_NE(nullptr, scene);

    // One duplicate request remains, but ownership transferred on the first poll.
    delete scene;
}
