#ifdef _MSC_VER
#define NOMINMAX
#endif

#define TEST_NO_MAIN
#include "acutest.h"

#include "unit-customdata.h"
#include "value-pprint.hh"
#include "prim-types.hh"

using namespace tinyusdz;

void customdata_test(void) {

  MetaVariable doubleVal = double(3.0);
  MetaVariable intVal = int(9);
  MetaVariable stringVal = std::string("dora");

  CustomDataType customData;

  //
  // customData = {
  //    dictionary hello = {
  //      double myval = 3.0
  //    }
  // } 

  // Namespace ':' to create nested dictionary data.
  TEST_CHECK(tinyusdz::SetCustomDataByKey("hello:myval", doubleVal, customData));

  TEST_CHECK(tinyusdz::HasCustomDataKey(customData, "hello:myval"));

  MetaVariable metavar;
  bool ret = tinyusdz::GetCustomDataByKey(customData, "hello:myval", &metavar);

  TEST_CHECK(ret == true);

  double retval{0.0};
  metavar.get_value<double>(&retval);

  TEST_CHECK(retval == 3.0);

  // Add another key
  TEST_CHECK(tinyusdz::SetCustomDataByKey("hello:myval2", stringVal, customData));

  TEST_CHECK(tinyusdz::HasCustomDataKey(customData, "hello:myval"));
  TEST_CHECK(tinyusdz::HasCustomDataKey(customData, "hello:myval2"));

  MetaVariable metavar2;
  ret = tinyusdz::GetCustomDataByKey(customData, "hello:myval2", &metavar2);

  TEST_CHECK(ret == true);

  std::string retval_str;
  metavar2.get_value<std::string>(&retval_str);

  TEST_CHECK(retval_str == "dora");

  // override
  {
    TEST_CHECK(tinyusdz::SetCustomDataByKey("hello:myval", intVal, customData));
    ret = tinyusdz::GetCustomDataByKey(customData, "hello:myval", &metavar);
    TEST_CHECK(ret == true);

    int ival{0};
    metavar.get_value<int>(&ival);

    TEST_CHECK(ival == 9);
  }
}
