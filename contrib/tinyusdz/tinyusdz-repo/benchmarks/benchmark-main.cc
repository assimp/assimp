#include <unistd.h>
#include "ubench.h"

#include "value-types.hh"
#include "prim-types.hh"
#include "usdGeom.hh"

using namespace tinyusdz;

UBENCH(perf, vector_double_push_back_10M)
{
  std::vector<double> v;
  constexpr size_t niter = 10 * 10000;
  for (size_t i = 0; i < niter; i++) {
    v.push_back(double(i));
  }
}

UBENCH(perf, any_value_double_10M)
{
  constexpr size_t niter = 10 * 10000;
  for (size_t i = 0; i < niter; i++) {
    linb::any a;
    a = double(i);
  }
}

UBENCH(perf, thelink2012_any_float_10M)
{
  constexpr size_t niter = 10 * 10000;
  for (size_t i = 0; i < niter; i++) {
    linb::any a;
    a = float(i);
  }
}

UBENCH(perf, thelink2012_any_double_10M)
{
  constexpr size_t niter = 10 * 10000;

  std::vector<linb::any> v;

  for (size_t i = 0; i < niter; i++) {
    v.push_back(double(i));
  }
}

UBENCH(perf, any_value_100M)
{
  constexpr size_t niter = 100 * 10000;
  for (size_t i = 0; i < niter; i++) {
    tinyusdz::value::Value a;
    a = i;
  }
}

UBENCH(perf, timesamples_double_10M)
{
  constexpr size_t ns = 10 * 10000;

  tinyusdz::value::TimeSamples ts;

  for (size_t i = 0; i < ns; i++) {
    ts.times.push_back(double(i));
    ts.values.push_back(double(i));
  }
}

UBENCH(perf, gprim_10M)
{
  constexpr size_t niter = 10 * 10000;
  std::vector<value::Value> prims;

  tinyusdz::Xform xform;
  for (size_t i = 0; i < niter; i++) {
    prims.emplace_back(xform);
  }

}

// Its rougly 3.5x slower compared to `string_vector_10M` in single-threaded run on Threadripper 1950X
// (even not using thread_safe_databse(no mutex))
UBENCH(perf, token_vector_10M)
{
  constexpr size_t niter = 10 * 10000;
  std::vector<value::token> v;

  for (size_t i = 0; i < niter; i++) {
    value::token tok(std::to_string(i));
    v.emplace_back(tok);
  }

}

UBENCH(perf, string_vector_10M)
{
  constexpr size_t niter = 10 * 10000;
  std::vector<std::string> v;

  for (size_t i = 0; i < niter; i++) {
    std::string s(std::to_string(i));
    v.emplace_back(s);
  }

}

//int main(int argc, char **argv)
//{
//  benchmark_any_type();
//
//  return 0;
//}

UBENCH_MAIN();
