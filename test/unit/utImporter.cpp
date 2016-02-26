/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2016, assimp team

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

#include "../../include/assimp/postprocess.h"
#include "../../include/assimp/scene.h"
#include <assimp/Importer.hpp>
#include <BaseImporter.h>


using namespace std;
using namespace Assimp;

class ImporterTest : public ::testing::Test
{
public:

    virtual void SetUp() { pImp = new Importer(); }
    virtual void TearDown() { delete pImp; }

protected:
    Importer* pImp;
};

#define InputData_BLOCK_SIZE 1310

// test data for Importer::ReadFileFromMemory() - ./test/3DS/CameraRollAnim.3ds
static unsigned char InputData_abRawBlock[1310] = {
77,77,30,5,0,0,2,0,10,0,0,0,3,0,0,0,61,61,91,3,0,0,62,61,10,0,0,0,3,0,0,0,
0,1,10,0,0,0,0,0,128,63,0,64,254,2,0,0,66,111,120,48,49,0,0,65,242,2,0,0,16,65,64,1,
0,0,26,0,102,74,198,193,102,74,198,193,0,0,0,0,205,121,55,66,102,74,198,193,0,0,0,0,102,74,198,193,
138,157,184,65,0,0,0,0,205,121,55,66,138,157,184,65,0,0,0,0,102,74,198,193,102,74,198,193,90,252,26,66,
205,121,55,66,102,74,198,193,90,252,26,66,102,74,198,193,138,157,184,65,90,252,26,66,205,121,55,66,138,157,184,65,
90,252,26,66,102,74,198,193,102,74,198,193,0,0,0,0,205,121,55,66,102,74,198,193,0,0,0,0,205,121,55,66,
102,74,198,193,90,252,26,66,205,121,55,66,102,74,198,193,90,252,26,66,102,74,198,193,102,74,198,193,90,252,26,66,
102,74,198,193,102,74,198,193,0,0,0,0,205,121,55,66,138,157,184,65,0,0,0,0,205,121,55,66,102,74,198,193,
90,252,26,66,205,121,55,66,138,157,184,65,0,0,0,0,102,74,198,193,138,157,184,65,0,0,0,0,102,74,198,193,
138,157,184,65,90,252,26,66,102,74,198,193,138,157,184,65,90,252,26,66,205,121,55,66,138,157,184,65,90,252,26,66,
205,121,55,66,138,157,184,65,0,0,0,0,102,74,198,193,138,157,184,65,0,0,0,0,102,74,198,193,102,74,198,193,
90,252,26,66,102,74,198,193,102,74,198,193,90,252,26,66,102,74,198,193,138,157,184,65,0,0,0,0,64,65,216,0,
0,0,26,0,0,0,128,63,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,63,0,0,128,63,0,0,0,0,
0,0,128,63,0,0,0,0,0,0,0,0,0,0,128,63,0,0,0,0,0,0,0,0,0,0,128,63,0,0,128,63,
0,0,128,63,0,0,0,0,0,0,0,0,0,0,128,63,0,0,0,0,0,0,128,63,0,0,128,63,0,0,128,63,
0,0,128,63,0,0,0,0,0,0,128,63,0,0,0,0,0,0,0,0,0,0,128,63,0,0,0,0,0,0,0,0,
0,0,128,63,0,0,0,0,0,0,0,0,0,0,128,63,0,0,0,0,0,0,128,63,0,0,128,63,0,0,128,63,
0,0,128,63,0,0,0,0,0,0,128,63,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,63,
0,0,128,63,0,0,128,63,0,0,128,63,0,0,0,0,0,0,0,0,96,65,54,0,0,0,0,0,128,63,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,128,63,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,63,53,169,
40,65,176,205,90,191,0,0,0,0,32,65,158,0,0,0,12,0,0,0,2,0,3,0,6,0,3,0,1,0,0,0,
6,0,4,0,5,0,7,0,6,0,7,0,6,0,4,0,6,0,8,0,9,0,10,0,6,0,11,0,12,0,13,0,
6,0,1,0,14,0,7,0,6,0,7,0,15,0,1,0,6,0,16,0,17,0,18,0,6,0,19,0,20,0,21,0,
6,0,22,0,0,0,23,0,6,0,24,0,6,0,25,0,6,0,80,65,54,0,0,0,2,0,0,0,2,0,0,0,
4,0,0,0,4,0,0,0,8,0,0,0,8,0,0,0,16,0,0,0,16,0,0,0,32,0,0,0,32,0,0,0,
64,0,0,0,64,0,0,0,0,64,67,0,0,0,67,97,109,101,114,97,48,49,0,0,71,52,0,0,0,189,19,25,
195,136,104,81,64,147,56,182,65,96,233,20,194,67,196,97,190,147,56,182,65,0,0,0,0,85,85,85,66,32,71,14,
0,0,0,0,0,0,0,0,0,122,68,0,176,179,1,0,0,10,176,21,0,0,0,5,0,77,65,88,83,67,69,78,
69,0,44,1,0,0,8,176,14,0,0,0,0,0,0,0,44,1,0,0,9,176,10,0,0,0,128,2,0,0,2,176,
168,0,0,0,48,176,8,0,0,0,0,0,16,176,18,0,0,0,66,111,120,48,49,0,0,64,0,0,255,255,19,176,
18,0,0,0,0,0,0,128,0,0,0,128,0,0,0,128,32,176,38,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,0,0,0,0,0,0,0,0,0,53,169,40,65,176,205,90,191,0,0,0,0,33,176,42,0,0,0,0,0,0,0,
0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
34,176,38,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,128,63,0,0,
128,63,0,0,128,63,3,176,143,0,0,0,48,176,8,0,0,0,1,0,16,176,21,0,0,0,67,97,109,101,114,97,
48,49,0,0,64,0,0,255,255,32,176,38,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,
0,0,0,189,19,25,195,136,104,81,64,147,56,182,65,35,176,30,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
0,0,0,0,0,0,0,0,0,0,0,52,66,36,176,40,0,0,0,0,0,0,0,0,0,120,0,0,0,2,0,0,
0,0,0,0,0,0,0,120,13,90,189,120,0,0,0,0,0,99,156,154,194,4,176,73,0,0,0,48,176,8,0,0,
0,2,0,16,176,21,0,0,0,67,97,109,101,114,97,48,49,0,0,64,0,0,255,255,32,176,38,0,0,0,0,0,
0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,96,233,20,194,67,196,97,190,147,56,182,65,
};

#define AIUT_DEF_ERROR_TEXT "sorry, this is a test"


static const aiImporterDesc desc = {
    "UNIT TEST - IMPORTER",
    "",
    "",
    "",
    0,
    0,
    0,
    0,
    0,
    "apple mac linux windows"
};


class TestPlugin : public BaseImporter
{
public:

    virtual bool CanRead(
        const std::string& pFile, IOSystem* /*pIOHandler*/, bool /*test*/) const
    {
        std::string::size_type pos = pFile.find_last_of('.');
        // no file extension - can't read
        if( pos == std::string::npos)
            return false;
        std::string extension = pFile.substr( pos);

        // todo ... make case-insensitive
        return (extension == ".apple" || extension == ".mac" ||
            extension == ".linux" || extension == ".windows" );

    }

    virtual const aiImporterDesc* GetInfo () const
    {
        return & desc;
    }

    virtual void InternReadFile(
        const std::string& /*pFile*/, aiScene* /*pScene*/, IOSystem* /*pIOHandler*/)
    {
        throw DeadlyImportError(AIUT_DEF_ERROR_TEXT);
    }
};

// ------------------------------------------------------------------------------------------------
TEST_F(ImporterTest, testMemoryRead)
{
    const aiScene* sc = pImp->ReadFileFromMemory(InputData_abRawBlock,InputData_BLOCK_SIZE,
        aiProcessPreset_TargetRealtime_Quality,"3ds");

    ASSERT_TRUE(sc != NULL);
    EXPECT_EQ(aiString("<3DSRoot>"), sc->mRootNode->mName);
    EXPECT_EQ(1U, sc->mNumMeshes);
    EXPECT_EQ(24U, sc->mMeshes[0]->mNumVertices);
    EXPECT_EQ(12U, sc->mMeshes[0]->mNumFaces);
}

// ------------------------------------------------------------------------------------------------
TEST_F(ImporterTest, testIntProperty)
{
    bool b = pImp->SetPropertyInteger("quakquak",1503);
    EXPECT_FALSE(b);
    EXPECT_EQ(1503, pImp->GetPropertyInteger("quakquak",0));
    EXPECT_EQ(314159, pImp->GetPropertyInteger("not_there",314159));

    b = pImp->SetPropertyInteger("quakquak",1504);
    EXPECT_TRUE(b);
}

// ------------------------------------------------------------------------------------------------
TEST_F(ImporterTest, testFloatProperty)
{
    bool b = pImp->SetPropertyFloat("quakquak",1503.f);
    EXPECT_TRUE(!b);
    EXPECT_EQ(1503.f, pImp->GetPropertyFloat("quakquak",0.f));
    EXPECT_EQ(314159.f, pImp->GetPropertyFloat("not_there",314159.f));
}

// ------------------------------------------------------------------------------------------------
TEST_F(ImporterTest, testStringProperty)
{
    bool b = pImp->SetPropertyString("quakquak","test");
    EXPECT_TRUE(!b);
    EXPECT_EQ("test", pImp->GetPropertyString("quakquak","weghwekg"));
    EXPECT_EQ("ILoveYou", pImp->GetPropertyString("not_there","ILoveYou"));
}

// ------------------------------------------------------------------------------------------------
TEST_F(ImporterTest, testPluginInterface)
{
    pImp->RegisterLoader(new TestPlugin());
    EXPECT_TRUE(pImp->IsExtensionSupported(".apple"));
    EXPECT_TRUE(pImp->IsExtensionSupported(".mac"));
    EXPECT_TRUE(pImp->IsExtensionSupported("*.linux"));
    EXPECT_TRUE(pImp->IsExtensionSupported("windows"));
    EXPECT_TRUE(pImp->IsExtensionSupported(".x")); /* x and 3ds must be available in this Assimp build, of course! */
    EXPECT_TRUE(pImp->IsExtensionSupported(".3ds"));
    EXPECT_FALSE(pImp->IsExtensionSupported("."));

    TestPlugin* p = (TestPlugin*) pImp->GetImporter(".windows");
    ASSERT_TRUE(NULL != p);

    try {
        p->InternReadFile("",0,NULL);
    }
    catch ( const DeadlyImportError& dead)
    {
        EXPECT_TRUE(!strcmp(dead.what(),AIUT_DEF_ERROR_TEXT));

        // unregister the plugin and delete it
        pImp->UnregisterLoader(p);
        delete p;

        return;
    }
    EXPECT_TRUE(false); // control shouldn't reach this point
}

// ------------------------------------------------------------------------------------------------
TEST_F(ImporterTest, testExtensionCheck)
{
    std::string s;
    pImp->GetExtensionList(s);

    // TODO
}

// ------------------------------------------------------------------------------------------------
TEST_F(ImporterTest, testMultipleReads)
{
    // see http://sourceforge.net/projects/assimp/forums/forum/817654/topic/3591099
    // Check whether reading and post-processing multiple times using
    // the same objects is *generally* fine. This test doesn't target
    // importers. Testing post-processing stability is the main point.

    const unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_ValidateDataStructure |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_SortByPType |
        aiProcess_FindDegenerates |
        aiProcess_FindInvalidData |
        aiProcess_GenUVCoords |
        aiProcess_OptimizeMeshes |
        aiProcess_OptimizeGraph;

    EXPECT_TRUE(pImp->ReadFile(ASSIMP_TEST_MODELS_DIR "/X/test.x",flags));
    //EXPECT_TRUE(pImp->ReadFile(ASSIMP_TEST_MODELS_DIR "/X/dwarf.x",flags)); # is in nonbsd
    EXPECT_TRUE(pImp->ReadFile(ASSIMP_TEST_MODELS_DIR "/X/Testwuson.X",flags));
    EXPECT_TRUE(pImp->ReadFile(ASSIMP_TEST_MODELS_DIR "/X/anim_test.x",flags));
    //EXPECT_TRUE(pImp->ReadFile(ASSIMP_TEST_MODELS_DIR "/X/dwarf.x",flags)); # is in nonbsd

    EXPECT_TRUE(pImp->ReadFile(ASSIMP_TEST_MODELS_DIR "/X/anim_test.x",flags));
    EXPECT_TRUE(pImp->ReadFile(ASSIMP_TEST_MODELS_DIR "/X/BCN_Epileptic.X",flags));
    //EXPECT_TRUE(pImp->ReadFile(ASSIMP_TEST_MODELS_DIR "/X/dwarf.x",flags)); # is in nonbsd
}
