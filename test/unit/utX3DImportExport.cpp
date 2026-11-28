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
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <string>
#include <utility>
#include <vector>

using namespace Assimp;

// A one-triangle scene whose ImageTexture carries the given url attribute, quotes included.
static std::string texturedTriangle(const std::string &urlAttribute) {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           "<X3D profile=\"Interchange\" version=\"3.3\">\n"
           "  <Scene>\n"
           "    <Shape>\n"
           "      <Appearance>\n"
           "        <Material diffuseColor=\"1 1 1\"/>\n"
           "        <ImageTexture url=" + urlAttribute + "/>\n"
           "      </Appearance>\n"
           "      <IndexedFaceSet coordIndex=\"0 1 2 -1\" texCoordIndex=\"0 1 2 -1\">\n"
           "        <Coordinate point=\"0 0 0, 1 0 0, 0 1 0\"/>\n"
           "        <TextureCoordinate point=\"0 0, 1 0, 0 1\"/>\n"
           "      </IndexedFaceSet>\n"
           "    </Shape>\n"
           "  </Scene>\n"
           "</X3D>\n";
}

// The diffuse texture path of the scene's one mesh.
static std::string diffuseTexture(const aiScene *scene) {
    aiString path;
    EXPECT_EQ(1u, scene->mNumMeshes);
    const aiMaterial *material = scene->mMaterials[scene->mMeshes[0]->mMaterialIndex];
    EXPECT_EQ(aiReturn_SUCCESS, material->GetTexture(aiTextureType_DIFFUSE, 0, &path));
    return path.C_Str();
}

class utX3DImportExport : public AbstractImportExportBase {
public:
    bool importerTest() override {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/X3D/HelloX3dTrademark.x3d", aiProcess_ValidateDataStructure);
        return nullptr != scene;
    }
};

TEST_F(utX3DImportExport, importX3DFromFileTest) {
    EXPECT_TRUE(importerTest());
}

TEST_F(utX3DImportExport, importX3DIndexedLineSet) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/X3D/IndexedLineSet.x3d", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    ASSERT_EQ(scene->mNumMeshes, 1u);
    ASSERT_EQ(scene->mMeshes[0]->mNumFaces, 4u);
    ASSERT_EQ(scene->mMeshes[0]->mPrimitiveTypes, aiPrimitiveType_LINE);
    ASSERT_EQ(scene->mMeshes[0]->mNumVertices, 4u);
    for (unsigned int i = 0; i < scene->mMeshes[0]->mNumFaces; i++) {
        ASSERT_EQ(scene->mMeshes[0]->mFaces[i].mNumIndices, 2u);
    }
}

TEST_F(utX3DImportExport, importX3DComputerKeyboard) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/X3D/ComputerKeyboard.x3d", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    ASSERT_EQ(100u, scene->mNumMeshes);
}

TEST_F(utX3DImportExport, importX3DMalformedColorIndex) {
    // IndexedFaceSet whose colorIndex is longer than coordIndex (colorPerVertex=true).
    // add_color walked the coordIndex vector past its end, a heap out-of-bounds read.
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/X3D/invalid/MalformedColorIndex.x3d", 0);
    ASSERT_NE(nullptr, scene);
    ASSERT_EQ(scene->mNumMeshes, 1u);
    ASSERT_EQ(scene->mMeshes[0]->mNumVertices, 3u);
}

TEST_F(utX3DImportExport, importX3DChevyTahoe) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/X3D/Chevy/ChevyTahoe.x3d", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    ASSERT_EQ(20u, scene->mNumMeshes);
}

TEST_F(utX3DImportExport, importX3DEmptyGroupingNodes) {
    // Empty Group, StaticGroup, Switch, and Transform (childless or comment-only)
    // should not crash or corrupt the node hierarchy via double Node_Exit.
    const std::string xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<X3D profile=\"Immersive\" version=\"3.0\">\n"
        "  <Scene>\n"
        "    <Group/>\n"
        "    <StaticGroup/>\n"
        "    <Switch whichChoice=\"-1\"/>\n"
        "    <Transform><!-- comment only --></Transform>\n"
        "    <Transform translation=\"0 0 0\"/>\n"
        "    <Shape>\n"
        "      <Appearance>\n"
        "        <Material diffuseColor=\"1 0 0\"/>\n"
        "      </Appearance>\n"
        "      <IndexedFaceSet coordIndex=\"0 1 2 -1\">\n"
        "        <Coordinate point=\"0 0 0, 1 0 0, 0 1 0\"/>\n"
        "      </IndexedFaceSet>\n"
        "    </Shape>\n"
        "  </Scene>\n"
        "</X3D>\n";
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFileFromMemory(xml.c_str(), xml.length(), aiProcess_ValidateDataStructure, "x3d");
    ASSERT_NE(nullptr, scene);
    ASSERT_EQ(1u, scene->mNumMeshes);
    ASSERT_EQ(3u, scene->mMeshes[0]->mNumVertices);
    ASSERT_EQ(1u, scene->mMeshes[0]->mNumFaces);
}

TEST_F(utX3DImportExport, importX3DNestedEmptyGroupingNodes) {
    const std::string xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<X3D profile=\"Immersive\" version=\"3.0\">\n"
        "  <Scene>\n"
        "    <Group>\n"
        "      <Transform>\n"
        "        <StaticGroup>\n"
        "          <Switch whichChoice=\"-1\"/>\n"
        "        </StaticGroup>\n"
        "      </Transform>\n"
        "    </Group>\n"
        "    <Shape>\n"
        "      <Appearance>\n"
        "        <Material diffuseColor=\"0 1 0\"/>\n"
        "      </Appearance>\n"
        "      <IndexedFaceSet coordIndex=\"0 1 2 -1\">\n"
        "        <Coordinate point=\"0 0 0, 1 0 0, 0 1 0\"/>\n"
        "      </IndexedFaceSet>\n"
        "    </Shape>\n"
        "  </Scene>\n"
        "</X3D>\n";
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFileFromMemory(xml.c_str(), xml.length(), aiProcess_ValidateDataStructure, "x3d");
    ASSERT_NE(nullptr, scene);
    ASSERT_EQ(1u, scene->mNumMeshes);
}

TEST_F(utX3DImportExport, importX3DGroupingNodeUseReferences) {
    // Verify USE instancing on Group, StaticGroup, Switch, and Transform nodes,
    // ensuring parent hierarchy pointers are preserved and subsequent siblings are loaded.
    const std::string xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<X3D profile=\"Immersive\" version=\"3.0\">\n"
        "  <Scene>\n"
        "    <Transform DEF=\"OriginalTransform\" translation=\"0 1 0\">\n"
        "      <Shape>\n"
        "        <Appearance>\n"
        "          <Material diffuseColor=\"0 0 1\"/>\n"
        "        </Appearance>\n"
        "        <IndexedFaceSet coordIndex=\"0 1 2 -1\">\n"
        "          <Coordinate point=\"0 0 0, 1 0 0, 0 1 0\"/>\n"
        "        </IndexedFaceSet>\n"
        "      </Shape>\n"
        "    </Transform>\n"
        "    <Transform USE=\"OriginalTransform\"/>\n"
        "    <Group USE=\"OriginalTransform\"/>\n"
        "    <StaticGroup USE=\"OriginalTransform\"/>\n"
        "    <Switch whichChoice=\"0\">\n"
        "      <Transform USE=\"OriginalTransform\"/>\n"
        "    </Switch>\n"
        "    <Shape>\n"
        "      <Appearance>\n"
        "        <Material diffuseColor=\"1 1 0\"/>\n"
        "      </Appearance>\n"
        "      <IndexedFaceSet coordIndex=\"0 1 2 -1\">\n"
        "        <Coordinate point=\"0 0 0, 1 0 0, 0 1 0\"/>\n"
        "      </IndexedFaceSet>\n"
        "    </Shape>\n"
        "  </Scene>\n"
        "</X3D>\n";
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFileFromMemory(xml.c_str(), xml.length(), aiProcess_ValidateDataStructure, "x3d");
    ASSERT_NE(nullptr, scene);
    // Original (1) + 4 USE references (4) + trailing Shape (1) = 6 meshes total
    ASSERT_EQ(6u, scene->mNumMeshes);
}

TEST_F(utX3DImportExport, importX3DEmptyScene) {
    const std::string xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<X3D profile=\"Immersive\" version=\"3.0\">\n"
        "  <Scene/>\n"
        "</X3D>\n";
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFileFromMemory(xml.c_str(), xml.length(), 0, "x3d");
    ASSERT_NE(nullptr, scene);
    ASSERT_EQ(0u, scene->mNumMeshes);
    ASSERT_NE(nullptr, scene->mRootNode);
}

TEST_F(utX3DImportExport, importX3DImageTextureUrlStrings) {
    // An MFString is zero or more strings, each in double quotes, separated by whitespace
    // (ISO/IEC 19776-1, 5.15). ImageTexture takes the first one.
    const std::vector<std::pair<std::string, std::string>> cases = {
        { "'\"wood.png\"'", "wood.png" },
        { "\"&quot;wood.png&quot;\"", "wood.png" },
        { "'\"wood.png\" \"https://example.com/wood.png\"'", "wood.png" },
        { "'\"wood.png\" \"b.png\" \"c.png\"'", "wood.png" },
        { "'\"wood.png\",\"b.png\"'", "wood.png" },
        { "'\"wood.png\"\n\"b.png\"'", "wood.png" },
        { "'\"my  wood.png\"'", "my  wood.png" },
        { "'\"wo\\\"od.png\"'", "wo\"od.png" },
        { "\"wood.png\"", "wood.png" }, // unquoted: not valid X3D, but files carry it
    };
    for (const auto &c : cases) {
        SCOPED_TRACE(c.first);
        const std::string xml = texturedTriangle(c.first);
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFileFromMemory(xml.c_str(), xml.length(), aiProcess_ValidateDataStructure, "x3d");
        if (scene == nullptr) {
            ADD_FAILURE() << importer.GetErrorString();
            continue;
        }
        EXPECT_EQ(c.second, diffuseTexture(scene));
    }
}

TEST_F(utX3DImportExport, importX3DUnterminatedUrlString) {
    const std::string xml = texturedTriangle("'\"wood.png'");
    Assimp::Importer importer;
    EXPECT_EQ(nullptr, importer.ReadFileFromMemory(xml.c_str(), xml.length(), 0, "x3d"));
}

TEST_F(utX3DImportExport, importX3DEmptyInlineUrl) {
    // An Inline whose only url is the empty string names no file to load.
    std::string xml = texturedTriangle("'\"wood.png\"'");
    xml.insert(xml.find("<Scene>") + 7, "<Inline url='\"\"'/>");
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFileFromMemory(xml.c_str(), xml.length(), aiProcess_ValidateDataStructure, "x3d");
    ASSERT_NE(nullptr, scene) << importer.GetErrorString();
    EXPECT_EQ("wood.png", diffuseTexture(scene));
}

#ifndef ASSIMP_BUILD_NO_EXPORT
TEST_F(utX3DImportExport, exportX3DImageTextureReadsBack) {
    // The X3D exporter writes the texture as one quoted string, url='"wood.png"'.
    const std::string xml = texturedTriangle("\"wood.png\"");
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFileFromMemory(xml.c_str(), xml.length(), aiProcess_ValidateDataStructure, "x3d");
    ASSERT_NE(nullptr, scene);
    Assimp::Exporter exporter;
    const aiExportDataBlob *blob = exporter.ExportToBlob(scene, "x3d");
    ASSERT_NE(nullptr, blob);
    const std::string written(static_cast<const char *>(blob->data), blob->size);
    EXPECT_NE(std::string::npos, written.find("url='\"wood.png\"'"));

    Assimp::Importer reader;
    const aiScene *readBack = reader.ReadFileFromMemory(blob->data, blob->size, aiProcess_ValidateDataStructure, "x3d");
    ASSERT_NE(nullptr, readBack) << reader.GetErrorString();
    EXPECT_EQ("wood.png", diffuseTexture(readBack));
}
#endif // ASSIMP_BUILD_NO_EXPORT
