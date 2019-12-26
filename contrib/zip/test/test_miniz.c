// Demonstrates miniz.c's compress() and uncompress() functions
// (same as zlib's). Public domain, May 15 2011, Rich Geldreich,
// richgel99@gmail.com. See "unlicense" statement at the end of tinfl.c.

#include <miniz.h>
#include <stdio.h>

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint;

// The string to compress.
static const char *s_pStr =
    "Good morning Dr. Chandra. This is Hal. I am ready for my first lesson."
    "Good morning Dr. Chandra. This is Hal. I am ready for my first lesson."
    "Good morning Dr. Chandra. This is Hal. I am ready for my first lesson."
    "Good morning Dr. Chandra. This is Hal. I am ready for my first lesson."
    "Good morning Dr. Chandra. This is Hal. I am ready for my first lesson."
    "Good morning Dr. Chandra. This is Hal. I am ready for my first lesson."
    "Good morning Dr. Chandra. This is Hal. I am ready for my first lesson.";

int main(int argc, char *argv[]) {
  uint step = 0;
  int cmp_status;
  uLong src_len = (uLong)strlen(s_pStr);
  uLong uncomp_len = src_len;
  uLong cmp_len;
  uint8 *pCmp, *pUncomp;
  size_t sz;
  uint total_succeeded = 0;
  (void)argc, (void)argv;

  printf("miniz.c version: %s\n", MZ_VERSION);

  do {
    pCmp = (uint8 *)tdefl_compress_mem_to_heap(s_pStr, src_len, &cmp_len, 0);
    if (!pCmp) {
      printf("tdefl_compress_mem_to_heap failed\n");
      return EXIT_FAILURE;
    }
    if (src_len <= cmp_len) {
      printf("tdefl_compress_mem_to_heap failed: from %u to %u bytes\n",
             (mz_uint32)uncomp_len, (mz_uint32)cmp_len);
      free(pCmp);
      return EXIT_FAILURE;
    }

    sz = tdefl_compress_mem_to_mem(pCmp, cmp_len, s_pStr, src_len, 0);
    if (sz != cmp_len) {
      printf("tdefl_compress_mem_to_mem failed: expected %u, got %u\n",
             (mz_uint32)cmp_len, (mz_uint32)sz);
      free(pCmp);
      return EXIT_FAILURE;
    }

    // Allocate buffers to hold compressed and uncompressed data.
    free(pCmp);
    cmp_len = compressBound(src_len);
    pCmp = (mz_uint8 *)malloc((size_t)cmp_len);
    pUncomp = (mz_uint8 *)malloc((size_t)src_len);
    if ((!pCmp) || (!pUncomp)) {
      printf("Out of memory!\n");
      return EXIT_FAILURE;
    }

    // Compress the string.
    cmp_status =
        compress(pCmp, &cmp_len, (const unsigned char *)s_pStr, src_len);
    if (cmp_status != Z_OK) {
      printf("compress() failed!\n");
      free(pCmp);
      free(pUncomp);
      return EXIT_FAILURE;
    }

    printf("Compressed from %u to %u bytes\n", (mz_uint32)src_len,
           (mz_uint32)cmp_len);

    if (step) {
      // Purposely corrupt the compressed data if fuzzy testing (this is a
      // very crude fuzzy test).
      uint n = 1 + (rand() % 3);
      while (n--) {
        uint i = rand() % cmp_len;
        pCmp[i] ^= (rand() & 0xFF);
      }
    }

    // Decompress.
    cmp_status = uncompress(pUncomp, &uncomp_len, pCmp, cmp_len);
    total_succeeded += (cmp_status == Z_OK);

    if (step) {
      printf("Simple fuzzy test: step %u total_succeeded: %u\n", step,
             total_succeeded);
    } else {
      if (cmp_status != Z_OK) {
        printf("uncompress failed!\n");
        free(pCmp);
        free(pUncomp);
        return EXIT_FAILURE;
      }

      printf("Decompressed from %u to %u bytes\n", (mz_uint32)cmp_len,
             (mz_uint32)uncomp_len);

      // Ensure uncompress() returned the expected data.
      if ((uncomp_len != src_len) ||
          (memcmp(pUncomp, s_pStr, (size_t)src_len))) {
        printf("Decompression failed!\n");
        free(pCmp);
        free(pUncomp);
        return EXIT_FAILURE;
      }
    }

    free(pCmp);
    free(pUncomp);

    step++;

    // Keep on fuzzy testing if there's a non-empty command line.
  } while (argc >= 2);

  printf("Success.\n");
  return EXIT_SUCCESS;
}
