#ifndef TESTOG_H
#define TESTOG_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <aiScene.h>
#include <OptimizeGraphProcess.h>


using namespace std;
using namespace Assimp;

class OptimizeGraphProcessTest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (OptimizeGraphProcessTest);
    CPPUNIT_TEST (testProcess);
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp (void);
        void tearDown (void);

    protected:

        void  testProcess (void);
		
   
	private:

		OptimizeGraphProcess* piProcess;
		aiScene* pcMesh;
};

#endif 
