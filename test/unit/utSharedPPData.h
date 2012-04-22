#ifndef TESTPPD_H
#define TESTPPD_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <assimp/scene.h>
#include <BaseProcess.h>


using namespace std;
using namespace Assimp;

class SharedPPDataTest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (SharedPPDataTest);
    CPPUNIT_TEST (testPODProperty);
	CPPUNIT_TEST (testPropertyPointer);
	CPPUNIT_TEST (testPropertyDeallocation);
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp (void);
        void tearDown (void);

    protected:

        void  testPODProperty (void);
		void  testPropertyPointer (void);
		void  testPropertyDeallocation (void);
   
	private:

		SharedPostProcessInfo* shared;
	
};

#endif 
