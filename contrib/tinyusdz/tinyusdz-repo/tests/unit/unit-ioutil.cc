#ifdef _MSC_VER
#define NOMINMAX
#endif

#define TEST_NO_MAIN
#include "acutest.h"

#include "unit-ioutil.h"
#include "io-util.hh"

using namespace tinyusdz;

void ioutil_test(void) {
  {
    TEST_CHECK(io::JoinPath("./", "./dora") == "./dora");
  }
}
