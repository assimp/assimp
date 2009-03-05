#ifndef TESTDEGENERATES_H
#define TESTDEGENERATES_H


#include <FindDegenerates.h>

using namespace std;
using namespace Assimp;

class FindDegeneratesProcessTest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (FindDegeneratesProcessTest);
	CPPUNIT_TEST (testDegeneratesDetection);
	CPPUNIT_TEST (testDegeneratesRemoval);
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp (void);
        void tearDown (void);

    protected:

        void  testDegeneratesDetection (void);
		void  testDegeneratesRemoval (void);
   
	private:

		aiMesh* mesh;
		FindDegeneratesProcess* process;
	
};

#endif 
