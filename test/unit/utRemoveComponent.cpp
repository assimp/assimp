
#include "UnitTestPCH.h"
#include "utRemoveComponent.h"


CPPUNIT_TEST_SUITE_REGISTRATION (RemoveVCProcessTest);

void RemoveVCProcessTest :: setUp (void)
{
	// construct the process
	piProcess = new RemoveVCProcess();
	pScene = new aiScene();

	// fill the scene ..
	pScene->mMeshes = new aiMesh*[pScene->mNumMeshes = 2];
	pScene->mMeshes[0] = new aiMesh();
	pScene->mMeshes[1] = new aiMesh();

	pScene->mMeshes[0]->mNumVertices = 120;
	pScene->mMeshes[0]->mVertices = new aiVector3D[120];
	pScene->mMeshes[0]->mNormals = new aiVector3D[120];
	pScene->mMeshes[0]->mTextureCoords[0] = new aiVector3D[120];
	pScene->mMeshes[0]->mTextureCoords[1] = new aiVector3D[120];
	pScene->mMeshes[0]->mTextureCoords[2] = new aiVector3D[120];
	pScene->mMeshes[0]->mTextureCoords[3] = new aiVector3D[120];

	pScene->mMeshes[1]->mNumVertices = 120;
	pScene->mMeshes[1]->mVertices = new aiVector3D[120];

	pScene->mAnimations    = new aiAnimation*[pScene->mNumAnimations = 2];
	pScene->mAnimations[0] = new aiAnimation();
	pScene->mAnimations[1] = new aiAnimation();

	pScene->mTextures = new aiTexture*[pScene->mNumTextures = 2];
	pScene->mTextures[0] = new aiTexture();
	pScene->mTextures[1] = new aiTexture();

	pScene->mMaterials    = new aiMaterial*[pScene->mNumMaterials = 2];
	pScene->mMaterials[0] = new aiMaterial();
	pScene->mMaterials[1] = new aiMaterial();

	pScene->mLights    = new aiLight*[pScene->mNumLights = 2];
	pScene->mLights[0] = new aiLight();
	pScene->mLights[1] = new aiLight();

	pScene->mCameras    = new aiCamera*[pScene->mNumCameras = 2];
	pScene->mCameras[0] = new aiCamera();
	pScene->mCameras[1] = new aiCamera();

	// COMPILE TEST: aiMaterial may no add any extra members,
	// so we don't need a virtual destructor
	char check[sizeof(aiMaterial) == sizeof(aiMaterial) ? 10 : -1];
	check[0] = 0; 
}

void RemoveVCProcessTest :: tearDown (void)
{
	delete pScene;
	delete piProcess;
}

void  RemoveVCProcessTest::testMeshRemove (void)
{
	piProcess->SetDeleteFlags(aiComponent_MESHES);
	piProcess->Execute(pScene);

	CPPUNIT_ASSERT(NULL == pScene->mMeshes && 0 == pScene->mNumMeshes);
	CPPUNIT_ASSERT(pScene->mFlags == AI_SCENE_FLAGS_INCOMPLETE);
}

void  RemoveVCProcessTest::testAnimRemove (void)
{
	piProcess->SetDeleteFlags(aiComponent_ANIMATIONS);
	piProcess->Execute(pScene);

	CPPUNIT_ASSERT(NULL == pScene->mAnimations && 0 == pScene->mNumAnimations);
	CPPUNIT_ASSERT(pScene->mFlags == 0);
}

void  RemoveVCProcessTest::testMaterialRemove (void)
{
	piProcess->SetDeleteFlags(aiComponent_MATERIALS);
	piProcess->Execute(pScene);

	// there should be one default material now ...
	CPPUNIT_ASSERT(1 == pScene->mNumMaterials   && 
		pScene->mMeshes[0]->mMaterialIndex == 0 &&
		pScene->mMeshes[1]->mMaterialIndex == 0);
	CPPUNIT_ASSERT(pScene->mFlags == 0);
}

void  RemoveVCProcessTest::testTextureRemove (void)
{
	piProcess->SetDeleteFlags(aiComponent_TEXTURES);
	piProcess->Execute(pScene);

	CPPUNIT_ASSERT(NULL == pScene->mTextures && 0 == pScene->mNumTextures);
	CPPUNIT_ASSERT(pScene->mFlags == 0);
}

void  RemoveVCProcessTest::testCameraRemove (void)
{
	piProcess->SetDeleteFlags(aiComponent_CAMERAS);
	piProcess->Execute(pScene);

	CPPUNIT_ASSERT(NULL == pScene->mCameras && 0 == pScene->mNumCameras);
	CPPUNIT_ASSERT(pScene->mFlags == 0);
}

void  RemoveVCProcessTest::testLightRemove (void)
{
	piProcess->SetDeleteFlags(aiComponent_LIGHTS);
	piProcess->Execute(pScene);

	CPPUNIT_ASSERT(NULL == pScene->mLights && 0 == pScene->mNumLights);
	CPPUNIT_ASSERT(pScene->mFlags == 0);
}

void  RemoveVCProcessTest::testMeshComponentsRemoveA (void)
{
	piProcess->SetDeleteFlags(aiComponent_TEXCOORDSn(1) | aiComponent_TEXCOORDSn(2) | aiComponent_TEXCOORDSn(3));
	piProcess->Execute(pScene);

	CPPUNIT_ASSERT(pScene->mMeshes[0]->mTextureCoords[0] &&
		!pScene->mMeshes[0]->mTextureCoords[1] &&
		!pScene->mMeshes[0]->mTextureCoords[2] &&
		!pScene->mMeshes[0]->mTextureCoords[3]);
	CPPUNIT_ASSERT(pScene->mFlags == 0);
}

void  RemoveVCProcessTest::testMeshComponentsRemoveB (void)
{
	piProcess->SetDeleteFlags(aiComponent_TEXCOORDSn(1) | aiComponent_NORMALS);
	piProcess->Execute(pScene);

	CPPUNIT_ASSERT(pScene->mMeshes[0]->mTextureCoords[0] &&
		pScene->mMeshes[0]->mTextureCoords[1]  &&
		pScene->mMeshes[0]->mTextureCoords[2]  &&     // shift forward ...
		!pScene->mMeshes[0]->mTextureCoords[3] &&
		!pScene->mMeshes[0]->mNormals);
	CPPUNIT_ASSERT(pScene->mFlags == 0);
}

void  RemoveVCProcessTest::testRemoveEverything (void)
{
	piProcess->SetDeleteFlags(aiComponent_LIGHTS | aiComponent_ANIMATIONS | 
		aiComponent_MATERIALS | aiComponent_MESHES | aiComponent_CAMERAS | aiComponent_TEXTURES);
	piProcess->Execute(pScene);
}
