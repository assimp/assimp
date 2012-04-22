#ifndef TESTRRM_H
#define TESTRRM_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <assimp/scene.h>
#include <RemoveRedundantMaterials.h>
#include <MaterialSystem.h>

using namespace std;
using namespace Assimp;

class RemoveRedundantMatsTest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (RemoveRedundantMatsTest);
    CPPUNIT_TEST (testRedundantMaterials);
	CPPUNIT_TEST (testRedundantMaterialsWithExcludeList);
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp (void);
        void tearDown (void);

    protected:

        void  testRedundantMaterials (void);
		void  testRedundantMaterialsWithExcludeList (void);
   
	private:

		RemoveRedundantMatsProcess* piProcess;
		
		aiScene* pcScene1;
		aiScene* pcScene2;
};

#endif 
