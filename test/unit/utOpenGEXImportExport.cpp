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

#include <assimp/scene.h>
#include "UnitTestPCH.h"

#include <assimp/Importer.hpp>

#include <string>

using namespace Assimp;

class utOpenGEXImportExport : public AbstractImportExportBase {
public:
    bool importerTest() override {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/OpenGEX/Example.ogex", 0);
        EXPECT_EQ(1u, scene->mNumMeshes);
        return nullptr != scene;
    }
};

TEST_F(utOpenGEXImportExport, importOpenGexFromFileTest) {
    EXPECT_TRUE(importerTest());
}

TEST_F(utOpenGEXImportExport, Importissue1262_NoCrash) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/OpenGEX/light_issue1262.ogex", 0);
    EXPECT_NE(nullptr, scene);
}

TEST_F(utOpenGEXImportExport, Importissue1340_EmptyCameraObject) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/OpenGEX/empty_camera.ogex", 0);
    EXPECT_NE(nullptr, scene);
}

TEST_F(utOpenGEXImportExport, importSeveralFilesWithOneImporter) {
    // A node that references a mesh and a material, and a camera.
    static const std::string file =
            "Metric (key = \"distance\") {float {1.0}}\n"
            "GeometryNode $node1\n{\n"
            "    Name {string {\"tri\"}}\n"
            "    ObjectRef {ref {$geometry1}}\n"
            "    MaterialRef (index = 0) {ref {$material1}}\n"
            "}\n"
            "CameraNode $node2\n{\n"
            "    Name {string {\"cam\"}}\n"
            "}\n"
            "GeometryObject $geometry1\n{\n"
            "    Mesh (primitive = \"triangles\")\n    {\n"
            "        VertexArray (attrib = \"position\") {float[3] {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}}\n"
            "        IndexArray {unsigned_int32[3] {{0, 1, 2}}}\n"
            "    }\n"
            "}\n"
            "Material $material1\n{\n"
            "    Color (attrib = \"diffuse\") {float[3] {{1, 0, 0}}}\n"
            "}\n";
    // The import fails at the out-of-range index, after the mesh has been created.
    static const std::string broken =
            "Metric (key = \"distance\") {float {1.0}}\n"
            "GeometryObject $geometry1\n{\n"
            "    Mesh (primitive = \"triangles\")\n    {\n"
            "        VertexArray (attrib = \"position\") {float[3] {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}}\n"
            "        IndexArray {unsigned_int32[3] {{0, 1, 3}}}\n"
            "    }\n"
            "}\n";

    Assimp::Importer importer;
    const std::string *files[] = { &file, &file, &broken, &file };
    for (const std::string *current : files) {
        const aiScene *scene = importer.ReadFileFromMemory(current->data(), current->size(), 0, "ogex");
        if (current == &broken) {
            EXPECT_EQ(nullptr, scene);
            continue;
        }
        ASSERT_NE(nullptr, scene);
        ASSERT_EQ(1u, scene->mNumMeshes);
        EXPECT_EQ(3u, scene->mMeshes[0]->mNumVertices);
        EXPECT_EQ(1u, scene->mNumMaterials);
        EXPECT_EQ(1u, scene->mNumCameras);
        ASSERT_NE(nullptr, scene->mRootNode);
        ASSERT_EQ(2u, scene->mRootNode->mNumChildren);
        const aiNode *node = scene->mRootNode->mChildren[0];
        EXPECT_STREQ("tri", node->mName.C_Str());
        ASSERT_EQ(1u, node->mNumMeshes);
        EXPECT_EQ(0u, node->mMeshes[0]);
    }
}

TEST_F(utOpenGEXImportExport, vertexAttributesAreNotCarriedToTheNextMesh) {
    static const std::string attributes =
            "        VertexArray (attrib = \"normal\") {float[3] {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}}}\n"
            "        VertexArray (attrib = \"color\") {float[3] {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}}\n"
            "        VertexArray (attrib = \"texcoord\") {float[2] {{0, 0}, {1, 0}, {0, 1}}}\n";
    static const std::string begin =
            "    Mesh (primitive = \"triangles\")\n    {\n"
            "        VertexArray (attrib = \"position\") {float[3] {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}}\n";
    static const std::string end =
            "        IndexArray {unsigned_int32[3] {{0, 1, 2}}}\n"
            "    }\n"
            "}\n";
    // The first and the third mesh have normals, colors and texture coordinates, the second has none.
    const std::string file =
            "GeometryObject $geometry1\n{\n" + begin + attributes + end +
            "GeometryObject $geometry2\n{\n" + begin + end +
            "GeometryObject $geometry3\n{\n" + begin + attributes + end;

    Assimp::Importer importer;
    for (int i = 0; i < 2; ++i) {
        const aiScene *scene = importer.ReadFileFromMemory(file.data(), file.size(), 0, "ogex");
        ASSERT_NE(nullptr, scene);
        ASSERT_EQ(3u, scene->mNumMeshes);
        for (unsigned int m = 0; m < 3; ++m) {
            const bool hasAttributes = m != 1;
            EXPECT_EQ(hasAttributes, scene->mMeshes[m]->HasNormals());
            EXPECT_EQ(hasAttributes, scene->mMeshes[m]->HasVertexColors(0));
            EXPECT_EQ(hasAttributes, scene->mMeshes[m]->HasTextureCoords(0));
        }
    }
}
