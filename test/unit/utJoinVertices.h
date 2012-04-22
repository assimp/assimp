#ifndef TESTLBW_H
#define TESTLBW_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <assimp/scene.h>
#include <JoinVerticesProcess.h>


using namespace std;
using namespace Assimp;
namespace Assimp {
class JoinVerticesTest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (JoinVerticesTest);
    CPPUNIT_TEST (testProcess);
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp (void);
        void tearDown (void);

    protected:

        void  testProcess (void);
		
   
	private:

		JoinVerticesProcess* piProcess;
		aiMesh* pcMesh;

};
}
#endif 
