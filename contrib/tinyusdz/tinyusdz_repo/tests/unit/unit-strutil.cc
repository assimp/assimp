#ifdef _MSC_VER
#define NOMINMAX
#endif

#define TEST_NO_MAIN
#include "acutest.h"

#include "unit-strutil.h"
#include "str-util.hh"

using namespace tinyusdz;

void strutil_test(void) {
  {
    std::string s = "HelloA WorlZ";
    std::string ls = tinyusdz::to_lower(s);
    TEST_CHECK(ls.compare("helloa worlz") == 0);

  }
}
