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
static int total_entries = 0;

#define TESTDATA1 "Some test data 1...\0"

void test_setup(void) {
  strncpy(ZIPNAME, "z-XXXXXX\0", L_tmpnam);
  MKTEMP(ZIPNAME);

  struct zip_t *zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');

  zip_entry_open(zip, "test/test-1.txt");
  zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1));
  zip_entry_close(zip);
  ++total_entries;

  zip_close(zip);
}

void test_teardown(void) { UNLINK(ZIPNAME); }

#define TESTDATA2 "Some test data 2...\0"
#define CRC32DATA2 2532008468

MU_TEST(test_append) {
  struct zip_t *zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'a');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "test\\test-2.txt"));
  mu_assert_int_eq(0, strcmp(zip_entry_name(zip), "test/test-2.txt"));
  mu_assert_int_eq(total_entries, zip_entry_index(zip));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2)));
  mu_assert_int_eq(strlen(TESTDATA2), zip_entry_size(zip));
  mu_check(CRC32DATA2 == zip_entry_crc32(zip));
  mu_assert_int_eq(0, zip_entry_close(zip));
  ++total_entries;
  zip_close(zip);

  zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'a');
  mu_assert_int_eq(0, zip_entry_open(zip, "test\\empty/"));
  mu_assert_int_eq(0, strcmp(zip_entry_name(zip), "test/empty/"));
  mu_assert_int_eq(0, zip_entry_size(zip));
  mu_assert_int_eq(0, zip_entry_crc32(zip));
  mu_assert_int_eq(total_entries, zip_entry_index(zip));
  mu_assert_int_eq(0, zip_entry_close(zip));
  ++total_entries;
  zip_close(zip);

  zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'a');
  mu_assert_int_eq(0, zip_entry_open(zip, "empty/"));
  mu_assert_int_eq(0, strcmp(zip_entry_name(zip), "empty/"));
  mu_assert_int_eq(0, zip_entry_size(zip));
  mu_assert_int_eq(0, zip_entry_crc32(zip));
  mu_assert_int_eq(total_entries, zip_entry_index(zip));
  mu_assert_int_eq(0, zip_entry_close(zip));
  ++total_entries;

  mu_assert_int_eq(0, zip_entry_open(zip, "dotfiles/.test"));
  mu_assert_int_eq(0, strcmp(zip_entry_name(zip), "dotfiles/.test"));
  mu_assert_int_eq(0, zip_entry_size(zip));
  mu_assert_int_eq(0, zip_entry_crc32(zip));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2)));
  mu_assert_int_eq(strlen(TESTDATA2), zip_entry_size(zip));
  mu_check(CRC32DATA2 == zip_entry_crc32(zip));
  mu_assert_int_eq(total_entries, zip_entry_index(zip));
  mu_assert_int_eq(0, zip_entry_close(zip));
  ++total_entries;

  mu_assert_int_eq(total_entries, zip_entries_total(zip));

  zip_close(zip);
}

MU_TEST_SUITE(test_append_suite) {
  MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

  MU_RUN_TEST(test_append);
}

#define UNUSED(x) (void)x

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  MU_RUN_SUITE(test_append_suite);
  MU_REPORT();
  return MU_EXIT_CODE;
}
