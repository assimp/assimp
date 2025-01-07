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
    virtual bool importerTest() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/STL/Spider_ascii.stl", aiProcess_ValidateDataStructure);
        return nullptr != scene;
    }
};

TEST_F(utSTLImporterExporter, importSTLFromFileTest) {
    EXPECT_TRUE(importerTest());
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
