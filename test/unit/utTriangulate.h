#ifndef TESTNORMALS_H
#define TESTNORMALS_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <aiTypes.h>
#include <aiMesh.h>
#include <aiScene.h>
#include <TriangulateProcess.h>


using namespace std;
using namespace Assimp;

class TriangulateProcessTest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (TriangulateProcessTest);
	CPPUNIT_TEST (testTriangulation);
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp (void);
        void tearDown (void);

    protected:

        void  testTriangulation (void);
   
	private:

		
		aiMesh* pcMesh;
		TriangulateProcess* piProcess;
};

#endif 
