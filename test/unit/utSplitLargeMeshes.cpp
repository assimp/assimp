
#include "UnitTestPCH.h"
#include "utSplitLargeMeshes.h"


CPPUNIT_TEST_SUITE_REGISTRATION (SplitLargeMeshesTest);

// ------------------------------------------------------------------------------------------------
void SplitLargeMeshesTest :: setUp (void)
{
	// construct the processes
	this->piProcessTriangle = new SplitLargeMeshesProcess_Triangle();
	this->piProcessVertex = new SplitLargeMeshesProcess_Vertex();

	this->piProcessTriangle->SetLimit(1000);
	this->piProcessVertex->SetLimit(1000);

	this->pcMesh1 = new aiMesh();
	pcMesh1->mNumVertices = 2100; // quersumme: 3
	pcMesh1->mVertices = new aiVector3D[pcMesh1->mNumVertices]; 
	pcMesh1->mNormals = new aiVector3D[pcMesh1->mNumVertices]; 

	pcMesh1->mNumFaces = pcMesh1->mNumVertices / 3;
	pcMesh1->mFaces = new aiFace[pcMesh1->mNumFaces];

	unsigned int qq = 0;
	for (unsigned int i = 0; i < pcMesh1->mNumFaces;++i)
	{
		aiFace& face = pcMesh1->mFaces[i];
		face.mNumIndices = 3;
		face.mIndices = new unsigned int[3];
		face.mIndices[0] = qq++;
		face.mIndices[1] = qq++;
		face.mIndices[2] = qq++;
	}

	// generate many, many faces with randomized indices for
	// the second mesh
	this->pcMesh2 = new aiMesh();
	pcMesh2->mNumVertices = 3000; 
	pcMesh2->mVertices = new aiVector3D[pcMesh2->mNumVertices]; 
	pcMesh2->mNormals = new aiVector3D[pcMesh2->mNumVertices]; 

	pcMesh2->mNumFaces = 10000;
	pcMesh2->mFaces = new aiFace[pcMesh2->mNumFaces];

	for (unsigned int i = 0; i < pcMesh2->mNumFaces;++i)
	{
		aiFace& face = pcMesh2->mFaces[i];
		face.mNumIndices = 3;
		face.mIndices = new unsigned int[3];
		face.mIndices[0] = (unsigned int)((rand() / (float)RAND_MAX) * pcMesh2->mNumVertices);
		face.mIndices[1] = (unsigned int)((rand() / (float)RAND_MAX) * pcMesh2->mNumVertices);
		face.mIndices[2] = (unsigned int)((rand() / (float)RAND_MAX) * pcMesh2->mNumVertices);
	}
}

// ------------------------------------------------------------------------------------------------
void SplitLargeMeshesTest :: tearDown (void)
{
	delete this->piProcessTriangle;
	delete this->piProcessVertex;
}

// ------------------------------------------------------------------------------------------------
void SplitLargeMeshesTest :: testVertexSplit()
{
	std::vector< std::pair<aiMesh*, unsigned int> > avOut;

	int iOldFaceNum = (int)pcMesh1->mNumFaces;
	piProcessVertex->SplitMesh(0,pcMesh1,avOut);

	for (std::vector< std::pair<aiMesh*, unsigned int> >::const_iterator
		iter =  avOut.begin(), end = avOut.end();
		iter != end; ++iter)
	{
		aiMesh* mesh = (*iter).first;
		CPPUNIT_ASSERT(mesh->mNumVertices < 1000);
		CPPUNIT_ASSERT(0 != mesh->mNormals && 0 != mesh->mVertices);

		iOldFaceNum -= mesh->mNumFaces;
		delete mesh;
	}
	CPPUNIT_ASSERT(0 == iOldFaceNum);
}

// ------------------------------------------------------------------------------------------------
void SplitLargeMeshesTest :: testTriangleSplit()
{
	std::vector< std::pair<aiMesh*, unsigned int> > avOut;

	// the number of faces shouldn't change
	int iOldFaceNum = (int)pcMesh2->mNumFaces;
	piProcessTriangle->SplitMesh(0,pcMesh2,avOut);

	for (std::vector< std::pair<aiMesh*, unsigned int> >::const_iterator
		iter =  avOut.begin(), end = avOut.end();
		iter != end; ++iter)
	{
		aiMesh* mesh = (*iter).first;
		CPPUNIT_ASSERT(mesh->mNumFaces < 1000);
		CPPUNIT_ASSERT(0 != mesh->mNormals && 0 != mesh->mVertices);

		iOldFaceNum -= mesh->mNumFaces;
		delete mesh;
	}
	CPPUNIT_ASSERT(0 == iOldFaceNum);
}