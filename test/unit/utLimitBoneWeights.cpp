#include "UnitTestPCH.h"

#include <assimp/scene.h>
#include <LimitBoneWeightsProcess.h>


using namespace std;
using namespace Assimp;

class LimitBoneWeightsTest : public ::testing::Test {
public:

    virtual void SetUp();
    virtual void TearDown();

protected:

    LimitBoneWeightsProcess* piProcess;
    aiMesh* pcMesh;
};

// ------------------------------------------------------------------------------------------------
void LimitBoneWeightsTest::SetUp()
{
    // construct the process
    this->piProcess = new LimitBoneWeightsProcess();

    // now need to create a nice mesh for testing purposes
    this->pcMesh = new aiMesh();

    pcMesh->mNumVertices = 500;
    pcMesh->mVertices = new aiVector3D[500]; // uninit.
    pcMesh->mNumBones = 30;
    pcMesh->mBones = new aiBone*[30];
    unsigned int iCur = 0;
    for (unsigned int i = 0; i < 30;++i)
    {
        aiBone* pc = pcMesh->mBones[i] = new aiBone();
        pc->mNumWeights = 250;
        pc->mWeights = new aiVertexWeight[pc->mNumWeights];
        for (unsigned int qq = 0; qq < pc->mNumWeights;++qq)
        {
            aiVertexWeight& v = pc->mWeights[qq];
            v.mVertexId = iCur++;
            if (500 == iCur)iCur = 0;
            v.mWeight = 1.0f / 15; // each vertex should occur once in two bones
        }
    }
}

// ------------------------------------------------------------------------------------------------
void LimitBoneWeightsTest::TearDown()
{
    delete pcMesh;
    delete piProcess;
}

// ------------------------------------------------------------------------------------------------
TEST_F(LimitBoneWeightsTest, testProcess)
{
    // execute the step on the given data
    piProcess->ProcessMesh(pcMesh);

    // check whether everything is ok ...
    typedef std::vector<LimitBoneWeightsProcess::Weight> VertexWeightList;
    VertexWeightList* asWeights = new VertexWeightList[pcMesh->mNumVertices];

    for (unsigned int i = 0; i < pcMesh->mNumVertices;++i)
        asWeights[i].reserve(4);

    // sort back as per-vertex lists
    for (unsigned int i = 0; i < pcMesh->mNumBones;++i)
    {
        aiBone& pcBone = **(pcMesh->mBones+i);
        for (unsigned int q = 0; q < pcBone.mNumWeights;++q)
        {
            aiVertexWeight weight = pcBone.mWeights[q];
            asWeights[weight.mVertexId].push_back(LimitBoneWeightsProcess::Weight (i,weight.mWeight));
        }
    }

    // now validate the size of the lists and check whether all weights sum to 1.0f
    for (unsigned int i = 0; i < pcMesh->mNumVertices;++i)
    {
        EXPECT_LE(asWeights[i].size(), 4U);
        float fSum = 0.0f;
        for (VertexWeightList::const_iterator
            iter =  asWeights[i].begin();
            iter != asWeights[i].end();++iter)
        {
            fSum += (*iter).mWeight;
        }
        EXPECT_GE(fSum, 0.95F);
        EXPECT_LE(fSum, 1.04F);
    }

    // delete allocated storage
    delete[] asWeights;

    // everything seems to be OK
}
