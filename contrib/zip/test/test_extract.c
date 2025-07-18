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

#define TESTDATA1 "Some test data 1...\0"
#define TESTDATA2 "Some test data 2...\0"

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

void test_teardown(void) {
  UNLINK("test/test-1.txt");
  UNLINK("test/test-2.txt");
  UNLINK("test/empty");
  UNLINK("test");
  UNLINK("empty");
  UNLINK("dotfiles/.test");
  UNLINK("dotfiles");
  UNLINK(ZIPNAME);
}

#define UNUSED(x) (void)x

struct buffer_t {
  char *data;
  size_t size;
};

static size_t on_extract(void *arg, uint64_t offset, const void *data,
                         size_t size) {
  UNUSED(offset);

  struct buffer_t *buf = (struct buffer_t *)arg;
  buf->data = realloc(buf->data, buf->size + size + 1);

  memcpy(&(buf->data[buf->size]), data, size);
  buf->size += size;
  buf->data[buf->size] = 0;

  return size;
}

MU_TEST(test_extract) {
  struct buffer_t buf;

  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  memset((void *)&buf, 0, sizeof(struct buffer_t));

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-1.txt"));
  mu_assert_int_eq(0, zip_entry_extract(zip, on_extract, &buf));
  mu_assert_int_eq(strlen(TESTDATA1), buf.size);
  mu_assert_int_eq(0, strncmp(buf.data, TESTDATA1, buf.size));
  mu_assert_int_eq(0, zip_entry_close(zip));

  free(buf.data);
  buf.data = NULL;
  buf.size = 0;

  memset((void *)&buf, 0, sizeof(struct buffer_t));

  mu_assert_int_eq(0, zip_entry_open(zip, "dotfiles/.test"));
  mu_assert_int_eq(0, zip_entry_extract(zip, on_extract, &buf));
  mu_assert_int_eq(strlen(TESTDATA2), buf.size);
  mu_assert_int_eq(0, strncmp(buf.data, TESTDATA2, buf.size));
  mu_assert_int_eq(0, zip_entry_close(zip));

  free(buf.data);
  buf.data = NULL;
  buf.size = 0;

  zip_close(zip);
}

MU_TEST(test_extract_stream) {
  mu_assert_int_eq(
      ZIP_ENOINIT,
      zip_extract("non_existing_directory/non_existing_archive.zip", ".", NULL,
                  NULL));
  mu_assert_int_eq(ZIP_ENOINIT, zip_stream_extract("", 0, ".", NULL, NULL));
  fprintf(stdout, "zip_stream_extract: %s\n", zip_strerror(ZIP_ENOINIT));

  FILE *fp = NULL;
#if defined(_MSC_VER)
  if (0 != fopen_s(&fp, ZIPNAME, "rb+"))
#else
  if (!(fp = fopen(ZIPNAME, "rb+")))
#endif
  {
    mu_fail("Cannot open filename\n");
  }

  fseek(fp, 0L, SEEK_END);
  size_t filesize = ftell(fp);
  fseek(fp, 0L, SEEK_SET);

  char *stream = (char *)malloc(filesize * sizeof(char));
  memset(stream, 0, filesize);

  size_t size = fread(stream, sizeof(char), filesize, fp);
  mu_assert_int_eq(filesize, size);

  mu_assert_int_eq(0, zip_stream_extract(stream, size, ".", NULL, NULL));

  free(stream);
  fclose(fp);
}

MU_TEST_SUITE(test_extract_suite) {
  MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

  MU_RUN_TEST(test_extract);
  MU_RUN_TEST(test_extract_stream);
}

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  MU_RUN_SUITE(test_extract_suite);
  MU_REPORT();
  return MU_EXIT_CODE;
}
