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
#include "AbstractImportExportBase.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/SpatialSort.h>

using namespace Assimp;

class utBlenderImporterExporter : public AbstractImportExportBase {
public:
    virtual bool importerTest() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile( ASSIMP_TEST_MODELS_DIR "/BLEND/box.blend", aiProcess_ValidateDataStructure );
        return nullptr != scene;
    }
};

TEST_F( utBlenderImporterExporter, importBlenFromFileTest ) {
    EXPECT_TRUE( importerTest() );
}


/*
This test contains a default cube with subdivision surface modifier
and a default cube with subdivision surface applied.
Vertices should be identical.
*/
TEST_F(utBlenderImporterExporter, importBlendWithSubdivisionSurface) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/subdivision_test_277.blend", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mNumMeshes, 2);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, scene->mMeshes[1]->mNumVertices);

    SpatialSort spatialSortVertices0;
    spatialSortVertices0.Fill(scene->mMeshes[0]->mVertices, scene->mMeshes[0]->mNumVertices, sizeof(aiVector3D), true);

    for (unsigned int i = 0; i < scene->mMeshes[1]->mNumVertices; ++i)
    {
        auto positionMesh1 = scene->mMeshes[1]->mVertices[i];
        std::vector<unsigned int> spatialSortResult;
        spatialSortVertices0.FindPositions(positionMesh1, 1.0e-6f, spatialSortResult);

        EXPECT_TRUE(scene->mMeshes[0]->mVertices[spatialSortResult[0]].Equal(positionMesh1));
    }
}
