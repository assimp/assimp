#ifndef TESTNORMALS_H
#define TESTNORMALS_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <assimp/scene.h>
#include <GenVertexNormalsProcess.h>


using namespace std;
using namespace Assimp;

class GenNormalsTest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (GenNormalsTest);
	CPPUNIT_TEST (testSimpleTriangle);
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp (void);
        void tearDown (void);

    protected:

        void  testSimpleTriangle (void);
   
	private:

		
		aiMesh* pcMesh;
		GenVertexNormalsProcess* piProcess;
};

#endif 
