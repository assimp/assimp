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

#define MZ_ZIP64_MAX_CENTRAL_EXTRA_FIELD_SIZE                                  \
  (sizeof(unsigned short) * 2 + sizeof(unsigned long long) * 3)
#define MZ_ZIP_LOCAL_DIR_HEADER_SIZE 30

static char ZIPNAME[L_tmpnam + 1] = {0};

#define CRC32DATA1 2220805626
#define TESTDATA1 "Some test data 1...\0"

#define TESTDATA2 "Some test data 2...\0"
#define CRC32DATA2 2532008468

static int total_entries = 0;

void test_setup(void) {
  strncpy(ZIPNAME, "z-XXXXXX\0", L_tmpnam);
  MKTEMP(ZIPNAME);

  struct zip_t *zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');

  zip_entry_open(zip, "test/test-1.txt");
  zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1));
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "test\\test-2.txt");
  zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2));
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "test\\empty/");
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "empty/");
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "dotfiles/.test");
  zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2));
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "delete.me");
  zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1));
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "_");
  zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2));
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "delete/file.1");
  zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1));
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "delete/file.2");
  zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2));
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "deleteme/file.3");
  zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1));
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "delete/file.4");
  zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2));
  zip_entry_close(zip);
  ++total_entries;

  zip_close(zip);
}

void test_teardown(void) {
  total_entries = 0;

  UNLINK(ZIPNAME);
}

MU_TEST(test_entry_name) {
  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  mu_check(zip_entry_name(zip) == NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "test\\test-1.txt"));
  mu_check(NULL != zip_entry_name(zip));
  mu_assert_int_eq(0, strcmp(zip_entry_name(zip), "test/test-1.txt"));
  mu_assert_int_eq(strlen(TESTDATA1), zip_entry_size(zip));
  mu_check(CRC32DATA1 == zip_entry_crc32(zip));
  mu_assert_int_eq(0, zip_entry_index(zip));

  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-2.txt"));
  mu_check(NULL != zip_entry_name(zip));
  mu_assert_int_eq(0, strcmp(zip_entry_name(zip), "test/test-2.txt"));
  mu_assert_int_eq(strlen(TESTDATA2), zip_entry_size(zip));
  mu_check(CRC32DATA2 == zip_entry_crc32(zip));
  mu_assert_int_eq(1, zip_entry_index(zip));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_entry_opencasesensitive) {
  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  mu_check(zip_entry_name(zip) == NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "test/TEST-1.TXT"));
  mu_check(NULL != zip_entry_name(zip));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(ZIP_ENOENT,
                   zip_entry_opencasesensitive(zip, "test/TEST-1.TXT"));

  zip_close(zip);
}

MU_TEST(test_entry_index) {
  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "test\\test-1.txt"));
  mu_assert_int_eq(0, zip_entry_index(zip));
  mu_assert_int_eq(0, strcmp(zip_entry_name(zip), "test/test-1.txt"));
  mu_assert_int_eq(strlen(TESTDATA1), zip_entry_size(zip));
  mu_check(CRC32DATA1 == zip_entry_crc32(zip));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-2.txt"));
  mu_assert_int_eq(1, zip_entry_index(zip));
  mu_assert_int_eq(0, strcmp(zip_entry_name(zip), "test/test-2.txt"));
  mu_assert_int_eq(strlen(TESTDATA2), zip_entry_size(zip));
  mu_check(CRC32DATA2 == zip_entry_crc32(zip));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_entry_openbyindex) {
  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_openbyindex(zip, 1));
  mu_assert_int_eq(1, zip_entry_index(zip));

  mu_assert_int_eq(strlen(TESTDATA2), zip_entry_size(zip));
  mu_check(CRC32DATA2 == zip_entry_crc32(zip));

  mu_assert_int_eq(0, strcmp(zip_entry_name(zip), "test/test-2.txt"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_openbyindex(zip, 0));
  mu_assert_int_eq(0, zip_entry_index(zip));
  mu_assert_int_eq(strlen(TESTDATA1), zip_entry_size(zip));
  mu_check(CRC32DATA1 == zip_entry_crc32(zip));

  mu_assert_int_eq(0, strcmp(zip_entry_name(zip), "test/test-1.txt"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_entry_read) {
  char *bufencode1 = NULL;
  char *bufencode2 = NULL;
  char *buf = NULL;
  size_t bufsize;

  struct zip_t *zip =
      zip_stream_open(NULL, 0, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-1.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  ssize_t n = zip_stream_copy(zip, (void **)&bufencode1, NULL);
  zip_stream_copy(zip, (void **)&bufencode2, &bufsize);
  mu_assert_int_eq(0, strncmp(bufencode1, bufencode2, bufsize));

  zip_stream_close(zip);

  struct zip_t *zipstream = zip_stream_open(bufencode1, n, 0, 'r');
  mu_check(zipstream != NULL);

  mu_assert_int_eq(0, zip_entry_open(zipstream, "test/test-1.txt"));
  n = zip_entry_read(zipstream, (void **)&buf, NULL);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA1, (size_t)n));
  mu_assert_int_eq(0, zip_entry_close(zipstream));

  zip_stream_close(zipstream);

  free(buf);
  free(bufencode1);
  free(bufencode2);
}

MU_TEST(test_list_entries) {
  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  int i = 0, n = zip_entries_total(zip);
  for (; i < n; ++i) {
    mu_assert_int_eq(0, zip_entry_openbyindex(zip, i));
    fprintf(stdout, "[%d]: %s", i, zip_entry_name(zip));
    if (zip_entry_isdir(zip)) {
      fprintf(stdout, " (DIR)");
    }
    fprintf(stdout, "\n");
    mu_assert_int_eq(0, zip_entry_close(zip));
  }

  zip_close(zip);
}

MU_TEST(test_entries_deletebyindex) {
  size_t entries[] = {5, 6, 7, 9, 8};

  struct zip_t *zip = zip_open(ZIPNAME, 0, 'd');
  mu_check(zip != NULL);

  mu_assert_int_eq(5, zip_entries_deletebyindex(zip, entries, 5));

  zip_close(zip);

  zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "delete.me"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "delete.me: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "_"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "_: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "delete/file.1"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "delete/file.1: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "deleteme/file.3"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "delete/file.3: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "delete/file.2"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "delete/file.2: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(total_entries - 5, zip_entries_total(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "delete/file.4"));

  size_t buftmp = 0;
  char *buf = NULL;
  ssize_t bufsize = zip_entry_read(zip, (void **)&buf, &buftmp);

  mu_assert_int_eq(bufsize, strlen(TESTDATA2));
  mu_assert_int_eq((size_t)bufsize, buftmp);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA2, bufsize));
  mu_assert_int_eq(0, zip_entry_close(zip));

  free(buf);
  buf = NULL;

  zip_close(zip);
}

MU_TEST(test_entries_deleteinvalid) {
  size_t entries[] = {111, 222, 333, 444};

  struct zip_t *zip = zip_open(ZIPNAME, 0, 'd');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entries_deletebyindex(zip, entries, 4));

  zip_close(zip);

  zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "delete.me"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "_"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "delete/file.1"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "deleteme/file.3"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "delete/file.2"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(total_entries, zip_entries_total(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "delete/file.4"));

  size_t buftmp = 0;
  char *buf = NULL;
  ssize_t bufsize = zip_entry_read(zip, (void **)&buf, &buftmp);

  mu_assert_int_eq(bufsize, strlen(TESTDATA2));
  mu_assert_int_eq((size_t)bufsize, buftmp);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA2, bufsize));
  mu_assert_int_eq(0, zip_entry_close(zip));

  free(buf);
  buf = NULL;

  zip_close(zip);
}

MU_TEST(test_entries_delete) {
  char *entries[] = {"delete.me", "_", "delete/file.1", "deleteme/file.3",
                     "delete/file.2"};

  struct zip_t *zip = zip_open(ZIPNAME, 0, 'd');
  mu_check(zip != NULL);

  mu_assert_int_eq(5, zip_entries_delete(zip, entries, 5));

  zip_close(zip);

  zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "delete.me"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "delete.me: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "_"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "_: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "delete/file.1"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "delete/file.1: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "deleteme/file.3"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "delete/file.3: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "delete/file.2"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "delete/file.2: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(total_entries - 5, zip_entries_total(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "delete/file.4"));

  size_t buftmp = 0;
  char *buf = NULL;
  ssize_t bufsize = zip_entry_read(zip, (void **)&buf, &buftmp);

  mu_assert_int_eq(bufsize, strlen(TESTDATA2));
  mu_assert_int_eq((size_t)bufsize, buftmp);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA2, bufsize));
  mu_assert_int_eq(0, zip_entry_close(zip));

  free(buf);
  buf = NULL;

  zip_close(zip);
}

MU_TEST(test_entry_offset) {
  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  unsigned long long off = 0ULL;
  int i = 0, n = zip_entries_total(zip);
  for (; i < n; i++) {
    mu_assert_int_eq(0, zip_entry_openbyindex(zip, i));
    mu_assert_int_eq(i, zip_entry_index(zip));

    mu_assert_int_eq(off, zip_entry_header_offset(zip));

    off = zip_entry_header_offset(zip) + MZ_ZIP_LOCAL_DIR_HEADER_SIZE +
          strlen(zip_entry_name(zip)) + MZ_ZIP64_MAX_CENTRAL_EXTRA_FIELD_SIZE +
          zip_entry_comp_size(zip);
    fprintf(stdout, "\n[%d: %s]: header: %llu, dir: %llu, size: %llu (%llu)\n",
            i, zip_entry_name(zip), zip_entry_header_offset(zip),
            zip_entry_dir_offset(zip), zip_entry_comp_size(zip), off);

    mu_assert_int_eq(0, zip_entry_close(zip));
  }

  zip_close(zip);
}

MU_TEST_SUITE(test_entry_suite) {
  MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

  MU_RUN_TEST(test_entry_name);
  MU_RUN_TEST(test_entry_opencasesensitive);
  MU_RUN_TEST(test_entry_index);
  MU_RUN_TEST(test_entry_openbyindex);
  MU_RUN_TEST(test_entry_read);
  MU_RUN_TEST(test_list_entries);
  MU_RUN_TEST(test_entries_deletebyindex);
  MU_RUN_TEST(test_entries_delete);
  MU_RUN_TEST(test_entry_offset);
}

#define UNUSED(x) (void)x

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  MU_RUN_SUITE(test_entry_suite);
  MU_REPORT();
  return MU_EXIT_CODE;
}
