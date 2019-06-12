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

#include <assimp/scene.h>
#include "PostProcessing/RemoveRedundantMaterials.h"
#include "Material/MaterialSystem.h"

using namespace std;
using namespace Assimp;

class RemoveRedundantMatsTest : public ::testing::Test {
public:
    virtual void SetUp();
    virtual void TearDown();

protected:
    RemoveRedundantMatsProcess* piProcess;
    aiScene* pcScene1;
};

// ------------------------------------------------------------------------------------------------
aiMaterial* getUniqueMaterial1()
{
    // setup an unique name for each material - this shouldn't care
    aiString mTemp;
    mTemp.Set("UniqueMat1");

    aiMaterial* pcMat = new aiMaterial();
    pcMat->AddProperty(&mTemp,AI_MATKEY_NAME);
    float f = 2.0f;
    pcMat->AddProperty<float>(&f, 1, AI_MATKEY_BUMPSCALING);
    pcMat->AddProperty<float>(&f, 1, AI_MATKEY_SHININESS_STRENGTH);
    return pcMat;
}

// ------------------------------------------------------------------------------------------------
aiMaterial* getUniqueMaterial2()
{
    // setup an unique name for each material - this shouldn't care
    aiString mTemp;
    mTemp.Set("Unique Mat2");

    aiMaterial* pcMat = new aiMaterial();
    pcMat->AddProperty(&mTemp,AI_MATKEY_NAME);
    float f = 4.0f;int i = 1;
    pcMat->AddProperty<float>(&f, 1, AI_MATKEY_BUMPSCALING);
    pcMat->AddProperty<int>(&i, 1, AI_MATKEY_ENABLE_WIREFRAME);
    return pcMat;
}

// ------------------------------------------------------------------------------------------------
aiMaterial* getUniqueMaterial3()
{
    // setup an unique name for each material - this shouldn't care
    aiString mTemp;
    mTemp.Set("Complex material name");

    aiMaterial* pcMat = new aiMaterial();
    pcMat->AddProperty(&mTemp,AI_MATKEY_NAME);
    return pcMat;
}

// ------------------------------------------------------------------------------------------------
void RemoveRedundantMatsTest::SetUp()
{
    // construct the process
    piProcess = new RemoveRedundantMatsProcess();

    // create a scene with 5 materials (2 is a duplicate of 0, 3 of 1)
    pcScene1 = new aiScene();
    pcScene1->mNumMaterials = 5;
    pcScene1->mMaterials = new aiMaterial*[5];

    pcScene1->mMaterials[0] = getUniqueMaterial1();
    pcScene1->mMaterials[1] = getUniqueMaterial2();
    pcScene1->mMaterials[4] = getUniqueMaterial3();

    // all materials must be referenced
    pcScene1->mNumMeshes = 5;
    pcScene1->mMeshes = new aiMesh*[5];
    for (unsigned int i = 0; i < 5;++i) {
        pcScene1->mMeshes[i] = new aiMesh();
        pcScene1->mMeshes[i]->mMaterialIndex = i;
    }

    // setup an unique name for each material - this shouldn't care
    aiString mTemp;
    mTemp.length = 1;
    mTemp.data[0] = 48;
    mTemp.data[1] = 0;

    aiMaterial* pcMat;
    pcScene1->mMaterials[2] = pcMat = new aiMaterial();
    aiMaterial::CopyPropertyList(pcMat,(const aiMaterial*)pcScene1->mMaterials[0]);
    pcMat->AddProperty(&mTemp,AI_MATKEY_NAME);
    mTemp.data[0]++;

    pcScene1->mMaterials[3] = pcMat = new aiMaterial();
    aiMaterial::CopyPropertyList(pcMat,(const aiMaterial*)pcScene1->mMaterials[1]);
    pcMat->AddProperty(&mTemp,AI_MATKEY_NAME);
    mTemp.data[0]++;
}

// ------------------------------------------------------------------------------------------------
void RemoveRedundantMatsTest::TearDown()
{
    delete piProcess;
    delete pcScene1;
}

// ------------------------------------------------------------------------------------------------
TEST_F(RemoveRedundantMatsTest, testRedundantMaterials)
{
    piProcess->SetFixedMaterialsString();

    piProcess->Execute(pcScene1);
    EXPECT_EQ(3U, pcScene1->mNumMaterials);
    EXPECT_TRUE(0 != pcScene1->mMaterials &&
        0 != pcScene1->mMaterials[0] &&
        0 != pcScene1->mMaterials[1] &&
        0 != pcScene1->mMaterials[2]);

    aiString sName;
    EXPECT_EQ(AI_SUCCESS, aiGetMaterialString(pcScene1->mMaterials[2],AI_MATKEY_NAME,&sName));
    EXPECT_STREQ("Complex material name", sName.data);

}

// ------------------------------------------------------------------------------------------------
TEST_F(RemoveRedundantMatsTest, testRedundantMaterialsWithExcludeList)
{
    piProcess->SetFixedMaterialsString("\'Unique Mat2\'\t\'Complex material name\' and_another_one_which_we_wont_use");

    piProcess->Execute(pcScene1);
    EXPECT_EQ(4U, pcScene1->mNumMaterials);
    EXPECT_TRUE(0 != pcScene1->mMaterials &&
        0 != pcScene1->mMaterials[0] &&
        0 != pcScene1->mMaterials[1] &&
        0 != pcScene1->mMaterials[2] &&
        0 != pcScene1->mMaterials[3]);

    aiString sName;
    EXPECT_EQ(AI_SUCCESS, aiGetMaterialString(pcScene1->mMaterials[3],AI_MATKEY_NAME,&sName));
    EXPECT_STREQ("Complex material name", sName.data);
}
