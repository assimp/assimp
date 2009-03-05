
#include "UnitTestPCH.h"
#include "utFindDegenerates.h"


CPPUNIT_TEST_SUITE_REGISTRATION (FindDegeneratesProcessTest);

// ------------------------------------------------------------------------------------------------
void FindDegeneratesProcessTest :: setUp (void)
{
	mesh = new aiMesh();
	process = new FindDegeneratesProcess();

	mesh->mNumFaces = 1000;
	mesh->mFaces = new aiFace[1000];

	mesh->mNumVertices = 5000*2;
	mesh->mVertices = new aiVector3D[5000*2];

	for (unsigned int i = 0; i < 5000; ++i) {
		mesh->mVertices[i] = mesh->mVertices[i+5000] = aiVector3D((float)i);
	}

	mesh->mPrimitiveTypes = aiPrimitiveType_LINE | aiPrimitiveType_POINT |
		aiPrimitiveType_POLYGON | aiPrimitiveType_TRIANGLE;

	unsigned int numOut = 0, numFaces = 0;
	for (unsigned int i = 0; i < 1000; ++i) {
		aiFace& f = mesh->mFaces[i];
		f.mNumIndices = (i % 5)+1; // between 1 and 5
		f.mIndices = new unsigned int[f.mNumIndices];
		bool had = false;
		for (unsigned int n = 0; n < f.mNumIndices;++n) {

			// FIXME
#if 0
			// some duplicate indices
			if ( n && n == (i / 200)+1) {
				f.mIndices[n] = f.mIndices[n-1];
				had = true;
			}
			// and some duplicate vertices
#endif
			if (n && i % 2 && 0 == n % 2) {
				f.mIndices[n] = f.mIndices[n-1]+5000;
				had = true;
			}
			else {
				f.mIndices[n] = numOut++;
			}
		}
		if (!had)
			++numFaces;
	}
	mesh->mNumUVComponents[0] = numOut;
	mesh->mNumUVComponents[1] = numFaces;
}

// ------------------------------------------------------------------------------------------------
void FindDegeneratesProcessTest :: tearDown (void)
{
	delete mesh;
	delete process;
}

// ------------------------------------------------------------------------------------------------
void FindDegeneratesProcessTest :: testDegeneratesDetection( void )
{
	process->EnableInstantRemoval(false);
	process->ExecuteOnMesh(mesh);

	unsigned int out = 0;
	for (unsigned int i = 0; i < 1000; ++i) {
		aiFace& f = mesh->mFaces[i];
		out += f.mNumIndices;
	}

	CPPUNIT_ASSERT(mesh->mNumFaces == 1000 && mesh->mNumVertices == 10000);
	CPPUNIT_ASSERT(mesh->mNumUVComponents[0] == out);
	CPPUNIT_ASSERT(mesh->mPrimitiveTypes == (aiPrimitiveType_LINE | aiPrimitiveType_POINT |
		aiPrimitiveType_POLYGON | aiPrimitiveType_TRIANGLE));
}

// ------------------------------------------------------------------------------------------------
void FindDegeneratesProcessTest :: testDegeneratesRemoval( void )
{
	process->EnableInstantRemoval(true);
	process->ExecuteOnMesh(mesh);

	CPPUNIT_ASSERT(mesh->mNumUVComponents[1] == mesh->mNumFaces);
}

