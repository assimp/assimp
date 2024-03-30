#ifdef _MSC_VER
#define NOMINMAX
#endif

#define TEST_NO_MAIN
#include "acutest.h"

#include "unit-value-types.h"
#include "value-types.hh"

using namespace tinyusdz;

void value_types_test(void) {

  value::token tok1("bora");
  value::token tok2("muda");
  value::token tok3("bora");
  TEST_CHECK(tok1 == tok1);
  TEST_CHECK(tok1 != tok2);
  TEST_CHECK(tok1 == tok3);

  TEST_CHECK(value::GetTypeName(value::TYPE_ID_TOKEN) == "token");
  TEST_CHECK(value::GetTypeName(value::TYPE_ID_TOKEN|value::TYPE_ID_1D_ARRAY_BIT) == "token[]");

  TEST_CHECK(value::GetTypeId("token") == value::TYPE_ID_TOKEN);
  TEST_CHECK(value::GetTypeId("token[]") == (value::TYPE_ID_TOKEN|value::TYPE_ID_1D_ARRAY_BIT));

  TEST_CHECK(!value::TryGetTypeName(value::TYPE_ID_ALL));

}
