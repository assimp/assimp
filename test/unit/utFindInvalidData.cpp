#include "UnitTestPCH.h"

#include <FindInvalidDataProcess.h>
#include "../../include/assimp/mesh.h"


using namespace std;
using namespace Assimp;

class FindInvalidDataProcessTest : public ::testing::Test
{
public:

    virtual void SetUp();
    virtual void TearDown();

protected:

    aiMesh* pcMesh;
    FindInvalidDataProcess* piProcess;
};

// ------------------------------------------------------------------------------------------------
void FindInvalidDataProcessTest::SetUp()
{
    ASSERT_TRUE( AI_MAX_NUMBER_OF_TEXTURECOORDS >= 3);

    piProcess = new FindInvalidDataProcess();
    pcMesh = new aiMesh();

    pcMesh->mNumVertices = 1000;
    pcMesh->mVertices = new aiVector3D[1000];
    for (unsigned int i = 0; i < 1000;++i)
        pcMesh->mVertices[i] = aiVector3D((float)i);

    pcMesh->mNormals = new aiVector3D[1000];
    for (unsigned int i = 0; i < 1000;++i)
        pcMesh->mNormals[i] = aiVector3D((float)i+1);

    pcMesh->mTangents = new aiVector3D[1000];
    for (unsigned int i = 0; i < 1000;++i)
        pcMesh->mTangents[i] = aiVector3D((float)i);

    pcMesh->mBitangents = new aiVector3D[1000];
    for (unsigned int i = 0; i < 1000;++i)
        pcMesh->mBitangents[i] = aiVector3D((float)i);

    for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS;++a)
    {
        pcMesh->mTextureCoords[a] = new aiVector3D[1000];
        for (unsigned int i = 0; i < 1000;++i)
            pcMesh->mTextureCoords[a][i] = aiVector3D((float)i);
    }
}

// ------------------------------------------------------------------------------------------------
void FindInvalidDataProcessTest::TearDown()
{
    delete piProcess;
    delete pcMesh;
}

// ------------------------------------------------------------------------------------------------
TEST_F(FindInvalidDataProcessTest, testStepNegativeResult)
{
    ::memset(pcMesh->mNormals,0,pcMesh->mNumVertices*sizeof(aiVector3D));
    ::memset(pcMesh->mBitangents,0,pcMesh->mNumVertices*sizeof(aiVector3D));

    pcMesh->mTextureCoords[2][455] = aiVector3D( std::numeric_limits<float>::quiet_NaN() );

    piProcess->ProcessMesh(pcMesh);

    EXPECT_TRUE(NULL != pcMesh->mVertices);
    EXPECT_TRUE(NULL == pcMesh->mNormals);
    EXPECT_TRUE(NULL == pcMesh->mTangents);
    EXPECT_TRUE(NULL == pcMesh->mBitangents);

    for (unsigned int i = 0; i < 2;++i)
        EXPECT_TRUE(NULL != pcMesh->mTextureCoords[i]);

    for (unsigned int i = 2; i < AI_MAX_NUMBER_OF_TEXTURECOORDS;++i)
        EXPECT_TRUE(NULL == pcMesh->mTextureCoords[i]);
}

// ------------------------------------------------------------------------------------------------
TEST_F(FindInvalidDataProcessTest, testStepPositiveResult)
{
    piProcess->ProcessMesh(pcMesh);

    EXPECT_TRUE(NULL != pcMesh->mVertices);

    EXPECT_TRUE(NULL != pcMesh->mNormals);
    EXPECT_TRUE(NULL != pcMesh->mTangents);
    EXPECT_TRUE(NULL != pcMesh->mBitangents);

    for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS;++i)
        EXPECT_TRUE(NULL != pcMesh->mTextureCoords[i]);
}
