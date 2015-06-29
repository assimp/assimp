#include "UnitTestPCH.h"

#include <RemoveComments.h>


using namespace std;
using namespace Assimp;


// ------------------------------------------------------------------------------------------------
TEST(RemoveCommentsTest, testSingleLineComments)
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
    EXPECT_STREQ(szTestResult, szTest2);

    delete[] szTest2;
}

// ------------------------------------------------------------------------------------------------
TEST(RemoveCommentsTest, testMultiLineComments)
{
    const char* szTest =
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
    EXPECT_STREQ(szTestResult, szTest2);

    delete[] szTest2;
}
