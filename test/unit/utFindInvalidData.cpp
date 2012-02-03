
#include "UnitTestPCH.h"
#include "utFindInvalidData.h"


CPPUNIT_TEST_SUITE_REGISTRATION (FindInvalidDataProcessTest);

// ------------------------------------------------------------------------------------------------
void FindInvalidDataProcessTest :: setUp (void)
{
	CPPUNIT_ASSERT( AI_MAX_NUMBER_OF_TEXTURECOORDS >= 3);

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
void FindInvalidDataProcessTest :: tearDown (void)
{
	delete piProcess;
	delete pcMesh;
}

// ------------------------------------------------------------------------------------------------
void FindInvalidDataProcessTest :: testStepNegativeResult (void)
{
	::memset(pcMesh->mNormals,0,pcMesh->mNumVertices*sizeof(aiVector3D));
	::memset(pcMesh->mBitangents,0,pcMesh->mNumVertices*sizeof(aiVector3D));

	pcMesh->mTextureCoords[2][455] = aiVector3D( std::numeric_limits<float>::quiet_NaN() );
	
	piProcess->ProcessMesh(pcMesh);

	CPPUNIT_ASSERT(NULL != pcMesh->mVertices);
	CPPUNIT_ASSERT(NULL == pcMesh->mNormals);
	CPPUNIT_ASSERT(NULL == pcMesh->mTangents);
	CPPUNIT_ASSERT(NULL == pcMesh->mBitangents);

	
	for (unsigned int i = 0; i < 2;++i)
		CPPUNIT_ASSERT(NULL != pcMesh->mTextureCoords[i]);
	
	for (unsigned int i = 2; i < AI_MAX_NUMBER_OF_TEXTURECOORDS;++i)
		CPPUNIT_ASSERT(NULL == pcMesh->mTextureCoords[i]);
}

// ------------------------------------------------------------------------------------------------
void FindInvalidDataProcessTest :: testStepPositiveResult (void)
{
	piProcess->ProcessMesh(pcMesh);

	CPPUNIT_ASSERT(NULL != pcMesh->mVertices);

	CPPUNIT_ASSERT(NULL != pcMesh->mNormals);
	CPPUNIT_ASSERT(NULL != pcMesh->mTangents);
	CPPUNIT_ASSERT(NULL != pcMesh->mBitangents);

	for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS;++i)
		CPPUNIT_ASSERT(NULL != pcMesh->mTextureCoords[i]);
}