#ifndef TESTMATERIALS_H
#define TESTMATERIALS_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <assimp/scene.h>
#include <MaterialSystem.h>


using namespace std;
using namespace Assimp;

class MaterialSystemTest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (MaterialSystemTest);
	CPPUNIT_TEST (testFloatProperty);
	CPPUNIT_TEST (testFloatArrayProperty);
	CPPUNIT_TEST (testIntProperty);
	CPPUNIT_TEST (testIntArrayProperty);
	CPPUNIT_TEST (testColorProperty);
	CPPUNIT_TEST (testStringProperty);
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp (void);
        void tearDown (void);

    protected:

        void  testFloatProperty (void);
		void  testFloatArrayProperty (void);
		void  testIntProperty (void);
		void  testIntArrayProperty (void);
		void  testColorProperty (void);
		void  testStringProperty (void);
   
	private:

		aiMaterial* pcMat;
};

#endif 
