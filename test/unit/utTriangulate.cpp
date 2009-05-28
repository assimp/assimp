
#include "UnitTestPCH.h"
#include "utTriangulate.h"


CPPUNIT_TEST_SUITE_REGISTRATION (TriangulateProcessTest);


void TriangulateProcessTest :: setUp (void)
{
	piProcess = new TriangulateProcess();
	pcMesh = new aiMesh();

	pcMesh->mNumFaces = 1000;
	pcMesh->mFaces = new aiFace[1000];
	pcMesh->mVertices = new aiVector3D[10000];

	pcMesh->mPrimitiveTypes = aiPrimitiveType_POINT | aiPrimitiveType_LINE | 
		aiPrimitiveType_LINE | aiPrimitiveType_POLYGON;

	for (unsigned int m = 0, t = 0, q = 4; m < 1000; ++m)
	{
		++t;
		aiFace& face = pcMesh->mFaces[m];
		face.mNumIndices = t;
		if (4 == t)
		{
			face.mNumIndices = q++;
			t = 0;

			if (10 == q)q = 4;
		}
		face.mIndices = new unsigned int[face.mNumIndices];
		for (unsigned int p = 0; p < face.mNumIndices; ++p)
		{
			face.mIndices[p] = pcMesh->mNumVertices;

			// construct fully convex input data in ccw winding, xy plane
			aiVector3D& v = pcMesh->mVertices[pcMesh->mNumVertices++];
			v.z = 0.f;
			v.x = cos (p * (float)(AI_MATH_TWO_PI)/face.mNumIndices);
			v.y = sin (p * (float)(AI_MATH_TWO_PI)/face.mNumIndices);
		}
	}
}

void TriangulateProcessTest :: tearDown (void)
{
	delete piProcess;
	delete pcMesh;
}

void  TriangulateProcessTest :: testTriangulation (void)
{
	piProcess->TriangulateMesh(pcMesh);

	for (unsigned int m = 0, t = 0, q = 4, max = 1000, idx = 0; m < max;++m)
	{
		++t;
		aiFace& face = pcMesh->mFaces[m];
		if (4 == t)
		{
			t = 0;
			max += q-3;

			std::vector<bool> ait(q,false);

			for (unsigned int i = 0, tt = q-2; i < tt; ++i,++m)
			{
				aiFace& face = pcMesh->mFaces[m];
				CPPUNIT_ASSERT(face.mNumIndices == 3);

				for (unsigned int qqq = 0; qqq < face.mNumIndices; ++qqq)
				{
					ait[face.mIndices[qqq]-idx] = true;
				}
			}
			for (std::vector<bool>::const_iterator it = ait.begin(); it != ait.end(); ++it)
			{
				CPPUNIT_ASSERT(*it);
			}
			--m;
			idx+=q;
			if(++q == 10)q = 4;
		}
		else
		{
			CPPUNIT_ASSERT(face.mNumIndices == t);

			for (unsigned int i = 0; i < face.mNumIndices; ++i,++idx)
			{
				CPPUNIT_ASSERT(face.mIndices[i] == idx);
			}
		}
	}

	// we should have no valid normal vectors now necause we aren't a pure polygon mesh
	CPPUNIT_ASSERT(pcMesh->mNormals == NULL);
}