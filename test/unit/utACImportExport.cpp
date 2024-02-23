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

#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>


using namespace Assimp;

TEST(utACImportExport, importClosedLine) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AC/closedLine.ac", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utACImportExport, importNoSurfaces) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AC/nosurfaces.ac", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utACImportExport, importOpenLine) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AC/openLine.ac", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utACImportExport, importSampleSubdiv) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AC/sample_subdiv.ac", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);

    // check approximate shape by averaging together all vertices
    ASSERT_EQ(scene->mNumMeshes, 1u);
    aiVector3D vertexAvg(0.0, 0.0, 0.0);
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        const aiMesh *mesh = scene->mMeshes[i];
        ASSERT_NE(mesh, nullptr);

        ai_real invVertexCount = 1.0 / mesh->mNumVertices;
        for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
            vertexAvg += mesh->mVertices[j] * invVertexCount;
        }
    }

    // must not be inf or nan
    ASSERT_TRUE(std::isfinite(vertexAvg.x));
    ASSERT_TRUE(std::isfinite(vertexAvg.y));
    ASSERT_TRUE(std::isfinite(vertexAvg.z));
    EXPECT_NEAR(vertexAvg.x, 0.079997420310974121, 0.0001);
    EXPECT_NEAR(vertexAvg.y, 0.099498569965362549, 0.0001);
    EXPECT_NEAR(vertexAvg.z, -0.10344827175140381, 0.0001);
}

TEST(utACImportExport, importSphereWithLight) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AC/SphereWithLight.ac", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utACImportExport, importSphereWithLightACC) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AC/SphereWithLight.acc", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utACImportExport, importSphereWithLightUTF16) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AC/SphereWithLight_UTF16LE.ac", aiProcess_ValidateDataStructure);
    // FIXME: this is probably wrong, loading the file should succeed
    ASSERT_EQ(nullptr, scene);
}

TEST(utACImportExport, importSphereWithLightUTF8BOM) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AC/SphereWithLight_UTF8BOM.ac", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utACImportExport, importSphereWithLightUvScaling4X) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AC/SphereWithLightUvScaling4X.ac", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utACImportExport, importWuson) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AC/Wuson.ac", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utACImportExport, importWusonACC) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AC/Wuson.acc", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utACImportExport, testFormatDetection) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AC/TestFormatDetection", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}
