#ifndef TESTPPD_H
#define TESTPPD_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <assimp/scene.h>
#include <SortByPTypeProcess.h>


using namespace std;
using namespace Assimp;

class SortByPTypeProcessTest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (SortByPTypeProcessTest);
	CPPUNIT_TEST (testSortByPTypeStep);
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp (void);
        void tearDown (void);

    protected:

        void  testDeterminePTypeStep (void);
		void  testSortByPTypeStep (void);
   
	private:

		SortByPTypeProcess* process1;
		aiScene* scene;
};

#endif 
