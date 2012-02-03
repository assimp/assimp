
#include "UnitTestPCH.h"
#include "utJoinVertices.h"

CPPUNIT_TEST_SUITE_REGISTRATION (JoinVerticesTest);

// ------------------------------------------------------------------------------------------------
void JoinVerticesTest :: setUp (void)
{
	// construct the process
	piProcess = new JoinVerticesProcess();

	// create a quite small mesh for testing purposes -
	// the mesh itself is *something* but it has redundant vertices
	pcMesh = new aiMesh();

	pcMesh->mNumVertices = 900;
	aiVector3D*& pv = pcMesh->mVertices = new aiVector3D[900];
	for (unsigned int i = 0; i < 3;++i)
	{
		const unsigned int base = i*300;
		for (unsigned int a = 0; a < 300;++a)
		{
			pv[base+a].x = pv[base+a].y = pv[base+a].z = (float)a;
		}
	}

	// generate faces - each vertex is referenced once
	pcMesh->mNumFaces = 300;
	pcMesh->mFaces = new aiFace[300];
	for (unsigned int i = 0,p = 0; i < 300;++i)
	{
		aiFace& face = pcMesh->mFaces[i];
		face.mIndices = new unsigned int[ face.mNumIndices = 3 ];
		for (unsigned int a = 0; a < 3;++a)
			face.mIndices[a] = p++;
	}

	// generate extra members - set them to zero to make sure they're identical
	pcMesh->mTextureCoords[0] = new aiVector3D[900];
	for (unsigned int i = 0; i < 900;++i)pcMesh->mTextureCoords[0][i] = aiVector3D( 0.f ); 

	pcMesh->mNormals = new aiVector3D[900];
	for (unsigned int i = 0; i < 900;++i)pcMesh->mNormals[i] = aiVector3D( 0.f ); 

	pcMesh->mTangents = new aiVector3D[900];
	for (unsigned int i = 0; i < 900;++i)pcMesh->mTangents[i] = aiVector3D( 0.f ); 

	pcMesh->mBitangents = new aiVector3D[900];
	for (unsigned int i = 0; i < 900;++i)pcMesh->mBitangents[i] = aiVector3D( 0.f ); 
}

// ------------------------------------------------------------------------------------------------
void JoinVerticesTest :: tearDown (void)
{
	delete this->pcMesh;
	delete this->piProcess;
}

// ------------------------------------------------------------------------------------------------
void JoinVerticesTest :: testProcess(void)
{
	// execute the step on the given data
	piProcess->ProcessMesh(pcMesh,0);

	// the number of faces shouldn't change
	CPPUNIT_ASSERT(pcMesh->mNumFaces == 300);
	CPPUNIT_ASSERT(pcMesh->mNumVertices == 300);

	CPPUNIT_ASSERT(NULL != pcMesh->mNormals);
	CPPUNIT_ASSERT(NULL != pcMesh->mTangents);
	CPPUNIT_ASSERT(NULL != pcMesh->mBitangents);
	CPPUNIT_ASSERT(NULL != pcMesh->mTextureCoords[0]);

	// the order doesn't care
	float fSum = 0.f;
	for (unsigned int i = 0; i < 300;++i)
	{
		aiVector3D& v = pcMesh->mVertices[i];
		fSum += v.x + v.y + v.z;

		CPPUNIT_ASSERT(!pcMesh->mNormals[i].x);
		CPPUNIT_ASSERT(!pcMesh->mTangents[i].x);
		CPPUNIT_ASSERT(!pcMesh->mBitangents[i].x);
		CPPUNIT_ASSERT(!pcMesh->mTextureCoords[0][i].x);
	}
	CPPUNIT_ASSERT(fSum == 150.f*299.f*3.f); // gaussian sum equation
}

