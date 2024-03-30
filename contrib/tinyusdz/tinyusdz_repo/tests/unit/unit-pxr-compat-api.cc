#ifdef _MSC_VER
#define NOMINMAX
#endif

#define TEST_NO_MAIN
#include "acutest.h"

#include "unit-pxr-compat-api.h"

#include "pxr-compat.hh"

using namespace PXR_INTERNAL_NS;

void pxr_compat_api_test(void) {
  {
    auto ref = UsdStage::CreateNew("creat.usda");
  }

  {
    auto ref = UsdStage::Open("input.usda");
  }

}
