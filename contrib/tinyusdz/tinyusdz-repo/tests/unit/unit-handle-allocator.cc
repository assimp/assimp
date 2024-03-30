#ifdef _MSC_VER
#define NOMINMAX
#endif

#define TEST_NO_MAIN
#include "acutest.h"

#include "handle-allocator.hh"
#include "unit-common.hh"

#include <random>
#include <functional>


using namespace tinyusdz;
using namespace tinyusdz_test;

void handle_allocator_test(void) {

  std::random_device seed_gen;
  std::mt19937 engine(seed_gen());

  uint64_t n = 1024*16;

  std::vector<uint64_t> perm_handles;

  HandleAllocator<uint64_t> allocator;

  bool ok = true;
  for (size_t i = 0; i < n; i++) {
    uint64_t handle;
    if (!allocator.Allocate(&handle)) {
      ok = false;
      break; 
    }

    perm_handles.push_back(handle);
  }
  TEST_CHECK(ok);
  TEST_CHECK(allocator.Size() == n);

  std::shuffle(perm_handles.begin(), perm_handles.end(), engine);

  // handle should exist
  ok = true;
  for (size_t i = 0; i < perm_handles.size(); i++) {
    //std::cout << "handle = " << perm_handles[i] << ", Size = " << allocator.Size() << "\n";
    if (!allocator.Has(perm_handles[i])) {
      ok = false;
      break;
    }
  }
  TEST_CHECK(ok);

  //std::cout << "del \n";
  // delete
  ok = true;
  for (size_t i = 0; i < perm_handles.size(); i++) {
    //std::cout << "del handle " << perm_handles[i] << ", Size = " << allocator.Size() << "\n";
    if (!allocator.Release(perm_handles[i])) {
      ok = false;
      break;
    }
  }
  TEST_CHECK(ok);
  //std::cout << "del done\n";
  TEST_CHECK(allocator.Size() == 0);
  
  // no handle exists
  ok = true;
  for (size_t i = 0; i < perm_handles.size(); i++) {
    //std::cout << "has handle " << perm_handles[i] << " = " << allocator.Has(perm_handles[i]) << "\n";
    if (allocator.Has(perm_handles[i])) {
      ok = false;
      break;
    }
  }
  TEST_CHECK(ok);

  // reallocate test
  ok = true;
  std::vector<uint64_t> handles;
  for (size_t i = 0; i < perm_handles.size(); i++) {
    uint64_t handle;
    if (!allocator.Allocate(&handle)) {
      ok = false;
      break;
    }
    handles.push_back(handle);
  }
  TEST_CHECK(ok);
  // uniqueness check
  std::sort(handles.begin(), handles.end());
  // array items = [1, ..., n]
  TEST_CHECK(handles.front() == 1); 
  uint64_t &last = handles.back();
  TEST_CHECK(last == n);
  handles.erase(std::unique(handles.begin(), handles.end()), handles.end());
  TEST_CHECK(handles.size() == perm_handles.size());


}
