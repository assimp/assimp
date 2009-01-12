#ifndef TESTNORMALS_H
#define TESTNORMALS_H


#include <FindInvalidDataProcess.h>

using namespace std;
using namespace Assimp;

class FindInvalidDataProcessTest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (FindInvalidDataProcessTest);
	CPPUNIT_TEST (testStepNegativeResult);
	CPPUNIT_TEST (testStepPositiveResult);
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp (void);
        void tearDown (void);

    protected:

        void  testStepPositiveResult (void);
		void  testStepNegativeResult (void);
   
	private:

		
		aiMesh* pcMesh;
		FindInvalidDataProcess* piProcess;
};

#endif 
