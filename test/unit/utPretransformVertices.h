#ifndef TESTLBW_H
#define TESTLBW_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <assimp/scene.h>
#include <PretransformVertices.h>


using namespace std;
using namespace Assimp;

class PretransformVerticesTest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (PretransformVerticesTest);
    CPPUNIT_TEST (testProcess_CollapseHierarchy);
	CPPUNIT_TEST (testProcess_KeepHierarchy);
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp (void);
        void tearDown (void);

    protected:

        void  testProcess_CollapseHierarchy (void);
		void  testProcess_KeepHierarchy (void);
		
   
	private:

		aiScene* scene;
		PretransformVertices* process;
};

#endif 
