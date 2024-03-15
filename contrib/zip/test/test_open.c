#include <stdio.h>
#include <stdlib.h>

#include <zip.h>

#include "minunit.h"

#if defined(_WIN32) || defined(_WIN64)
#define MKTEMP _mktemp
#else
#define MKTEMP mkstemp
#endif

static char ZIPNAME[L_tmpnam + 1] = {0};

void test_setup(void) {
  strncpy(ZIPNAME, "z-XXXXXX\0", L_tmpnam);
  MKTEMP(ZIPNAME);
}

void test_teardown(void) { remove(ZIPNAME); }

MU_TEST(test_openwitherror) {
  int errnum;
  struct zip_t *zip =
      zip_openwitherror(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'r', &errnum);
  mu_check(zip == NULL);
  mu_assert_int_eq(ZIP_ERINIT, errnum);

  zip = zip_openwitherror(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w', &errnum);
  mu_check(zip != NULL);
  mu_assert_int_eq(0, errnum);

  zip_close(zip);
}

MU_TEST(test_stream_openwitherror) {
  int errnum;
  struct zip_t *zip = zip_stream_openwitherror(
      NULL, 0, ZIP_DEFAULT_COMPRESSION_LEVEL, 'r', &errnum);
  mu_check(zip == NULL);
  mu_assert_int_eq(ZIP_EINVMODE, errnum);

  zip = zip_stream_openwitherror(NULL, 0, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                 &errnum);
  mu_check(zip != NULL);
  mu_assert_int_eq(0, errnum);

  zip_stream_close(zip);
}

MU_TEST_SUITE(test_entry_suite) {
  MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

  MU_RUN_TEST(test_openwitherror);
  MU_RUN_TEST(test_stream_openwitherror);
}

#define UNUSED(x) (void)x

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  MU_RUN_SUITE(test_entry_suite);
  MU_REPORT();
  return MU_EXIT_CODE;
}