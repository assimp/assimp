#ifndef REMOVECOMMENTS_H
#define REMOVECOMMENTS_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <RemoveComments.h>


using namespace std;
using namespace Assimp;

class RemoveCommentsTest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (RemoveCommentsTest);
    CPPUNIT_TEST (testSingleLineComments);
	CPPUNIT_TEST (testMultiLineComments);
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp (void);
        void tearDown (void);

    protected:

        void testSingleLineComments (void);
		void testMultiLineComments (void);
		
   
	private:

};

#endif 
