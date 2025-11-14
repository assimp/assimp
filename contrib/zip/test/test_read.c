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

#define CRC32DATA1 2220805626
#define TESTDATA1 "Some test data 1...\0"

#define TESTDATA2 "Some test data 2...\0"
#define CRC32DATA2 2532008468

void test_setup(void) {
  strncpy(ZIPNAME, "z-XXXXXX\0", L_tmpnam);
  MKTEMP(ZIPNAME);

  struct zip_t *zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');

  zip_entry_open(zip, "test/test-1.txt");
  zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1));
  zip_entry_close(zip);

  zip_entry_open(zip, "test\\test-2.txt");
  zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2));
  zip_entry_close(zip);

  zip_entry_open(zip, "test\\empty/");
  zip_entry_close(zip);

  zip_entry_open(zip, "empty/");
  zip_entry_close(zip);

  zip_entry_open(zip, "dotfiles/.test");
  zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2));
  zip_entry_close(zip);

  zip_close(zip);
}

void test_teardown(void) { remove(ZIPNAME); }

MU_TEST(test_read) {
  char *buf = NULL;
  ssize_t bufsize;
  size_t buftmp;

  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);
  mu_assert_int_eq(1, zip_is64(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "test\\test-1.txt"));
  mu_assert_int_eq(strlen(TESTDATA1), zip_entry_size(zip));
  mu_check(CRC32DATA1 == zip_entry_crc32(zip));
  bufsize = zip_entry_read(zip, (void **)&buf, &buftmp);
  mu_assert_int_eq(strlen(TESTDATA1), bufsize);
  mu_assert_int_eq((size_t)bufsize, buftmp);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA1, bufsize));
  mu_assert_int_eq(0, zip_entry_close(zip));
  free(buf);
  buf = NULL;

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-2.txt"));
  mu_assert_int_eq(strlen(TESTDATA2), zip_entry_size(zip));
  mu_check(CRC32DATA2 == zip_entry_crc32(zip));
  bufsize = zip_entry_read(zip, (void **)&buf, NULL);
  mu_assert_int_eq(strlen(TESTDATA2), (size_t)bufsize);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA2, (size_t)bufsize));
  mu_assert_int_eq(0, zip_entry_close(zip));
  free(buf);
  buf = NULL;

  mu_assert_int_eq(0, zip_entry_open(zip, "test\\empty/"));
  mu_assert_int_eq(0, strcmp(zip_entry_name(zip), "test/empty/"));
  mu_assert_int_eq(0, zip_entry_size(zip));
  mu_assert_int_eq(0, zip_entry_crc32(zip));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_noallocread) {
  ssize_t bufsize;
  size_t buftmp = strlen(TESTDATA2);
  char *buf = calloc(buftmp, sizeof(char));

  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);
  mu_assert_int_eq(1, zip_is64(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-2.txt"));
  bufsize = zip_entry_noallocread(zip, (void *)buf, buftmp);
  mu_assert_int_eq(buftmp, (size_t)bufsize);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA2, buftmp));
  mu_assert_int_eq(0, zip_entry_close(zip));
  free(buf);
  buf = NULL;

  buftmp = strlen(TESTDATA1);
  buf = calloc(buftmp, sizeof(char));
  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-1.txt"));
  bufsize = zip_entry_noallocread(zip, (void *)buf, buftmp);
  mu_assert_int_eq(buftmp, (size_t)bufsize);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA1, buftmp));
  mu_assert_int_eq(0, zip_entry_close(zip));
  free(buf);
  buf = NULL;

  buftmp = strlen(TESTDATA2);
  buf = calloc(buftmp, sizeof(char));
  mu_assert_int_eq(0, zip_entry_open(zip, "dotfiles/.test"));
  bufsize = zip_entry_noallocread(zip, (void *)buf, buftmp);
  mu_assert_int_eq(buftmp, (size_t)bufsize);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA2, buftmp));
  mu_assert_int_eq(0, zip_entry_close(zip));
  free(buf);
  buf = NULL;

  zip_close(zip);
}

MU_TEST_SUITE(test_read_suite) {
  MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

  MU_RUN_TEST(test_read);
  MU_RUN_TEST(test_noallocread);
}

#define UNUSED(x) (void)x

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  MU_RUN_SUITE(test_read_suite);
  MU_REPORT();
  return MU_EXIT_CODE;
}
