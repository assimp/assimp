#ifndef TESTLBW_H
#define TESTLBW_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <assimp/scene.h>
#include <RemoveVCProcess.h>
#include <MaterialSystem.h>

using namespace std;
using namespace Assimp;

class RemoveVCProcessTest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (RemoveVCProcessTest);
    CPPUNIT_TEST (testMeshRemove);
	CPPUNIT_TEST (testAnimRemove);
	CPPUNIT_TEST (testMaterialRemove);
	CPPUNIT_TEST (testTextureRemove);
	CPPUNIT_TEST (testCameraRemove);
	CPPUNIT_TEST (testLightRemove);
	CPPUNIT_TEST (testMeshComponentsRemoveA);
	CPPUNIT_TEST (testMeshComponentsRemoveB);
	CPPUNIT_TEST (testRemoveEverything);
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp (void);
        void tearDown (void);

    protected:

        void  testMeshRemove (void);
		void  testAnimRemove (void);
		void  testMaterialRemove (void);
		void  testTextureRemove (void);
		void  testCameraRemove (void);
		void  testLightRemove (void);

		void  testMeshComponentsRemoveA (void);
		void  testMeshComponentsRemoveB (void);
		void  testRemoveEverything (void);

	private:

		RemoveVCProcess* piProcess;
		aiScene* pScene;
};

#endif 
