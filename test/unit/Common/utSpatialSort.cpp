/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team

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

#include <assimp/SpatialSort.h>

using namespace Assimp;

class utSpatialSort : public ::testing::Test {
public
        :
    aiVector3D *vecs;

protected:
    void SetUp() override {
        ::srand(static_cast<unsigned>(time(0)));
        vecs = new aiVector3D[100];
        for (size_t i = 0; i < 100; ++i) {
            vecs[i].x = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 100));
            vecs[i].y = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 100));
            vecs[i].z = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 100));
        }
    }

    void TearDown() override {
        delete[] vecs;
    }
};

TEST_F( utSpatialSort, findIdenticalsTest ) {
    SpatialSort sSort;
    sSort.Fill(vecs, 100, sizeof(aiVector3D));

    std::vector<unsigned int> indices;
    sSort.FindIdenticalPositions(vecs[0], indices);
    EXPECT_EQ(1u, indices.size());
}

TEST_F(utSpatialSort, findPositionsTest) {
    SpatialSort sSort;
    sSort.Fill(vecs, 100, sizeof(aiVector3D));

    std::vector<unsigned int> indices;
    sSort.FindPositions(vecs[0], 0.01f, indices);
    EXPECT_EQ(1u, indices.size());
}

TEST_F(utSpatialSort, highlyDisplacedPositionsTest) {
    // Make a cube of positions, and then query it using the SpatialSort object.
    constexpr unsigned int verticesPerAxis = 10;
    constexpr ai_real step = 0.001f;
    // Note the large constant offset here.
    constexpr ai_real offset = 5000.0f - (0.5f * verticesPerAxis * step);
    constexpr unsigned int totalNumPositions = verticesPerAxis * verticesPerAxis * verticesPerAxis;
    aiVector3D* positions = new aiVector3D[totalNumPositions];
    for (unsigned int x = 0; x < verticesPerAxis; ++x) {
        for (unsigned int y = 0; y < verticesPerAxis; ++y) {
            for (unsigned int z = 0; z < verticesPerAxis; ++z) {
                const unsigned int index = (x * verticesPerAxis * verticesPerAxis) + (y * verticesPerAxis) + z;
                positions[index] = aiVector3D(offset + (x * step), offset + (y * step), offset + (z * step));
            }
        }
    }

    SpatialSort sSort;
    sSort.Fill(positions, totalNumPositions, sizeof(aiVector3D));

    // Enough to find a point and its 6 immediate neighbors, but not any other point.
    const ai_real epsilon = 1.1f * step;
    std::vector<unsigned int> indices;
    // Iterate through the _interior_ points of the cube.
    for (unsigned int x = 1; x < verticesPerAxis - 1; ++x) {
        for (unsigned int y = 1; y < verticesPerAxis - 1; ++y) {
            for (unsigned int z = 1; z < verticesPerAxis - 1; ++z) {
                const unsigned int index = (x * verticesPerAxis * verticesPerAxis) + (y * verticesPerAxis) + z;
                sSort.FindPositions(positions[index], epsilon, indices);
                ASSERT_EQ(7u, indices.size());
            }
        }
    }
    delete[] positions;
}
