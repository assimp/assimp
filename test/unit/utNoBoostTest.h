#ifndef TESTNOBOOST_H
#define TESTNOBOOST_H

namespace noboost {

#define ASSIMP_FORCE_NOBOOST
#include "..\..\code\BoostWorkaround\boost\format.hpp"
	using boost::format;
	using boost::str;

}

using namespace std;
using namespace Assimp;

class NoBoostTest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (NoBoostTest);
	CPPUNIT_TEST (testFormat);
    CPPUNIT_TEST_SUITE_END ();

    public:
		void setUp (void) {
		}
		void tearDown (void) {
		}

    protected:

        void  testFormat (void);
   
	private:

	
};

#endif 