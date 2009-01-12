
#include "UnitTestPCH.h"
#include "utRemoveComments.h"


CPPUNIT_TEST_SUITE_REGISTRATION (RemoveCommentsTest);

// ------------------------------------------------------------------------------------------------
void RemoveCommentsTest :: setUp (void)
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
void RemoveCommentsTest :: tearDown (void)
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
void RemoveCommentsTest :: testSingleLineComments (void)
{
	const char* szTest = "int i = 0; \n"
		"if (4 == //)\n"
		"\ttrue) { // do something here \n"
		"\t// hello ... and bye //\n";

	
	char* szTest2 = new char[::strlen(szTest)+1];
	::strcpy(szTest2,szTest);

	const char* szTestResult = "int i = 0; \n"
		"if (4 ==    \n"
		"\ttrue) {                      \n"
		"\t                       \n";

	CommentRemover::RemoveLineComments("//",szTest2,' ');
	CPPUNIT_ASSERT(0 == ::strcmp(szTest2,szTestResult));

	delete[] szTest2;
}

// ------------------------------------------------------------------------------------------------
void RemoveCommentsTest :: testMultiLineComments (void)
{
	char* szTest = 
		"/* comment to be removed */\n"
		"valid text /* \n "
		" comment across multiple lines */"
		" / * Incomplete comment */ /* /* multiple comments */ */";

	const char* szTestResult = 
		"                           \n"
		"valid text      "
		"                                 "
		" / * Incomplete comment */                            */";

	char* szTest2 = new char[::strlen(szTest)+1];
	::strcpy(szTest2,szTest);

	CommentRemover::RemoveMultiLineComments("/*","*/",szTest2,' ');
	CPPUNIT_ASSERT(0 == ::strcmp(szTest2,szTestResult));

	delete[] szTest2;
}