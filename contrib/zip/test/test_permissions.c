#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

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
static char XFILE[L_tmpnam + 1] = {0};
static char RFILE[L_tmpnam + 1] = {0};
static char WFILE[L_tmpnam + 1] = {0};

void test_setup(void) {
  strncpy(ZIPNAME, "z-XXXXXX\0", L_tmpnam);
  strncpy(XFILE, "x-XXXXXX\0", L_tmpnam);
  strncpy(RFILE, "r-XXXXXX\0", L_tmpnam);
  strncpy(WFILE, "w-XXXXXX\0", L_tmpnam);

  MKTEMP(ZIPNAME);
  MKTEMP(XFILE);
  MKTEMP(RFILE);
  MKTEMP(WFILE);
}

void test_teardown(void) {
  UNLINK(WFILE);
  UNLINK(RFILE);
  UNLINK(XFILE);
  UNLINK(ZIPNAME);
}

#if defined(_MSC_VER) || defined(__MINGW32__)
#define MZ_FILE_STAT_STRUCT _stat
#define MZ_FILE_STAT _stat
#else
#define MZ_FILE_STAT_STRUCT stat
#define MZ_FILE_STAT stat
#endif

#define XMODE 0100777
#define RMODE 0100444
#define WMODE 0100666
#define UNIXMODE 0100600

MU_TEST(test_exe_permissions) {
  struct MZ_FILE_STAT_STRUCT file_stats;

  const char *filenames[] = {XFILE};
  FILE *f = fopen(XFILE, "w");
  fclose(f);
  chmod(XFILE, XMODE);

  mu_assert_int_eq(0, zip_create(ZIPNAME, filenames, 1));
  remove(XFILE);

  mu_assert_int_eq(0, zip_extract(ZIPNAME, ".", NULL, NULL));

  mu_assert_int_eq(0, MZ_FILE_STAT(XFILE, &file_stats));
  mu_assert_int_eq(XMODE, file_stats.st_mode);
}

MU_TEST(test_read_permissions) {
  struct MZ_FILE_STAT_STRUCT file_stats;

  const char *filenames[] = {RFILE};
  FILE *f = fopen(RFILE, "w");
  fclose(f);
  chmod(RFILE, RMODE);

  mu_assert_int_eq(0, zip_create(ZIPNAME, filenames, 1));
  remove(RFILE);

  mu_assert_int_eq(0, zip_extract(ZIPNAME, ".", NULL, NULL));
  mu_assert_int_eq(0, MZ_FILE_STAT(RFILE, &file_stats));
  mu_assert_int_eq(RMODE, file_stats.st_mode);

  // chmod from 444 to 666 to be able delete the file on windows
  chmod(RFILE, WMODE);
}

MU_TEST(test_write_permissions) {
  struct MZ_FILE_STAT_STRUCT file_stats;

  const char *filenames[] = {WFILE};
  FILE *f = fopen(WFILE, "w");
  fclose(f);
  chmod(WFILE, WMODE);

  mu_assert_int_eq(0, zip_create(ZIPNAME, filenames, 1));
  remove(WFILE);

  mu_assert_int_eq(0, zip_extract(ZIPNAME, ".", NULL, NULL));
  mu_assert_int_eq(0, MZ_FILE_STAT(WFILE, &file_stats));
  mu_assert_int_eq(WMODE, file_stats.st_mode);
}

#define TESTDATA1 "Some test data 1...\0"

MU_TEST(test_unix_permissions) {
  // UNIX or APPLE
  struct MZ_FILE_STAT_STRUCT file_stats;

  struct zip_t *zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, RFILE));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  mu_assert_int_eq(0, zip_extract(ZIPNAME, ".", NULL, NULL));

  mu_assert_int_eq(0, MZ_FILE_STAT(RFILE, &file_stats));

  mu_assert_int_eq(UNIXMODE, file_stats.st_mode);
}

MU_TEST(test_mtime) {
  struct MZ_FILE_STAT_STRUCT file_stat1, file_stat2;

  const char *filename = "test.data";
  FILE *stream = NULL;
  struct zip_t *zip = NULL;
#if defined(_MSC_VER)
  if (0 != fopen_s(&stream, filename, "w+"))
#else
  if (!(stream = fopen(filename, "w+")))
#endif
  {
    mu_fail("Cannot open filename\n");
  }
  fwrite(TESTDATA1, sizeof(char), strlen(TESTDATA1), stream);
  mu_assert_int_eq(0, fclose(stream));

  memset(&file_stat1, 0, sizeof(file_stat1));
  memset(&file_stat2, 0, sizeof(file_stat2));

  zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, filename));
  mu_assert_int_eq(0, zip_entry_fwrite(zip, filename));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  mu_assert_int_eq(0, MZ_FILE_STAT(filename, &file_stat1));
  remove(filename);

  mu_assert_int_eq(0, zip_extract(ZIPNAME, ".", NULL, NULL));
  mu_assert_int_eq(0, MZ_FILE_STAT(filename, &file_stat2));
  remove(filename);

  fprintf(stdout, "file_stat1.st_mtime: %lu\n", file_stat1.st_mtime);
  fprintf(stdout, "file_stat2.st_mtime: %lu\n", file_stat2.st_mtime);
  mu_check(labs(file_stat1.st_mtime - file_stat2.st_mtime) <= 1);
}

MU_TEST_SUITE(test_permissions_suite) {
  MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

#if defined(_WIN32) || defined(__WIN32__)
#else
  MU_RUN_TEST(test_exe_permissions);
  MU_RUN_TEST(test_read_permissions);
  MU_RUN_TEST(test_write_permissions);
  MU_RUN_TEST(test_unix_permissions);
#endif
  MU_RUN_TEST(test_mtime);
}

#define UNUSED(x) (void)x

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  MU_RUN_SUITE(test_permissions_suite);
  MU_REPORT();
  return MU_EXIT_CODE;
}
