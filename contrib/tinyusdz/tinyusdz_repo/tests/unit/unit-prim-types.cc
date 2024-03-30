#ifdef _MSC_VER
#define NOMINMAX
#endif

#define TEST_NO_MAIN
#include "acutest.h"

#include "unit-prim-types.h"
#include "prim-types.hh"

using namespace tinyusdz;

void prim_type_test(void) {
  // Path
  {
    Path path("/", "");
    TEST_CHECK(path.is_root_path() == true);
    TEST_CHECK(path.is_root_prim() == false);
    // invalid 
    TEST_CHECK(path.get_parent_path().is_valid() == false);
  }

  {
    Path path("/bora", "");
    auto ret = path.split_at_root();
    TEST_CHECK(std::get<0>(ret).full_path_name() == "/bora");
    TEST_CHECK(std::get<1>(ret).is_empty() == true);
    TEST_CHECK(path.get_parent_path().full_path_name() == "/");
    TEST_CHECK(path.get_parent_prim_path().full_path_name() == "/bora");
  }

  {
    Path path("/dora/bora", "");
    TEST_CHECK(path.element_name() == "bora"); // leaf name
    auto ret = path.split_at_root();
    TEST_CHECK(std::get<0>(ret).is_valid() == true);
    TEST_CHECK(std::get<0>(ret).full_path_name() == "/dora");
    TEST_CHECK(std::get<1>(ret).is_valid() == true);
    TEST_CHECK(std::get<1>(ret).full_path_name() == "/bora");
  }

  {
    Path path("dora", "");
    auto ret = path.split_at_root();
    TEST_CHECK(std::get<0>(ret).is_empty() == true);
    TEST_CHECK(std::get<1>(ret).is_valid() == true);
    TEST_CHECK(std::get<1>(ret).full_path_name() == "dora");
    TEST_CHECK(path.get_parent_path().is_valid() == false);
  }

  {
    Path rpath("dora", "");
    TEST_CHECK(rpath.make_relative().full_path_name() == "dora");

    Path apath("/dora", "");
    TEST_CHECK(apath.make_relative().full_path_name() == "dora");

    Path cpath("/dora", "");
    Path c;
    TEST_CHECK(c.make_relative(cpath).full_path_name() == "dora"); // std::move
  }

  {
    Path rpath("/dora", "bora");
    TEST_CHECK(rpath.full_path_name() == "/dora.bora");

    // Currently Allow prop path in prim
    // TODO: Disallow prop path in prim
    Path apath("/dora.bora", "");
    TEST_CHECK(apath.full_path_name() == "/dora.bora");
    TEST_CHECK(apath.element_name() == "bora");
  }

  {
    Path apath("/dora", "bora");
    TEST_CHECK(apath.full_path_name() == "/dora.bora");
    TEST_CHECK(apath.element_name() == "bora");
    std::cout << "parent_path = " << apath.get_parent_path().full_path_name() << "\n";
    std::cout << "parent_path = " << apath.get_parent_prim_path().full_path_name() << "\n";

    TEST_CHECK(apath.get_parent_path().full_path_name() == "/dora");
  }

  {
    Path apath("/dora/bora", "");
    Path bpath("/dora", "");
    Path cpath("/doraa", "");
    Path dpath = bpath.AppendProperty("hello");
    Path epath = bpath.AppendProperty("hell");

    std::cout << "epath = " << epath.full_path_name() << "\n";

    TEST_CHECK(bpath < apath);
    TEST_CHECK(bpath < cpath);
    TEST_CHECK(bpath < dpath);
    TEST_CHECK(epath < dpath);
  }

  {
    Path apath("/dora/bora", "");
    Path bpath("/dora/bora2", "");
    Path cpath("/doraa", "");
    Path dpath("/", "");
    Path epath("/dora", "");
    Path fpath("/dora", "bora");
    Path gpath("/dora2", "bora");

    TEST_CHECK(apath.has_prefix(dpath) == true);
    TEST_CHECK(apath.has_prefix(epath) == true);
    TEST_CHECK(bpath.has_prefix(apath) == false);
    TEST_CHECK(apath.has_prefix(cpath) == false);
    TEST_CHECK(fpath.has_prefix(dpath) == true);
    TEST_CHECK(fpath.has_prefix(fpath) == true);
    TEST_CHECK(gpath.has_prefix(fpath) == false);
  }

}

void prim_add_test(void) {
  Model amodel;
  Model bmodel;
  Model cmodel;
  Model dmodel;
  Model rootmodel;

  Prim aprim("test01", amodel);
  Prim bprim("test02", bmodel);
  Prim cprim("test01", cmodel);
  Prim dprim("test02", dmodel);
  Prim root("root", rootmodel);

  TEST_CHECK(root.add_child(std::move(aprim)));

  TEST_CHECK(root.add_child(std::move(bprim)));

  // cannot add child Prim with same elementName
  TEST_CHECK(!root.add_child(std::move(cprim), /* rename_if_required */false)); 

  // can add child Prim with renaming
  TEST_CHECK(root.add_child(std::move(dprim), /* rename_if_required */true)); 
  
}
