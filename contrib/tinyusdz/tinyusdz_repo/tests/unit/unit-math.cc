#ifdef _MSC_VER
#define NOMINMAX
#endif

#include <iostream>

#define TEST_NO_MAIN
#include "acutest.h"

#include "value-types.hh"
#include "unit-value-types.h"
#include "prim-types.hh"
#include "math-util.inc"
#include "unit-common.hh"


using namespace tinyusdz;
using namespace tinyusdz_test;

void math_sin_pi_test(void) {

  double kPI = math::constants<double>::pi();

  for (int i = -360; i <= 360; i++) {
    double ref_v = std::sin(double(i) * kPI / 180.0);
    double sin_v = math::sin_pi(double(i)/180.0);
    // TODO: dyanmically change eps based on input degree value.
    if (!math::is_close(ref_v, sin_v, 5*std::numeric_limits<double>::epsilon())) {
      printf("sin(%d degree) differes: ref = %lf, sin_pi = %lf\n", i, ref_v, sin_v);
      TEST_CHECK(0);
    }
  }
  TEST_CHECK(1); // ok
}

void math_cos_pi_test(void) {
  double kPI = math::constants<double>::pi();

  for (int i = -360; i <= 360; i++) {
    double ref_v = std::cos(double(i) * kPI / 180.0);
    double cos_v = math::cos_pi(double(i)/180.0);
    // TODO: dyanmically change eps based on input degree value.
    if (!math::is_close(ref_v, cos_v, 4*std::numeric_limits<double>::epsilon())) {
      printf("cos(%d degree) differes: ref = %lf, sin_pi = %lf\n", i, ref_v, cos_v);
      TEST_CHECK(0);
    }
  }
  TEST_CHECK(1); // ok
}

void math_sin_cos_pi_test(void) {

  // should exactly match(whereas std::sin/cos is not)
  TEST_CHECK(math::is_close(math::cos_pi(315.0/180.0), -math::sin_pi(315.0/180.0), 0.0));
  TEST_MSG("cos(315) = %lf, sin(315) = %lf", math::cos_pi(315.0/180.0), math::sin_pi(315.0/180.0));

  TEST_CHECK(math::is_close(math::cos_pi(225.0/180.0), math::sin_pi(225.0/180.0), 0.0));
  TEST_MSG("cos(225) = %lf, sin(225) = %lf", math::cos_pi(225.0/180.0), math::sin_pi(225.0/180.0));

  TEST_CHECK(math::is_close(-math::cos_pi(135.0/180.0), math::sin_pi(135.0/180.0), 0.0));
  TEST_MSG("cos(135) = %lf, sin(135) = %lf", math::cos_pi(135.0/180.0), math::sin_pi(135.0/180.0));

  TEST_CHECK(math::is_close(math::cos_pi(45.0/180.0), math::sin_pi(45.0/180.0), 0.0));
  TEST_MSG("cos(45) = %lf, sin(45) = %lf", math::cos_pi(45.0/180.0), math::sin_pi(45.0/180.0));

  TEST_CHECK(math::is_close(math::cos_pi(-45.0/180.0), -math::sin_pi(-45.0/180.0), 0.0));
  TEST_MSG("cos(-45) = %lf, sin(-45) = %lf", math::cos_pi(-45.0/180.0), math::sin_pi(-45.0/180.0));

  TEST_CHECK(math::is_close(math::cos_pi(-135.0/180.0), math::sin_pi(-135.0/180.0), 0.0));
  TEST_MSG("cos(-135) = %lf, sin(-135) = %lf", math::cos_pi(-135.0/180.0), math::sin_pi(-135.0/180.0));

  TEST_CHECK(math::is_close(-math::cos_pi(-225.0/180.0), math::sin_pi(-225.0/180.0), 0.0));
  TEST_MSG("cos(-225) = %lf, sin(-225) = %lf", math::cos_pi(-225.0/180.0), math::sin_pi(-225.0/180.0));

  TEST_CHECK(math::is_close(math::cos_pi(-315.0/180.0), math::sin_pi(-315.0/180.0), 0.0));
  TEST_MSG("cos(-315) = %lf, sin(-315) = %lf", math::cos_pi(-315.0/180.0), math::sin_pi(-315.0/180.0));

  // must be exactly zero
  TEST_CHECK(math::is_close(math::cos_pi(90.0/180.0), 0.0, 0.0));
  TEST_CHECK(math::is_close(math::cos_pi(270.0/180.0), 0.0, 0.0));
  TEST_CHECK(math::is_close(math::cos_pi(-90.0/180.0), 0.0, 0.0));
  TEST_CHECK(math::is_close(math::cos_pi(-270.0/180.0), 0.0, 0.0));
  TEST_CHECK(math::is_close(math::sin_pi(0.0), 0.0, 0.0));
  TEST_CHECK(math::is_close(math::sin_pi(360.0/180.0), 0.0, 0.0));
  TEST_CHECK(math::is_close(math::sin_pi(180.0/180.0), 0.0, 0.0));
  TEST_CHECK(math::is_close(math::sin_pi(-180.0/180.0), 0.0, 0.0));
  TEST_CHECK(math::is_close(math::sin_pi(-360.0/180.0), 0.0, 0.0));
}

