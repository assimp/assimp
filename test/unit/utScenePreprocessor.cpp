
#include "UnitTestPCH.h"
#include "utScenePreprocessor.h"
CPPUNIT_TEST_SUITE_REGISTRATION (ScenePreprocessorTest);

// ------------------------------------------------------------------------------------------------
void ScenePreprocessorTest::setUp (void)
{
	// setup a dummy scene with a single node
	scene = new aiScene();
	scene->mRootNode = new aiNode();
	scene->mRootNode->mName.Set("<test>");

	// add some translation
	scene->mRootNode->mTransformation.a4 = 1.f;
	scene->mRootNode->mTransformation.b4 = 2.f;
	scene->mRootNode->mTransformation.c4 = 3.f;

	// and allocate a ScenePreprocessor to operate on the scene
	pp = new ScenePreprocessor(scene);
}

// ------------------------------------------------------------------------------------------------
void ScenePreprocessorTest::tearDown (void)
{
	delete pp;
	delete scene;
}

// ------------------------------------------------------------------------------------------------
// Check whether ProcessMesh() returns flag for a mesh that consist of primitives with num indices
void ScenePreprocessorTest::CheckIfOnly(aiMesh* p,unsigned int num, unsigned int flag)
{
	// Triangles only
	for (unsigned i = 0; i < p->mNumFaces;++i)	{
		p->mFaces[i].mNumIndices = num;
	}
	pp->ProcessMesh(p);
	CPPUNIT_ASSERT(p->mPrimitiveTypes == flag);
	p->mPrimitiveTypes = 0;
}

// ------------------------------------------------------------------------------------------------
// Check whether a mesh is preprocessed correctly. Case 1: The mesh needs preprocessing
void  ScenePreprocessorTest::testMeshPreprocessingPos (void)
{
	aiMesh* p = new aiMesh();
	p->mNumFaces = 100;
	p->mFaces = new aiFace[p->mNumFaces];
	
	p->mTextureCoords[0] = new aiVector3D[10];
	p->mNumUVComponents[0] = 0;
	p->mNumUVComponents[1] = 0;
	
	CheckIfOnly(p,1,aiPrimitiveType_POINT);
	CheckIfOnly(p,2,aiPrimitiveType_LINE);
	CheckIfOnly(p,3,aiPrimitiveType_TRIANGLE);
	CheckIfOnly(p,4,aiPrimitiveType_POLYGON);
	CheckIfOnly(p,1249,aiPrimitiveType_POLYGON);

	// Polygons and triangles mixed
	unsigned i;
	for (i = 0; i < p->mNumFaces/2;++i)	{
		p->mFaces[i].mNumIndices = 3;
	}
	for (; i < p->mNumFaces-p->mNumFaces/4;++i)	{
		p->mFaces[i].mNumIndices = 4;
	}
	for (; i < p->mNumFaces;++i)	{
		p->mFaces[i].mNumIndices = 10;
	}
	pp->ProcessMesh(p);
	CPPUNIT_ASSERT(p->mPrimitiveTypes == (aiPrimitiveType_TRIANGLE|aiPrimitiveType_POLYGON));
	CPPUNIT_ASSERT(p->mNumUVComponents[0] == 2);
	CPPUNIT_ASSERT(p->mNumUVComponents[1] == 0);
	delete p;
}

// ------------------------------------------------------------------------------------------------
// Check whether a mesh is preprocessed correctly. Case 1: The mesh doesn't need preprocessing
void  ScenePreprocessorTest::testMeshPreprocessingNeg (void)
{
	aiMesh* p = new aiMesh();
	p->mPrimitiveTypes = aiPrimitiveType_TRIANGLE|aiPrimitiveType_POLYGON;
	pp->ProcessMesh(p);

	// should be unmodified
	CPPUNIT_ASSERT(p->mPrimitiveTypes == (aiPrimitiveType_TRIANGLE|aiPrimitiveType_POLYGON));

	delete p;
}

// ------------------------------------------------------------------------------------------------
// Make a dummy animation with a single channel, '<test>'
aiAnimation* MakeDummyAnimation()	
{
	aiAnimation* p =  new aiAnimation();
	p->mNumChannels = 1;
	p->mChannels = new aiNodeAnim*[1];
	aiNodeAnim* anim = p->mChannels[0] = new aiNodeAnim();
	anim->mNodeName.Set("<test>");
	return p;
}

// ------------------------------------------------------------------------------------------------
// Check whether an anim is preprocessed correctly. Case 1: The anim needs preprocessing
void  ScenePreprocessorTest::testAnimationPreprocessingPos (void)
{
	aiAnimation* p = MakeDummyAnimation();
	aiNodeAnim* anim = p->mChannels[0];

	// we don't set the animation duration, but generate scaling channels
	anim->mNumScalingKeys = 10;
	anim->mScalingKeys = new aiVectorKey[10];

	for (unsigned int i = 0; i < 10;++i)	{
		anim->mScalingKeys[i].mTime = i;
		anim->mScalingKeys[i].mValue = aiVector3D((float)i);
	}
	pp->ProcessAnimation(p);

	// we should now have a proper duration
	CPPUNIT_ASSERT_DOUBLES_EQUAL(p->mDuration,9.,0.005);

	// ... one scaling key
	CPPUNIT_ASSERT(anim->mNumPositionKeys == 1 && 
		anim->mPositionKeys && 
		anim->mPositionKeys[0].mTime == 0.0 && 
		anim->mPositionKeys[0].mValue == aiVector3D(1.f,2.f,3.f));

	// ... and one rotation key
	CPPUNIT_ASSERT(anim->mNumRotationKeys == 1 && anim->mRotationKeys && 
		anim->mRotationKeys[0].mTime == 0.0); 

	delete p;
}

