
#include "UnitTestPCH.h"
#include "utGenNormals.h"


CPPUNIT_TEST_SUITE_REGISTRATION (GenNormalsTest);

void GenNormalsTest :: setUp (void)
{
	piProcess = new GenVertexNormalsProcess();
	pcMesh = new aiMesh();
	pcMesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
	pcMesh->mNumFaces = 1;
	pcMesh->mFaces = new aiFace[1];
	pcMesh->mFaces[0].mIndices = new unsigned int[pcMesh->mFaces[0].mNumIndices = 3];
	pcMesh->mFaces[0].mIndices[0] = 0;
	pcMesh->mFaces[0].mIndices[1] = 1;
	pcMesh->mFaces[0].mIndices[2] = 1;
	pcMesh->mNumVertices = 3;
	pcMesh->mVertices = new aiVector3D[3];
	pcMesh->mVertices[0] = aiVector3D(0.0f,1.0f,6.0f);
	pcMesh->mVertices[1] = aiVector3D(2.0f,3.0f,1.0f);
	pcMesh->mVertices[2] = aiVector3D(3.0f,2.0f,4.0f);
}

void GenNormalsTest :: tearDown (void)
{
	delete this->pcMesh;
	delete this->piProcess;
}

void  GenNormalsTest :: testSimpleTriangle (void)
{
	piProcess->GenMeshVertexNormals(pcMesh,0);
	CPPUNIT_ASSERT(0 != pcMesh->mNormals);
}
