/*
   The latest version of this library is available on GitHub;
   https://github.com/sheredom/ubench.h
*/

/*
   This is free and unencumbered software released into the public domain.

   Anyone is free to copy, modify, publish, use, compile, sell, or
   distribute this software, either in source code form or as a compiled
   binary, for any purpose, commercial or non-commercial, and by any
   means.

   In jurisdictions that recognize copyright laws, the author or authors
   of this software dedicate any and all copyright interest in the
   software to the public domain. We make this dedication for the benefit
   of the public at large and to the detriment of our heirs and
   successors. We intend this dedication to be an overt act of
   relinquishment in perpetuity of all present and future rights to this
   software under copyright law.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.

   For more information, please refer to <http://unlicense.org/>
*/

#ifndef SHEREDOM_UBENCH_H_INCLUDED
#define SHEREDOM_UBENCH_H_INCLUDED

#ifdef _MSC_VER
/*
   Disable warning about not inlining 'inline' functions.
   TODO: We'll fix this later by not using fprintf within our macros, and
   instead use snprintf to a realloc'ed buffer.
*/
#pragma warning(disable : 4710)

/*
   Disable warning about inlining functions that are not marked 'inline'.
   TODO: add a UBENCH_NOINLINE onto the macro generated functions to fix this.
*/
#pragma warning(disable : 4711)

/*
   Disable warning about replacing undefined preprocessor macro '__cplusplus' with
   0 emitted from microsofts own headers.
   See: https://developercommunity.visualstudio.com/t/issue-in-corecrth-header-results-in-an-undefined-m/433021
*/
#pragma warning(disable : 4668)

/*
    Disabled warning about dangerous use of section.
    section '.CRT$XCU' is reserved for C++ dynamic initialization. Manually 
    creating the section will interfere with C++ dynamic initialization and may lead to undefined behavior
*/
#if defined(_MSC_FULL_VER)
#if _MSC_FULL_VER >= 192930100 // this warning was introduced in Visual Studio 2019 version 16.11
#pragma warning(disable : 5247)
#pragma warning(disable : 5248)
#endif
#endif

#pragma warning(push, 1)
#endif

#if defined(__cplusplus)
#define UBENCH_C_FUNC extern "C"
#else
#define UBENCH_C_FUNC
#endif

#if defined(__cplusplus)
#define UBENCH_NULL NULL
#else
#define UBENCH_NULL 0
#endif

#if defined(_MSC_VER) && (_MSC_VER < 1920)
typedef __int64 ubench_int64_t;
typedef unsigned __int64 ubench_uint64_t;
#else
#include <stdint.h>
typedef int64_t ubench_int64_t;
typedef uint64_t ubench_uint64_t;
#endif

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#if defined(_MSC_VER)
typedef union {
  struct {
    unsigned long LowPart;
    long  HighPart;
  } DUMMYSTRUCTNAME;
  struct {
    unsigned long LowPart;
    long  HighPart;
  } u;
  ubench_int64_t QuadPart;
} ubench_large_integer;

UBENCH_C_FUNC __declspec(dllimport) int __stdcall QueryPerformanceCounter(ubench_large_integer *);
UBENCH_C_FUNC __declspec(dllimport) int __stdcall QueryPerformanceFrequency(ubench_large_integer *);
#elif defined(__linux__)

/*
   slightly obscure include here - we need to include glibc's features.h, but
   we don't want to just include a header that might not be defined for other
   c libraries like musl. Instead we include limits.h, which we know on all
   glibc distributions includes features.h
*/
#include <limits.h>

#if defined(__GLIBC__) && defined(__GLIBC_MINOR__)
#include <time.h>

#if ((2 < __GLIBC__) || ((2 == __GLIBC__) && (17 <= __GLIBC_MINOR__)))
/* glibc is version 2.17 or above, so we can just use clock_gettime */
#define UBENCH_USE_CLOCKGETTIME
#else
#include <sys/syscall.h>
#include <unistd.h>
#endif
#endif

#elif defined(__APPLE__)
#include <mach/mach_time.h>
#endif

#if defined(__cplusplus)
#define UBENCH_C_FUNC extern "C"
#else
#define UBENCH_C_FUNC
#endif

#if defined(__cplusplus) && (__cplusplus >= 201103L)
#define UBENCH_NOEXCEPT noexcept
#else
#define UBENCH_NOEXCEPT
#endif

#if defined(__cplusplus) && defined(_MSC_VER)
#define UBENCH_NOTHROW __declspec(nothrow)
#else
#define UBENCH_NOTHROW
#endif

#if defined(_MSC_VER) && (_MSC_VER < 1920)
#define UBENCH_PRId64 "I64d"
#define UBENCH_PRIu64 "I64u"
#else
#include <inttypes.h>

#define UBENCH_PRId64 PRId64
#define UBENCH_PRIu64 PRIu64
#endif

#if defined(_MSC_VER)
#define UBENCH_INLINE __forceinline
#define UBENCH_NOINLINE __declspec(noinline)

#if defined(_WIN64)
#define UBENCH_SYMBOL_PREFIX
#else
#define UBENCH_SYMBOL_PREFIX "_"
#endif

#if defined(__clang__)
#define UBENCH_INITIALIZER_BEGIN_DISABLE_WARNINGS                              \
  _Pragma("clang diagnostic push")                                             \
      _Pragma("clang diagnostic ignored \"-Wmissing-variable-declarations\"")

#define UBENCH_INITIALIZER_END_DISABLE_WARNINGS _Pragma("clang diagnostic pop")
#else
#define UBENCH_INITIALIZER_BEGIN_DISABLE_WARNINGS
#define UBENCH_INITIALIZER_END_DISABLE_WARNINGS
#endif

#pragma section(".CRT$XCU", read)
#define UBENCH_INITIALIZER(f)                                                  \
  static void __cdecl f(void);                                                 \
  UBENCH_INITIALIZER_BEGIN_DISABLE_WARNINGS __pragma(                          \
      comment(linker, "/include:" UBENCH_SYMBOL_PREFIX #f "_"))                \
      UBENCH_C_FUNC __declspec(allocate(".CRT$XCU")) void(__cdecl *            \
                                                          f##_)(void) = f;     \
  UBENCH_INITIALIZER_END_DISABLE_WARNINGS static void __cdecl f(void)
#else
#if defined(__linux__)
#if defined(__clang__)
#if __has_warning("-Wreserved-id-macro")
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#endif

#define __STDC_FORMAT_MACROS 1

#if defined(__clang__)
#if __has_warning("-Wreserved-id-macro")
#pragma clang diagnostic pop
#endif
#endif
#endif

#define UBENCH_INLINE inline
#define UBENCH_NOINLINE __attribute__((noinline))

#define UBENCH_INITIALIZER(f)                                                  \
  static void f(void) __attribute__((constructor));                            \
  static void f(void)
#endif

#if defined(__cplusplus)
#define UBENCH_CAST(type, x) static_cast<type>(x)
#define UBENCH_PTR_CAST(type, x) reinterpret_cast<type>(x)
#define UBENCH_EXTERN extern "C"
#else
#define UBENCH_CAST(type, x) ((type)(x))
#define UBENCH_PTR_CAST(type, x) ((type)(x))
#define UBENCH_EXTERN extern
#endif

#ifdef _MSC_VER
/*
    io.h contains definitions for some structures with natural padding. This is
    uninteresting, but for some reason MSVC's behaviour is to warn about
    including this system header. That *is* interesting
*/
#pragma warning(disable : 4820)
#pragma warning(push, 1)
#include <io.h>
#pragma warning(pop)
#define UBENCH_COLOUR_OUTPUT() (_isatty(_fileno(stdout)))
#else
#include <unistd.h>
#define UBENCH_COLOUR_OUTPUT() (isatty(STDOUT_FILENO))
#endif

static UBENCH_INLINE ubench_int64_t ubench_ns(void) {
#ifdef _MSC_VER
  ubench_large_integer counter;
  ubench_large_integer frequency;
  QueryPerformanceCounter(&counter);
  QueryPerformanceFrequency(&frequency);
  return UBENCH_CAST(ubench_int64_t,
                     (counter.QuadPart * 1000000000) / frequency.QuadPart);
#elif defined(__linux)
  struct timespec ts;
  const clockid_t cid = CLOCK_REALTIME;
#if defined(UBENCH_USE_CLOCKGETTIME)
  clock_gettime(cid, &ts);
#else
  syscall(SYS_clock_gettime, cid, &ts);
#endif
  return UBENCH_CAST(ubench_int64_t, ts.tv_sec) * 1000 * 1000 * 1000 +
         ts.tv_nsec;
#elif __APPLE__
  return UBENCH_CAST(ubench_int64_t, mach_absolute_time());
#endif
}

struct ubench_run_state_s {
  ubench_int64_t* ns;
  ubench_int64_t  size;
  ubench_int64_t  sample;
};

typedef void (*ubench_benchmark_t)(struct ubench_run_state_s* ubs);

struct ubench_benchmark_state_s {
  ubench_benchmark_t func;
  char *name;
};

struct ubench_state_s {
  struct ubench_benchmark_state_s *benchmarks;
  size_t benchmarks_length;
  FILE *output;
  double confidence;
};

/* extern to the global state ubench needs to execute */
UBENCH_EXTERN struct ubench_state_s ubench_state;

#if defined(_MSC_VER)
#define UBENCH_UNUSED
#else
#define UBENCH_UNUSED __attribute__((unused))
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvariadic-macros"
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#endif
#define UBENCH_PRINTF(...)                                                     \
  if (ubench_state.output) {                                                   \
    fprintf(ubench_state.output, __VA_ARGS__);                                 \
  }                                                                            \
  printf(__VA_ARGS__)
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvariadic-macros"
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#endif

#ifdef _MSC_VER
#define UBENCH_SNPRINTF(BUFFER, N, ...) _snprintf_s(BUFFER, N, N, __VA_ARGS__)
#else
#define UBENCH_SNPRINTF(...) snprintf(__VA_ARGS__)
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

static UBENCH_INLINE int ubench_do_benchmark(struct ubench_run_state_s* ubs)
{
  ubench_int64_t curr_sample = ubs->sample++;
  ubs->ns[curr_sample] = ubench_ns();
  return curr_sample < ubs->size ? 1 : 0;
}

#define UBENCH_DO_BENCHMARK()                                                  \
  while(ubench_do_benchmark(ubench_run_state) > 0)

#define UBENCH_EX(SET, NAME)                                                   \
  UBENCH_EXTERN struct ubench_state_s ubench_state;                            \
  static void ubench_##SET##_##NAME(struct ubench_run_state_s* ubs);           \
  UBENCH_INITIALIZER(ubench_register_##SET##_##NAME) {                         \
    const size_t index = ubench_state.benchmarks_length++;                     \
    const char *name_part = #SET "." #NAME;                                    \
    const size_t name_size = strlen(name_part) + 1;                            \
    char *name = UBENCH_PTR_CAST(char *, malloc(name_size));                   \
    ubench_state.benchmarks = UBENCH_PTR_CAST(                                 \
        struct ubench_benchmark_state_s *,                                     \
        realloc(UBENCH_PTR_CAST(void *, ubench_state.benchmarks),              \
                sizeof(struct ubench_benchmark_state_s) *                      \
                    ubench_state.benchmarks_length));                          \
    ubench_state.benchmarks[index].func = &ubench_##SET##_##NAME;              \
    ubench_state.benchmarks[index].name = name;                                \
    UBENCH_SNPRINTF(name, name_size, "%s", name_part);                         \
  }                                                                            \
  void ubench_##SET##_##NAME(struct ubench_run_state_s* ubench_run_state)

#define UBENCH(SET, NAME)                                                      \
  static void ubench_run_##SET##_##NAME(void);                                 \
  UBENCH_EX(SET, NAME) {                                                       \
    UBENCH_DO_BENCHMARK() {                                                    \
      ubench_run_##SET##_##NAME();                                             \
    }                                                                          \
  }                                                                            \
  void ubench_run_##SET##_##NAME(void)

#define UBENCH_F_SETUP(FIXTURE)                                                \
  static void ubench_f_setup_##FIXTURE(struct FIXTURE *ubench_fixture)

#define UBENCH_F_TEARDOWN(FIXTURE)                                             \
  static void ubench_f_teardown_##FIXTURE(struct FIXTURE *ubench_fixture)

#define UBENCH_EX_F(FIXTURE, NAME)                                             \
  UBENCH_EXTERN struct ubench_state_s ubench_state;                            \
  static void ubench_f_setup_##FIXTURE(struct FIXTURE *);                      \
  static void ubench_f_teardown_##FIXTURE(struct FIXTURE *);                   \
  static void ubench_run_ex_##FIXTURE##_##NAME(struct FIXTURE *,               \
                                            struct ubench_run_state_s*);       \
  static void ubench_f_##FIXTURE##_##NAME(struct ubench_run_state_s* ubench_run_state) { \
    struct FIXTURE fixture;                                                    \
    memset(&fixture, 0, sizeof(fixture));                                      \
    ubench_f_setup_##FIXTURE(&fixture);                                        \
    ubench_run_ex_##FIXTURE##_##NAME(&fixture, ubench_run_state);              \
    ubench_f_teardown_##FIXTURE(&fixture);                                     \
  }                                                                            \
  UBENCH_INITIALIZER(ubench_register_##FIXTURE##_##NAME) {                     \
    const size_t index = ubench_state.benchmarks_length++;                     \
    const char *name_part = #FIXTURE "." #NAME;                                \
    const size_t name_size = strlen(name_part) + 1;                            \
    char *name = UBENCH_PTR_CAST(char *, malloc(name_size));                   \
    ubench_state.benchmarks = UBENCH_PTR_CAST(                                 \
        struct ubench_benchmark_state_s *,                                     \
        realloc(UBENCH_PTR_CAST(void *, ubench_state.benchmarks),              \
                sizeof(struct ubench_benchmark_state_s) *                      \
                    ubench_state.benchmarks_length));                          \
    ubench_state.benchmarks[index].func = &ubench_f_##FIXTURE##_##NAME;        \
    ubench_state.benchmarks[index].name = name;                                \
    UBENCH_SNPRINTF(name, name_size, "%s", name_part);                         \
  }                                                                            \
  void ubench_run_ex_##FIXTURE##_##NAME(struct FIXTURE *ubench_fixture,        \
                                        struct ubench_run_state_s* ubench_run_state)

#define UBENCH_F(FIXTURE, NAME)                                                \
  static void ubench_run_##FIXTURE##_##NAME(struct FIXTURE *);                 \
  UBENCH_EX_F(FIXTURE, NAME) {                                                 \
    UBENCH_DO_BENCHMARK() {                                                    \
       ubench_run_##FIXTURE##_##NAME(ubench_fixture);                          \
     }                                                                         \
  }                                                                            \
  void ubench_run_##FIXTURE##_##NAME(struct FIXTURE *ubench_fixture)

static UBENCH_INLINE int ubench_should_filter(const char *filter,
                                              const char *benchmark);
int ubench_should_filter(const char *filter, const char *benchmark) {
  if (filter) {
    const char *filter_cur = filter;
    const char *benchmark_cur = benchmark;
    const char *filter_wildcard = UBENCH_NULL;

    while (('\0' != *filter_cur) && ('\0' != *benchmark_cur)) {
      if ('*' == *filter_cur) {
        /* store the position of the wildcard */
        filter_wildcard = filter_cur;

        /* skip the wildcard character */
        filter_cur++;

        while (('\0' != *filter_cur) && ('\0' != *benchmark_cur)) {
          if ('*' == *filter_cur) {
            /*
               we found another wildcard (filter is something like *foo*) so we
               exit the current loop, and return to the parent loop to handle
               the wildcard case
            */
            break;
          } else if (*filter_cur != *benchmark_cur) {
            /* otherwise our filter didn't match, so reset it */
            filter_cur = filter_wildcard;
          }

          /* move benchmark along */
          benchmark_cur++;

          /* move filter along */
          filter_cur++;
        }

        if (('\0' == *filter_cur) && ('\0' == *benchmark_cur)) {
          return 0;
        }

        /* if the benchmarks have been exhausted, we don't have a match! */
        if ('\0' == *benchmark_cur) {
          return 1;
        }
      } else {
        if (*benchmark_cur != *filter_cur) {
          /* benchmark doesn't match filter */
          return 1;
        } else {
          /* move our filter and benchmark forward */
          benchmark_cur++;
          filter_cur++;
        }
      }
    }

    if (('\0' != *filter_cur) ||
        (('\0' != *benchmark_cur) &&
         ((filter == filter_cur) || ('*' != filter_cur[-1])))) {
      /* we have a mismatch! */
      return 1;
    }
  }

  return 0;
}

static UBENCH_INLINE int ubench_strncmp(const char *a, const char *b,
                                        size_t n) {
  /* strncmp breaks on Wall / Werror on gcc/clang, so we avoid using it */
  unsigned i;

  for (i = 0; i < n; i++) {
    if (a[i] < b[i]) {
      return -1;
    } else if (a[i] > b[i]) {
      return 1;
    }
  }

  return 0;
}

static UBENCH_INLINE FILE *ubench_fopen(const char *filename,
                                        const char *mode) {
#ifdef _MSC_VER
  FILE *file;
  if (0 == fopen_s(&file, filename, mode)) {
    return file;
  } else {
    return UBENCH_NULL;
  }
#else
  return fopen(filename, mode);
#endif
}

static UBENCH_INLINE int ubench_main(int argc, const char *const argv[]);
int ubench_main(int argc, const char *const argv[]) {
  ubench_uint64_t failed = 0;
  size_t index = 0;
  size_t *failed_benchmarks = UBENCH_NULL;
  size_t failed_benchmarks_length = 0;
  const char *filter = UBENCH_NULL;
  ubench_uint64_t ran_benchmarks = 0;

  enum colours { RESET, GREEN, RED };

  const int use_colours = UBENCH_COLOUR_OUTPUT();
  const char *colours[] = {"\033[0m", "\033[32m", "\033[31m"};
  if (!use_colours) {
    for (index = 0; index < sizeof colours / sizeof colours[0]; index++) {
      colours[index] = "";
    }
  }

  /* loop through all arguments looking for our options */
  for (index = 1; index < UBENCH_CAST(size_t, argc); index++) {
    /* Informational switches */
    const char help_str[] = "--help";
    const char list_str[] = "--list-benchmarks";
    /* Benchmark config switches */
    const char filter_str[] = "--filter=";
    const char output_str[] = "--output=";
    const char confidence_str[] = "--confidence=";

    if (0 == ubench_strncmp(argv[index], help_str, strlen(help_str))) {
      printf("ubench.h - the single file benchmarking solution for C/C++!\n"
             "Command line Options:\n");
      printf("  --help                    Show this message and exit.\n"
             "  --filter=<filter>         Filter the benchmarks to run (EG. "
             "MyBench*.a would run MyBenchmark.a but not MyBenchmark.b).\n"
             "  --list-benchmarks         List benchmarks, one per line. "
             "Output names can be passed to --filter.\n"
             "  --output=<output>         Output a CSV file of the results.\n"
             "  --confidence=<confidence> Change the confidence cut-off for a "
             "failed test. Defaults to 2.5%%\n");
      goto cleanup;
    } else if (0 ==
               ubench_strncmp(argv[index], filter_str, strlen(filter_str))) {
      /* user wants to filter what benchmarks run! */
      filter = argv[index] + strlen(filter_str);
    } else if (0 ==
               ubench_strncmp(argv[index], output_str, strlen(output_str))) {
      ubench_state.output =
          ubench_fopen(argv[index] + strlen(output_str), "w+");
    } else if (0 == ubench_strncmp(argv[index], list_str, strlen(list_str))) {
      for (index = 0; index < ubench_state.benchmarks_length; index++) {
        UBENCH_PRINTF("%s\n", ubench_state.benchmarks[index].name);
      }

      /* when printing the benchmark list, don't actually run the benchmarks */
      goto cleanup;
    } else if (0 == ubench_strncmp(argv[index], confidence_str,
                                   strlen(confidence_str))) {
      /* user wants to specify a different confidence */
      ubench_state.confidence = atof(argv[index] + strlen(confidence_str));

      /* must be between 0 and 100 */
      if ((ubench_state.confidence < 0) || (ubench_state.confidence > 100)) {
        fprintf(stderr,
                "Confidence must be in the range [0..100] (you specified %f)\n",
                ubench_state.confidence);
        goto cleanup;
      }
    }
  }

  for (index = 0; index < ubench_state.benchmarks_length; index++) {
    if (ubench_should_filter(filter, ubench_state.benchmarks[index].name)) {
      continue;
    }

    ran_benchmarks++;
  }

  printf("%s[==========]%s Running %" UBENCH_PRIu64 " benchmarks.\n",
         colours[GREEN], colours[RESET],
         UBENCH_CAST(ubench_uint64_t, ran_benchmarks));

  if (ubench_state.output) {
    fprintf(ubench_state.output,
            "name, mean (ns), stddev (%%), confidence (%%)\n");
  }

  for (index = 0; index < ubench_state.benchmarks_length; index++) {
    int result = 1;
    size_t mndex = 0;
    ubench_int64_t best_avg_ns = 0;
    double best_deviation = 0;
    double best_confidence = 101.0;
    struct ubench_run_state_s ubs;

#define UBENCH_MIN_ITERATIONS 10
#define UBENCH_MAX_ITERATIONS 500
    ubench_int64_t iterations = 10;
    const ubench_int64_t max_iterations = UBENCH_MAX_ITERATIONS;
    const ubench_int64_t min_iterations = UBENCH_MIN_ITERATIONS;
    /* Add one extra timestamp slot, as we save times between runs and time after exiting the last one */
    ubench_int64_t ns[UBENCH_MAX_ITERATIONS+1];
#undef UBENCH_MAX_ITERATIONS
#undef UBENCH_MIN_ITERATIONS

    if (ubench_should_filter(filter, ubench_state.benchmarks[index].name)) {
      continue;
    }

    printf("%s[ RUN      ]%s %s\n", colours[GREEN], colours[RESET],
           ubench_state.benchmarks[index].name);

    ubs.ns     = ns;
    ubs.size   = 1;
    ubs.sample = 0;

    /* Time once to work out the base number of iterations to use. */
    ubench_state.benchmarks[index].func(&ubs);

    iterations = (100 * 1000 * 1000) / ((ns[1] <= ns[0]) ? 1 : ns[1] - ns[0]);
    iterations = iterations < min_iterations ? min_iterations : iterations;
    iterations = iterations > max_iterations ? max_iterations : iterations;

    for (mndex = 0; (mndex < 100) && (result != 0); mndex++) {
      ubench_int64_t kndex = 0;
      ubench_int64_t avg_ns = 0;
      double deviation = 0;
      double confidence = 0;

      iterations = iterations * (UBENCH_CAST(ubench_int64_t, mndex) + 1);
      iterations = iterations > max_iterations ? max_iterations : iterations;

      ubs.sample = 0;
      ubs.size   = iterations;
      ubench_state.benchmarks[index].func(&ubs);

      /* Calculate benchmark run-times */
      for (kndex = 0; kndex < iterations; kndex++) {
        ns[kndex] = ns[kndex + 1] - ns[kndex];
      }

      for (kndex = 0; kndex < iterations; kndex++) {
        avg_ns += ns[kndex];
      }

      avg_ns /= iterations;

      for (kndex = 0; kndex < iterations; kndex++) {
        const double v = UBENCH_CAST(double, ns[kndex] - avg_ns);
        deviation += v * v;
      }

      deviation = sqrt(deviation / UBENCH_CAST(double, iterations));

      /* Confidence is the 99% confidence index - whose magic value is 2.576. */
      confidence = 2.576 * deviation / sqrt(UBENCH_CAST(double, iterations));
      confidence = (confidence / UBENCH_CAST(double, avg_ns)) * 100.0;

      deviation = (deviation / UBENCH_CAST(double, avg_ns)) * 100.0;

      /* If we've found a more confident solution, use that. */
      result = confidence > ubench_state.confidence;

      /* If the deviation beats our previous best, record it. */
      if (confidence < best_confidence) {
        best_avg_ns = avg_ns;
        best_deviation = deviation;
        best_confidence = confidence;
      }
    }

    if (result) {
      printf("confidence interval %f%% exceeds maximum permitted %f%%\n",
             best_confidence, ubench_state.confidence);
    }

    if (ubench_state.output) {
      fprintf(ubench_state.output, "%s, %" UBENCH_PRId64 ", %f, %f,\n",
              ubench_state.benchmarks[index].name, best_avg_ns, best_deviation,
              best_confidence);
    }

    {
      const char *const colour = (0 != result) ? colours[RED] : colours[GREEN];
      const char *const status =
          (0 != result) ? "[  FAILED  ]" : "[       OK ]";
      const char *unit = "us";

      if (0 != result) {
        const size_t failed_benchmark_index = failed_benchmarks_length++;
        failed_benchmarks = UBENCH_PTR_CAST(
            size_t *, realloc(UBENCH_PTR_CAST(void *, failed_benchmarks),
                              sizeof(size_t) * failed_benchmarks_length));
        failed_benchmarks[failed_benchmark_index] = index;
        failed++;
      }

      printf("%s%s%s %s (mean ", colour, status, colours[RESET],
             ubench_state.benchmarks[index].name);

      for (mndex = 0; mndex < 2; mndex++) {
        if (best_avg_ns <= 1000000) {
          break;
        }

        /* If the average is greater than a million, we reduce it and change the
        unit we report. */
        best_avg_ns /= 1000;

        switch (mndex) {
        case 0:
          unit = "ms";
          break;
        case 1:
          unit = "s";
          break;
        }
      }

      printf("%" UBENCH_PRId64 ".%03" UBENCH_PRId64
             "%s, confidence interval +- %f%%)\n",
             best_avg_ns / 1000, best_avg_ns % 1000, unit, best_confidence);
    }
  }

  printf("%s[==========]%s %" UBENCH_PRIu64 " benchmarks ran.\n",
         colours[GREEN], colours[RESET], ran_benchmarks);
  printf("%s[  PASSED  ]%s %" UBENCH_PRIu64 " benchmarks.\n", colours[GREEN],
         colours[RESET], ran_benchmarks - failed);

  if (0 != failed) {
    printf("%s[  FAILED  ]%s %" UBENCH_PRIu64 " benchmarks, listed below:\n",
           colours[RED], colours[RESET], failed);
    for (index = 0; index < failed_benchmarks_length; index++) {
      printf("%s[  FAILED  ]%s %s\n", colours[RED], colours[RESET],
             ubench_state.benchmarks[failed_benchmarks[index]].name);
    }
  }

cleanup:
  for (index = 0; index < ubench_state.benchmarks_length; index++) {
    free(UBENCH_PTR_CAST(void *, ubench_state.benchmarks[index].name));
  }

  free(UBENCH_PTR_CAST(void *, failed_benchmarks));
  free(UBENCH_PTR_CAST(void *, ubench_state.benchmarks));

  if (ubench_state.output) {
    fclose(ubench_state.output);
  }

  return UBENCH_CAST(int, failed);
}

UBENCH_C_FUNC UBENCH_NOINLINE void ubench_do_nothing(void *const);

#define UBENCH_DO_NOTHING(x) ubench_do_nothing(x)

#if defined(_MSC_VER)
UBENCH_C_FUNC void _ReadWriteBarrier(void);

#define UBENCH_DECLARE_DO_NOTHING()                                            \
  void ubench_do_nothing(void *ptr) {                                          \
    (void)ptr;                                                                 \
    _ReadWriteBarrier();                                                       \
  }
#elif defined(__clang__)
#define UBENCH_DECLARE_DO_NOTHING()                                            \
  void ubench_do_nothing(void *ptr) {                                          \
    _Pragma("clang diagnostic push")                                           \
        _Pragma("clang diagnostic ignored \"-Wlanguage-extension-token\"");    \
    asm volatile("" : : "r,m"(ptr) : "memory");                                \
    _Pragma("clang diagnostic pop");                                           \
  }
#else
#define UBENCH_DECLARE_DO_NOTHING()                                            \
  void ubench_do_nothing(void *ptr) {                                          \
    asm volatile("" : : "r,m"(ptr) : "memory");                                \
  }
#endif

/*
   We need, in exactly one source file, define the global struct that will hold
   the data we need to run ubench. This macro allows the user to declare the
   data without having to use the UBENCH_MAIN macro, thus allowing them to write
   their own main() function.

   We also use this to define the 'do nothing' method that lets us keep data
   that the compiler would normally deem is dead for the purposes of timing.
*/
#define UBENCH_STATE()                                                         \
  UBENCH_DECLARE_DO_NOTHING()                                                  \
  struct ubench_state_s ubench_state = {0, 0, 0, 2.5}

/*
   define a main() function to call into ubench.h and start executing
   benchmarks! A user can optionally not use this macro, and instead define
   their own main() function and manually call ubench_main. The user must, in
   exactly one source file, use the UBENCH_STATE macro to declare a global
   struct variable that ubench requires.
*/
#define UBENCH_MAIN()                                                          \
  UBENCH_STATE();                                                              \
  int main(int argc, const char *const argv[]) {                               \
    return ubench_main(argc, argv);                                            \
  }

#endif /* SHEREDOM_UBENCH_H_INCLUDED */
