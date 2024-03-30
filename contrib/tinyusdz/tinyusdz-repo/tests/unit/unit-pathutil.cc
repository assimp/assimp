#ifdef _MSC_VER
#define NOMINMAX
#endif

#define TEST_NO_MAIN
#include "acutest.h"

#include "unit-pathutil.h"
#include "prim-types.hh"
#include "path-util.hh"

using namespace tinyusdz;

void pathutil_test(void) {
  {
    Path basepath("/", "");
    Path relpath("../bora", "");
    Path abspath("", "");
    std::string err;
    bool ret = pathutil::ResolveRelativePath(basepath, relpath, &abspath, &err);
    if (err.size()) {
      std::cout << err;
    }
    std::cout << "abs_path = " << abspath.full_path_name() << "\n";
    TEST_CHECK(ret == true);
    TEST_CHECK(abspath.prim_part() == "/bora");
  }

  {
    Path basepath("/root", "");
    Path relpath("../bora", "");
    Path abspath("", "");
    std::string err;
    bool ret = pathutil::ResolveRelativePath(basepath, relpath, &abspath);
    if (err.size()) {
      std::cout << err;
    }
    std::cout << "abs_path = " << abspath.full_path_name() << "\n";
    TEST_CHECK(ret == true);
    TEST_CHECK(abspath.prim_part() == "/bora");
  }

  {
    Path basepath("/root/muda", "");
    Path relpath("../bora", "");
    Path abspath("", "");
    std::string err;
    bool ret = pathutil::ResolveRelativePath(basepath, relpath, &abspath);
    if (err.size()) {
      std::cout << err;
    }
    std::cout << "abs_path = " << abspath.full_path_name() << "\n";
    TEST_CHECK(ret == true);
    TEST_CHECK(abspath.prim_part() == "/root/bora");
  }

  {
    Path basepath("/root", "");
    Path relpath("../../boraa", "");
    Path abspath("", "");
    std::string err;
    bool ret = pathutil::ResolveRelativePath(basepath, relpath, &abspath);
    if (err.size()) {
      std::cout << err;
    }
    std::cout << "abs_path = " << abspath.full_path_name() << "\n";
    TEST_CHECK(ret == true);
    TEST_CHECK(abspath.prim_part() == "/boraa");
  }

  {
    // Too deep
    Path basepath("/root", "");
    Path relpath("../../../boraaa", "");
    Path abspath("", "");
    std::string err;
    bool ret = pathutil::ResolveRelativePath(basepath, relpath, &abspath);
    if (err.size()) {
      std::cout << err;
    }
    std::cout << "abs_path = " << abspath.full_path_name() << "\n";
    TEST_CHECK(ret == false);
  }

  {
    Path basepath("/root", "");
    Path relpath("../bora1", "myprop");
    Path abspath("", "");
    std::string err;
    bool ret = pathutil::ResolveRelativePath(basepath, relpath, &abspath);
    if (err.size()) {
      std::cout << err;
    }
    std::cout << "abs_path = " << abspath.full_path_name() << "\n";
    TEST_CHECK(ret == true);
    TEST_CHECK(abspath.full_path_name() == "/bora1.myprop");
  }

  {
    Path basepath("/root", "");
    Path relpath("../bora2.myprop", "");
    Path abspath("", "");
    std::string err;
    bool ret = pathutil::ResolveRelativePath(basepath, relpath, &abspath);
    if (err.size()) {
      std::cout << err;
    }
    TEST_CHECK(ret == false);
  }

  {
    // `./` is invalid
    Path basepath("/root", "");
    Path relpath("./bora3", "");
    Path abspath("", "");
    std::string err;
    bool ret = pathutil::ResolveRelativePath(basepath, relpath, &abspath);
    if (err.size()) {
      std::cout << err;
    }
    TEST_CHECK(ret == false);
  }

  {
    Path basepath("/root", "");
    Path relpath("bora3", "");
    Path abspath("", "");
    std::string err;
    bool ret = pathutil::ResolveRelativePath(basepath, relpath, &abspath);
    if (err.size()) {
      std::cout << err;
    }
    TEST_CHECK(ret == true);
    TEST_CHECK(abspath.full_path_name() == "/root/bora3");
  }


  {
    // .. in the middle of relative path is invalid
    Path basepath("/root", "");
    Path relpath("../bora4/../dora", "");
    Path abspath("", "");
    std::string err;
    bool ret = pathutil::ResolveRelativePath(basepath, relpath, &abspath);
    if (err.size()) {
      std::cout << err;
    }
    TEST_CHECK(ret == false);
  }

}
