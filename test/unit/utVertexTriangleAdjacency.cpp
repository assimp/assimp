
#include "UnitTestPCH.h"
#include "utVertexTriangleAdjacency.h"


CPPUNIT_TEST_SUITE_REGISTRATION (VTAdjacency);

// ------------------------------------------------------------------------------------------------
void VTAdjacency :: setUp (void)
{
	// build a test mesh with randomized input data
	// *******************************************************************************
	pMesh = new aiMesh();

	pMesh->mNumVertices = 500;
	pMesh->mNumFaces = 600;

	pMesh->mFaces = new aiFace[600];
	unsigned int iCurrent = 0;
	for (unsigned int i = 0; i < 600;++i)
	{
		aiFace& face = pMesh->mFaces[i];
		face.mNumIndices = 3;
		face.mIndices = new unsigned int[3];

		if (499 == iCurrent)iCurrent = 0;
		face.mIndices[0] = iCurrent++;


		while(face.mIndices[0] == ( face.mIndices[1] = (unsigned int)(((float)rand()/RAND_MAX)*499)));
		while(face.mIndices[0] == ( face.mIndices[2] = (unsigned int)(((float)rand()/RAND_MAX)*499)) ||
			face.mIndices[1] == face.mIndices[2]);
	}


	// build a second test mesh - this one is extremely small
	// *******************************************************************************
	pMesh2 = new aiMesh();

	pMesh2->mNumVertices = 5;
	pMesh2->mNumFaces = 3;

	pMesh2->mFaces = new aiFace[3];
	pMesh2->mFaces[0].mIndices = new unsigned int[3];
	pMesh2->mFaces[1].mIndices = new unsigned int[3];
	pMesh2->mFaces[2].mIndices = new unsigned int[3];

	pMesh2->mFaces[0].mIndices[0] = 1;
	pMesh2->mFaces[0].mIndices[1] = 3;
	pMesh2->mFaces[0].mIndices[2] = 2;

	pMesh2->mFaces[1].mIndices[0] = 0;
	pMesh2->mFaces[1].mIndices[1] = 2;
	pMesh2->mFaces[1].mIndices[2] = 3;

	pMesh2->mFaces[2].mIndices[0] = 3;
	pMesh2->mFaces[2].mIndices[1] = 0;
	pMesh2->mFaces[2].mIndices[2] = 4;


	// build a third test mesh which does not reference all vertices
	// *******************************************************************************
	pMesh3 = new aiMesh();

	pMesh3->mNumVertices = 500;
	pMesh3->mNumFaces = 600;

	pMesh3->mFaces = new aiFace[600];
	iCurrent = 0;
	for (unsigned int i = 0; i < 600;++i)
	{
		aiFace& face = pMesh3->mFaces[i];
		face.mNumIndices = 3;
		face.mIndices = new unsigned int[3];

		if (499 == iCurrent)iCurrent = 0;
		face.mIndices[0] = iCurrent++;

		if (499 == iCurrent)iCurrent = 0;
		face.mIndices[1] = iCurrent++;

		if (499 == iCurrent)iCurrent = 0;
		face.mIndices[2] = iCurrent++;

		if (rand() > RAND_MAX/2 && face.mIndices[0])
		{
			face.mIndices[0]--;
		}
		else if (face.mIndices[1]) face.mIndices[1]--;
	}
}

// ------------------------------------------------------------------------------------------------
void VTAdjacency :: tearDown (void) 
{
	delete pMesh;
	pMesh = 0;

	delete pMesh2;
	pMesh2 = 0;

	delete pMesh3;
	pMesh3 = 0;
}

// ------------------------------------------------------------------------------------------------
void VTAdjacency :: largeRandomDataSet (void)
{
	checkMesh(pMesh);
}

// ------------------------------------------------------------------------------------------------
void VTAdjacency :: smallDataSet (void)
{
	checkMesh(pMesh2);
}

// ------------------------------------------------------------------------------------------------
void VTAdjacency :: unreferencedVerticesSet (void)
{
	checkMesh(pMesh3);
}

// ------------------------------------------------------------------------------------------------
void VTAdjacency :: checkMesh (aiMesh* pMesh)
{
	pAdj = new VertexTriangleAdjacency(pMesh->mFaces,pMesh->mNumFaces,pMesh->mNumVertices,true);

	
	unsigned int* const piNum = pAdj->mLiveTriangles;

	// check the primary adjacency table and check whether all faces
	// are contained in the list
	unsigned int maxOfs = 0;
	for (unsigned int i = 0; i < pMesh->mNumFaces;++i)
	{
		aiFace& face = pMesh->mFaces[i];
		for (unsigned int qq = 0; qq < 3 ;++qq)
		{
			const unsigned int idx = face.mIndices[qq];
			const unsigned int num = piNum[idx];

			// go to this offset
			const unsigned int ofs = pAdj->mOffsetTable[idx];
			maxOfs = std::max(ofs+num,maxOfs);
			unsigned int* pi = &pAdj->mAdjacencyTable[ofs];

			// and search for us ...
			unsigned int tt = 0;
			for (; tt < num;++tt,++pi)
			{
				if (i == *pi)
				{
					// mask our entry in the table. Finally all entries should be masked
					*pi = 0xffffffff;

					// there shouldn't be two entries for the same face
					break;
				}
			}
			// assert if *this* vertex has not been found in the table
			CPPUNIT_ASSERT(tt < num);
		}
	}

	// now check whether there are invalid faces
	const unsigned int* pi = pAdj->mAdjacencyTable;
	for (unsigned int i = 0; i < maxOfs;++i,++pi)
	{
		CPPUNIT_ASSERT(0xffffffff == *pi);
	}

	// check the numTrianglesPerVertex table
	for (unsigned int i = 0; i < pMesh->mNumFaces;++i)
	{
		aiFace& face = pMesh->mFaces[i];
		for (unsigned int qq = 0; qq < 3 ;++qq)
		{
			const unsigned int idx = face.mIndices[qq];

			// we should not reach 0 here ...
			CPPUNIT_ASSERT( 0 != piNum[idx]);
			piNum[idx]--;
		}
	}

	// check whether we reached 0 in all entries
	for (unsigned int i = 0; i < pMesh->mNumVertices;++i)
	{
		CPPUNIT_ASSERT(!piNum[i]);
	}
	delete pAdj;
}
