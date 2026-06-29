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

#include "AbstractImportExportBase.h"
#include "UnitTestPCH.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>

#include <vector>

using namespace Assimp;

class utSTLImporterExporter : public AbstractImportExportBase {
public:
    // Import an STL model with structure validation and return its scene.
    // The supplied importer owns the returned scene and must outlive it.
    const aiScene *importValidatedSTL(Assimp::Importer &importer, const char *file) {
        return importer.ReadFile(file, aiProcess_ValidateDataStructure);
    }
    virtual bool importerTest() {
        Assimp::Importer importer;
        return nullptr != importValidatedSTL(importer, ASSIMP_TEST_MODELS_DIR "/STL/Spider_ascii.stl");
    }
};

TEST_F(utSTLImporterExporter, importSTLFromFileTest) {
    EXPECT_TRUE(importerTest());
}

TEST_F(utSTLImporterExporter, importBinarySTLFromFileTest) {
    // Regression test for issue #5509: binary STL files were not recognized on
    // big-endian hosts (e.g. s390x) because the little-endian on-disk facet
    // count and geometry were read without byte-swapping. Asserting concrete
    // counts and coordinates ensures both the facet count and the per-float
    // geometry are decoded correctly rather than read as byte-swapped values.
    Assimp::Importer importer;
    const aiScene *scene = importValidatedSTL(importer, ASSIMP_TEST_MODELS_DIR "/STL/Spider_binary.stl");
    ASSERT_NE(nullptr, scene);
    ASSERT_EQ(1u, scene->mNumMeshes);
    const aiMesh *mesh = scene->mMeshes[0];
    EXPECT_EQ(1368u, mesh->mNumFaces);
    EXPECT_EQ(4104u, mesh->mNumVertices);
    // First facet's first vertex and normal (stored as little-endian floats).
    EXPECT_NEAR(0.90712798f, mesh->mVertices[0].x, 1e-4f);
    EXPECT_NEAR(0.64616501f, mesh->mVertices[0].y, 1e-4f);
    EXPECT_NEAR(0.79519337f, mesh->mVertices[0].z, 1e-4f);
    EXPECT_NEAR(0.46828195f, mesh->mNormals[0].x, 1e-4f);
    EXPECT_NEAR(-0.86349779f, mesh->mNormals[0].y, 1e-4f);
    EXPECT_NEAR(-0.18730624f, mesh->mNormals[0].z, 1e-4f);
}

TEST_F(utSTLImporterExporter, test_multiple) {
    // import same file twice, each with its own importer
    // must work both times and not crash
    Assimp::Importer importer1;
    const aiScene *scene1 = importer1.ReadFile(ASSIMP_TEST_MODELS_DIR "/STL/Spider_ascii.stl", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene1);

    Assimp::Importer importer2;
    const aiScene *scene2 = importer2.ReadFile(ASSIMP_TEST_MODELS_DIR "/STL/Spider_ascii.stl", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene2);
}

TEST_F(utSTLImporterExporter, importSTLformatdetection) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/STL/formatDetection", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}

TEST_F(utSTLImporterExporter, test_with_two_solids) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/STL/triangle_with_two_solids.stl", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
}

TEST_F(utSTLImporterExporter, test_with_empty_solid) {
    Assimp::Importer importer;
    //STL File with empty mesh. We should still be able to import other meshes in this file. ValidateDataStructure should fail.
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/STL/triangle_with_empty_solid.stl", 0);
    EXPECT_NE(nullptr, scene);

    const aiScene *scene2 = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/STL/triangle_with_empty_solid.stl", aiProcess_ValidateDataStructure);
    EXPECT_EQ(nullptr, scene2);
}

#ifndef ASSIMP_BUILD_NO_EXPORT

TEST_F(utSTLImporterExporter, exporterTest) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/STL/Spider_ascii.stl", aiProcess_ValidateDataStructure);

    Assimp::Exporter mAiExporter;
    const char *stlFileName = "spiderExport.stl";
    mAiExporter.Export(scene, "stl", stlFileName);

    const aiScene *scene2 = importer.ReadFile(stlFileName, aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene2);

    // Cleanup, delete the exported file
    std::remove(stlFileName);
}

TEST_F(utSTLImporterExporter, test_export_pointclouds) {
    struct XYZ {
        float x, y, z;
    };

    std::vector<XYZ> points;

    for (size_t i = 0; i < 10; ++i) {
        XYZ current;
        current.x = static_cast<float>(i);
        current.y = static_cast<float>(i);
        current.z = static_cast<float>(i);
        points.push_back(current);
    }
    aiScene scene;
    scene.mRootNode = new aiNode();

    scene.mMeshes = new aiMesh *[1];
    scene.mMeshes[0] = nullptr;
    scene.mNumMeshes = 1;

    scene.mMaterials = new aiMaterial *[1];
    scene.mMaterials[0] = nullptr;
    scene.mNumMaterials = 1;

    scene.mMaterials[0] = new aiMaterial();

    scene.mMeshes[0] = new aiMesh();
    scene.mMeshes[0]->mMaterialIndex = 0;

    scene.mRootNode->mMeshes = new unsigned int[1];
    scene.mRootNode->mMeshes[0] = 0;
    scene.mRootNode->mNumMeshes = 1;

    auto pMesh = scene.mMeshes[0];

    size_t numValidPoints = points.size();

    pMesh->mVertices = new aiVector3D[numValidPoints];
    pMesh->mNumVertices = static_cast<unsigned int>(numValidPoints);

    int i = 0;
    for (XYZ &p : points) {
        pMesh->mVertices[i] = aiVector3D(p.x, p.y, p.z);
        ++i;
    }

    Assimp::Exporter mAiExporter;
    ExportProperties *properties = new ExportProperties;
    properties->SetPropertyBool(AI_CONFIG_EXPORT_POINT_CLOUDS, true);

    const char *stlFileName = "testExport.stl";
    mAiExporter.Export(&scene, "stl", stlFileName, 0, properties);

    // Cleanup, delete the exported file
    ::remove(stlFileName);
    delete properties;
}

#endif
