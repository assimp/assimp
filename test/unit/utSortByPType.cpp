
#include "UnitTestPCH.h"
#include "utSortByPType.h"

#include "utScenePreprocessor.h"

CPPUNIT_TEST_SUITE_REGISTRATION (SortByPTypeProcessTest);

// ------------------------------------------------------------------------------------------------
static unsigned int num[10][4] = 
	{
		{0,0,0,1000},
		{0,0,1000,0},
		{0,1000,0,0},
		{1000,0,0,0},
		{500,500,0,0},
		{500,0,500,0},
		{0,330,330,340},
		{250,250,250,250},
		{100,100,100,700},
		{0,100,0,900},
	};

// ------------------------------------------------------------------------------------------------
static unsigned int result[10] = 
{
	aiPrimitiveType_POLYGON,
	aiPrimitiveType_TRIANGLE,
	aiPrimitiveType_LINE,
	aiPrimitiveType_POINT,
	aiPrimitiveType_POINT | aiPrimitiveType_LINE,
	aiPrimitiveType_POINT | aiPrimitiveType_TRIANGLE,
	aiPrimitiveType_TRIANGLE | aiPrimitiveType_LINE | aiPrimitiveType_POLYGON,
	aiPrimitiveType_POLYGON  | aiPrimitiveType_LINE | aiPrimitiveType_TRIANGLE | aiPrimitiveType_POINT,
	aiPrimitiveType_POLYGON  | aiPrimitiveType_LINE | aiPrimitiveType_TRIANGLE | aiPrimitiveType_POINT,
	aiPrimitiveType_LINE | aiPrimitiveType_POLYGON,
};

// ------------------------------------------------------------------------------------------------
void SortByPTypeProcessTest :: setUp (void)
{
//	process0 = new DeterminePTypeHelperProcess();
	process1 = new SortByPTypeProcess();
	scene = new aiScene();

	scene->mNumMeshes = 10;
	scene->mMeshes = new aiMesh*[10];

	bool five = false;
	for (unsigned int i = 0; i < 10; ++i)
	{
		aiMesh* mesh = scene->mMeshes[i] = new aiMesh();
		mesh->mNumFaces = 1000;
		aiFace* faces =  mesh->mFaces = new aiFace[1000];
		aiVector3D* pv = mesh->mVertices = new aiVector3D[mesh->mNumFaces*5];
		aiVector3D* pn = mesh->mNormals = new aiVector3D[mesh->mNumFaces*5];

		aiVector3D* pt = mesh->mTangents = new aiVector3D[mesh->mNumFaces*5];
		aiVector3D* pb = mesh->mBitangents = new aiVector3D[mesh->mNumFaces*5];

		aiVector3D* puv = mesh->mTextureCoords[0] = new aiVector3D[mesh->mNumFaces*5];

		unsigned int remaining[4] = {num[i][0],num[i][1],num[i][2],num[i][3]};
		unsigned int n = 0;
		for (unsigned int m = 0; m < 1000; ++m)
		{
			unsigned int idx = m % 4;
			while (true)
			{
				if (!remaining[idx])
				{
					if (4 == ++idx)idx = 0;
					continue;
				}
				break;
			}
			faces->mNumIndices = idx+1;
			if (4 == faces->mNumIndices)
			{
				if(five)++faces->mNumIndices;
				five = !five;
			}
			faces->mIndices = new unsigned int[faces->mNumIndices];
			for (unsigned int q = 0; q <faces->mNumIndices;++q,++n)
			{
				faces->mIndices[q] = n;
				float f = (float)remaining[idx];

				// (the values need to be unique - otherwise all degenerates would be removed)
				*pv++ = aiVector3D(f,f+1.f,f+q);
				*pn++ = aiVector3D(f,f+1.f,f+q);
				*pt++ = aiVector3D(f,f+1.f,f+q);
				*pb++ = aiVector3D(f,f+1.f,f+q);
				*puv++ = aiVector3D(f,f+1.f,f+q);
			}
			++faces;
			--remaining[idx];
		}
		mesh->mNumVertices = n;
	}

	scene->mRootNode = new aiNode();
	scene->mRootNode->mNumChildren = 5;
	scene->mRootNode->mChildren = new aiNode*[5];
	for (unsigned int i = 0; i< 5;++i )
	{
		aiNode* node = scene->mRootNode->mChildren[i] = new aiNode();
		node->mNumMeshes = 2;
		node->mMeshes = new unsigned int[2];
		node->mMeshes[0] = (i<<1u);
		node->mMeshes[1] = (i<<1u)+1;
	}
}

// ------------------------------------------------------------------------------------------------
void SortByPTypeProcessTest :: tearDown (void)
{
	//delete process0;
	delete process1;
	delete scene;
}

// ------------------------------------------------------------------------------------------------
//void  SortByPTypeProcessTest :: testDeterminePTypeStep (void)
//{
//	process0->Execute(scene);
//
//	for (unsigned int i = 0; i < 10; ++i)
//	{
//		aiMesh* mesh = scene->mMeshes[i];
//		CPPUNIT_ASSERT(mesh->mPrimitiveTypes == result[i]);
//	}
//}

// ------------------------------------------------------------------------------------------------
void  SortByPTypeProcessTest :: testSortByPTypeStep (void)
{
//	process0->Execute(scene);

	// and another small test for ScenePreprocessor
	ScenePreprocessor s(scene);
	s.ProcessScene();
	for (unsigned int m = 0; m< 10;++m)
		CPPUNIT_ASSERT(scene->mMeshes[m]->mPrimitiveTypes == result[m]);

	process1->Execute(scene);

	unsigned int idx = 0;
	for (unsigned int m = 0,real = 0; m< 10;++m)
	{
		for (unsigned int n = 0; n < 4;++n)
		{
			if ((idx = num[m][n]))
			{
				CPPUNIT_ASSERT(real < scene->mNumMeshes);

				aiMesh* mesh = scene->mMeshes[real];

				CPPUNIT_ASSERT(NULL != mesh);
				CPPUNIT_ASSERT(mesh->mPrimitiveTypes == AI_PRIMITIVE_TYPE_FOR_N_INDICES(n+1));
				CPPUNIT_ASSERT(NULL != mesh->mVertices);
				CPPUNIT_ASSERT(NULL != mesh->mNormals);
				CPPUNIT_ASSERT(NULL != mesh->mTangents);
				CPPUNIT_ASSERT(NULL != mesh->mBitangents);
				CPPUNIT_ASSERT(NULL != mesh->mTextureCoords[0]);

				CPPUNIT_ASSERT(mesh->mNumFaces == idx);
				for (unsigned int f = 0; f < mesh->mNumFaces;++f)
				{
					aiFace& face = mesh->mFaces[f];
					CPPUNIT_ASSERT(face.mNumIndices == (n+1) || (3 == n && face.mNumIndices > 3));
				}
				++real;
			}
		}
	}
}

