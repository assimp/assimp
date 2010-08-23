
#include "UnitTestPCH.h"
#include "utNoBoostTest.h"


CPPUNIT_TEST_SUITE_REGISTRATION (NoBoostTest);

// ------------------------------------------------------------------------------------------------
void NoBoostTest :: testFormat()
{

	CPPUNIT_ASSERT( noboost::str( noboost::format("Ahoi!") ) == "Ahoi!" );
	CPPUNIT_ASSERT( noboost::str( noboost::format("Ahoi! %%") ) == "Ahoi! %" );
	CPPUNIT_ASSERT( noboost::str( noboost::format("Ahoi! %s") ) == "Ahoi! " );
	CPPUNIT_ASSERT( noboost::str( noboost::format("Ahoi! %s") % "!!" ) == "Ahoi! !!" );
	CPPUNIT_ASSERT( noboost::str( noboost::format("Ahoi! %s") % "!!" % "!!" ) == "Ahoi! !!" );
	CPPUNIT_ASSERT( noboost::str( noboost::format("%s%s%s") % "a" % std::string("b") % "c" ) == "abc" );
}
