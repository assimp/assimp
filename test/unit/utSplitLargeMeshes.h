#ifndef TESTLM_H
#define TESTLM_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <assimp/scene.h>
#include <SplitLargeMeshes.h>


using namespace std;
using namespace Assimp;

class SplitLargeMeshesTest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (SplitLargeMeshesTest);
    CPPUNIT_TEST (testVertexSplit);
	CPPUNIT_TEST (testTriangleSplit);
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp (void);
        void tearDown (void);

    protected:

        void  testVertexSplit (void);
		void  testTriangleSplit (void);
		
   
	private:

		SplitLargeMeshesProcess_Triangle* piProcessTriangle;
		SplitLargeMeshesProcess_Vertex* piProcessVertex;
		aiMesh* pcMesh1;
		aiMesh* pcMesh2;
};

#endif 
