#include "utRemoveComponent.h"


CPPUNIT_TEST_SUITE_REGISTRATION (RemoveVCProcessTest);

void RemoveVCProcessTest :: setUp (void)
{
	// construct the process
	piProcess = new RemoveVCProcess();
	pScene = new aiScene();
}

void RemoveVCProcessTest :: tearDown (void)
{
	delete pScene;
	delete piProcess;
}

void  RemoveVCProcessTest::testMeshRemove (void)
{
}

void  RemoveVCProcessTest::testAnimRemove (void)
{
}

void  RemoveVCProcessTest::testMaterialRemove (void)
{
}

void  RemoveVCProcessTest::testTextureRemove (void)
{
}

void  RemoveVCProcessTest::testCameraRemove (void)
{
}

void  RemoveVCProcessTest::testLightRemove (void)
{
}

void  RemoveVCProcessTest::testMeshComponentsRemoveA (void)
{
}

void  RemoveVCProcessTest::testMeshComponentsRemoveB (void)
{
}

void  RemoveVCProcessTest::testRemoveEverything (void)
{
}
