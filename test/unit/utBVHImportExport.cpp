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
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <string>

using namespace Assimp;

class utBVHImportExport : public AbstractImportExportBase {
public:
    bool importerTest() override {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BVH/01_01.bvh", aiProcess_ValidateDataStructure);
        return nullptr != scene;
    }
};

TEST_F(utBVHImportExport, import01_01) {
    EXPECT_TRUE(importerTest());
}


TEST_F(utBVHImportExport, import01_03) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BVH/01_03.bvh", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utBVHImportExport, importBoxingToes) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BVH/Boxing_Toes.bvh", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST_F(utBVHImportExport, importSeveralFilesWithOneImporter) {
    static const std::string hierarchy =
            "HIERARCHY\n"
            "ROOT Hips\n{\n"
            "  OFFSET 0 0 0\n"
            "  CHANNELS 6 Xposition Yposition Zposition Zrotation Xrotation Yrotation\n"
            "  JOINT Spine\n  {\n"
            "    OFFSET 0 5 0\n"
            "    CHANNELS 3 Zrotation Xrotation Yrotation\n"
            "    End Site\n    {\n      OFFSET 0 5 0\n    }\n"
            "  }\n"
            "}\n"
            "MOTION\nFrames: 2\nFrame Time: 0.0333\n";
    const std::string file = hierarchy + "0 0 0 0 0 0 0 0 0\n1 0 0 10 0 0 5 0 0\n";
    // The same motion, followed by values that the loader does not need.
    const std::string longFile = file + "2 2 2 2 2 2 2 2 2\n3 3 3 3 3 3 3 3 3\n4 4 4 4 4 4 4 4 4\n5 5 5 5 5 5 5 5 5\n";

    Assimp::Importer importer;
    const std::string *files[] = { &file, &longFile, &file };
    for (const std::string *current : files) {
        const aiScene *scene = importer.ReadFileFromMemory(current->data(), current->size(), 0, "bvh");
        ASSERT_NE(nullptr, scene) << importer.GetErrorString();
        ASSERT_EQ(1u, scene->mNumAnimations);
        const aiAnimation *animation = scene->mAnimations[0];
        ASSERT_EQ(2u, animation->mNumChannels);
        EXPECT_STREQ("Hips", animation->mChannels[0]->mNodeName.C_Str());
        EXPECT_STREQ("Spine", animation->mChannels[1]->mNodeName.C_Str());
        ASSERT_EQ(2u, animation->mChannels[0]->mNumPositionKeys);
        EXPECT_EQ(aiVector3D(1, 0, 0), animation->mChannels[0]->mPositionKeys[1].mValue);
    }
}
