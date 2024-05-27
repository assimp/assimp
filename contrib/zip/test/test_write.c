#include <stdio.h>
#include <stdlib.h>

#include <zip.h>

#include "minunit.h"

#if defined(_WIN32) || defined(_WIN64)
#define MKTEMP _mktemp
#define UNLINK _unlink
#else
#define MKTEMP mkstemp
#define UNLINK unlink
#endif

static char ZIPNAME[L_tmpnam + 1] = {0};
static char WFILE[L_tmpnam + 1] = {0};

void test_setup(void) {
  strncpy(ZIPNAME, "z-XXXXXX\0", L_tmpnam);
  strncpy(WFILE, "w-XXXXXX\0", L_tmpnam);

  MKTEMP(ZIPNAME);
  MKTEMP(WFILE);
}

void test_teardown(void) {
  UNLINK(WFILE);
  UNLINK(ZIPNAME);
}

#define CRC32DATA1 2220805626
#define TESTDATA1 "Some test data 1...\0"

MU_TEST(test_write) {
  struct zip_t *zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-1.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, strcmp(zip_entry_name(zip), "test/test-1.txt"));
  mu_assert_int_eq(0, zip_entry_index(zip));
  mu_assert_int_eq(strlen(TESTDATA1), zip_entry_size(zip));
  mu_check(CRC32DATA1 == zip_entry_crc32(zip));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(1, zip_is64(zip));

  zip_close(zip);
}

MU_TEST(test_write_utf) {
  struct zip_t *zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "тест/Если-б-не-было-войны.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(
      0, strcmp(zip_entry_name(zip), "тест/Если-б-не-было-войны.txt"));
  mu_assert_int_eq(0, zip_entry_index(zip));
  mu_assert_int_eq(strlen(TESTDATA1), zip_entry_size(zip));
  mu_check(CRC32DATA1 == zip_entry_crc32(zip));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(1, zip_is64(zip));

  zip_close(zip);
}

MU_TEST(test_fwrite) {
  const char *filename = WFILE;
  FILE *stream = NULL;
  struct zip_t *zip = NULL;
#if defined(_MSC_VER)
  if (0 != fopen_s(&stream, filename, "w+"))
#else
  if (!(stream = fopen(filename, "w+")))
#endif
  {
    // Cannot open filename
    mu_fail("Cannot open filename\n");
  }
  fwrite(TESTDATA1, sizeof(char), strlen(TESTDATA1), stream);
  mu_assert_int_eq(0, fclose(stream));

  zip = zip_open(ZIPNAME, 9, 'w');
  mu_check(zip != NULL);
  mu_assert_int_eq(0, zip_entry_open(zip, WFILE));
  mu_assert_int_eq(0, zip_entry_fwrite(zip, WFILE));
  mu_assert_int_eq(0, zip_entry_close(zip));
  mu_assert_int_eq(1, zip_is64(zip));

  zip_close(zip);
}

MU_TEST_SUITE(test_write_suite) {
  MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

  MU_RUN_TEST(test_write);
  MU_RUN_TEST(test_write_utf);
  MU_RUN_TEST(test_fwrite);
}

#define UNUSED(x) (void)x

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  MU_RUN_SUITE(test_write_suite);
  MU_REPORT();
  return MU_EXIT_CODE;
}