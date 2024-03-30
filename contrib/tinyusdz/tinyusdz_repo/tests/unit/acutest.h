/*
 * Acutest -- Another C/C++ Unit Test facility
 * <https://github.com/mity/acutest>
 *
 * Copyright 2013-2020 Martin Mitas
 * Copyright 2019 Garrett D'Amore
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef ACUTEST_H
#define ACUTEST_H


/************************
 *** Public interface ***
 ************************/

/* By default, "acutest.h" provides the main program entry point (function
 * main()). However, if the test suite is composed of multiple source files
 * which include "acutest.h", then this causes a problem of multiple main()
 * definitions. To avoid this problem, #define macro TEST_NO_MAIN in all
 * compilation units but one.
 */

/* Macro to specify list of unit tests in the suite.
 * The unit test implementation MUST provide list of unit tests it implements
 * with this macro:
 *
 *   TEST_LIST = {
 *       { "test1_name", test1_func_ptr },
 *       { "test2_name", test2_func_ptr },
 *       ...
 *       { NULL, NULL }     // zeroed record marking the end of the list
 *   };
 *
 * The list specifies names of each test (must be unique) and pointer to
 * a function implementing it. The function does not take any arguments
 * and has no return values, i.e. every test function has to be compatible
 * with this prototype:
 *
 *   void test_func(void);
 *
 * Note the list has to be ended with a zeroed record.
 */
#define TEST_LIST               const struct acutest_test_ acutest_list_[]


/* Macros for testing whether an unit test succeeds or fails. These macros
 * can be used arbitrarily in functions implementing the unit tests.
 *
 * If any condition fails throughout execution of a test, the test fails.
 *
 * TEST_CHECK takes only one argument (the condition), TEST_CHECK_ allows
 * also to specify an error message to print out if the condition fails.
 * (It expects printf-like format string and its parameters). The macros
 * return non-zero (condition passes) or 0 (condition fails).
 *
 * That can be useful when more conditions should be checked only if some
 * preceding condition passes, as illustrated in this code snippet:
 *
 *   SomeStruct* ptr = allocate_some_struct();
 *   if(TEST_CHECK(ptr != NULL)) {
 *       TEST_CHECK(ptr->member1 < 100);
 *       TEST_CHECK(ptr->member2 > 200);
 *   }
 */
#define TEST_CHECK_(cond,...)   acutest_check_((cond), __FILE__, __LINE__, __VA_ARGS__)
#define TEST_CHECK(cond)        acutest_check_((cond), __FILE__, __LINE__, "%s", #cond)


/* These macros are the same as TEST_CHECK_ and TEST_CHECK except that if the
 * condition fails, the currently executed unit test is immediately aborted.
 *
 * That is done either by calling abort() if the unit test is executed as a
 * child process; or via longjmp() if the unit test is executed within the
 * main Acutest process.
 *
 * As a side effect of such abortion, your unit tests may cause memory leaks,
 * unflushed file descriptors, and other phenomena caused by the abortion.
 *
 * Therefore you should not use these as a general replacement for TEST_CHECK.
 * Use it with some caution, especially if your test causes some other side
 * effects to the outside world (e.g. communicating with some server, inserting
 * into a database etc.).
 */
#define TEST_ASSERT_(cond,...)                                                 \
    do {                                                                       \
        if(!acutest_check_((cond), __FILE__, __LINE__, __VA_ARGS__))           \
            acutest_abort_();                                                  \
    } while(0)
#define TEST_ASSERT(cond)                                                      \
    do {                                                                       \
        if(!acutest_check_((cond), __FILE__, __LINE__, "%s", #cond))           \
            acutest_abort_();                                                  \
    } while(0)


#ifdef __cplusplus
/* TinyUSDZ: Disable exception */

#if 0
/* Macros to verify that the code (the 1st argument) throws exception of given
 * type (the 2nd argument). (Note these macros are only available in C++.)
 *
 * TEST_EXCEPTION_ is like TEST_EXCEPTION but accepts custom printf-like
 * message.
 *
 * For example:
 *
 *   TEST_EXCEPTION(function_that_throw(), ExpectedExceptionType);
 *
 * If the function_that_throw() throws ExpectedExceptionType, the check passes.
 * If the function throws anything incompatible with ExpectedExceptionType
 * (or if it does not thrown an exception at all), the check fails.
 */
#define TEST_EXCEPTION(code, exctype)                                          \
    do {                                                                       \
        bool exc_ok_ = false;                                                  \
        const char *msg_ = NULL;                                               \
        try {                                                                  \
            code;                                                              \
            msg_ = "No exception thrown.";                                     \
        } catch(exctype const&) {                                              \
            exc_ok_= true;                                                     \
        } catch(...) {                                                         \
            msg_ = "Unexpected exception thrown.";                             \
        }                                                                      \
        acutest_check_(exc_ok_, __FILE__, __LINE__, #code " throws " #exctype);\
        if(msg_ != NULL)                                                       \
            acutest_message_("%s", msg_);                                      \
    } while(0)
#define TEST_EXCEPTION_(code, exctype, ...)                                    \
    do {                                                                       \
        bool exc_ok_ = false;                                                  \
        const char *msg_ = NULL;                                               \
        try {                                                                  \
            code;                                                              \
            msg_ = "No exception thrown.";                                     \
        } catch(exctype const&) {                                              \
            exc_ok_= true;                                                     \
        } catch(...) {                                                         \
            msg_ = "Unexpected exception thrown.";                             \
        }                                                                      \
        acutest_check_(exc_ok_, __FILE__, __LINE__, __VA_ARGS__);              \
        if(msg_ != NULL)                                                       \
            acutest_message_("%s", msg_);                                      \
    } while(0)

#endif

#endif  /* #ifdef __cplusplus */


/* Sometimes it is useful to split execution of more complex unit tests to some
 * smaller parts and associate those parts with some names.
 *
 * This is especially handy if the given unit test is implemented as a loop
 * over some vector of multiple testing inputs. Using these macros allow to use
 * sort of subtitle for each iteration of the loop (e.g. outputting the input
 * itself or a name associated to it), so that if any TEST_CHECK condition
 * fails in the loop, it can be easily seen which iteration triggers the
 * failure, without the need to manually output the iteration-specific data in
 * every single TEST_CHECK inside the loop body.
 *
 * TEST_CASE allows to specify only single string as the name of the case,
 * TEST_CASE_ provides all the power of printf-like string formatting.
 *
 * Note that the test cases cannot be nested. Starting a new test case ends
 * implicitly the previous one. To end the test case explicitly (e.g. to end
 * the last test case after exiting the loop), you may use TEST_CASE(NULL).
 */
#define TEST_CASE_(...)         acutest_case_(__VA_ARGS__)
#define TEST_CASE(name)         acutest_case_("%s", name)


/* Maximal output per TEST_CASE call. Longer messages are cut.
 * You may define another limit prior including "acutest.h"
 */
#ifndef TEST_CASE_MAXSIZE
    #define TEST_CASE_MAXSIZE    64
#endif


/* printf-like macro for outputting an extra information about a failure.
 *
 * Intended use is to output some computed output versus the expected value,
 * e.g. like this:
 *
 *   if(!TEST_CHECK(produced == expected)) {
 *       TEST_MSG("Expected: %d", expected);
 *       TEST_MSG("Produced: %d", produced);
 *   }
 *
 * Note the message is only written down if the most recent use of any checking
 * macro (like e.g. TEST_CHECK or TEST_EXCEPTION) in the current test failed.
 * This means the above is equivalent to just this:
 *
 *   TEST_CHECK(produced == expected);
 *   TEST_MSG("Expected: %d", expected);
 *   TEST_MSG("Produced: %d", produced);
 *
 * The macro can deal with multi-line output fairly well. It also automatically
 * adds a final new-line if there is none present.
 */
#define TEST_MSG(...)           acutest_message_(__VA_ARGS__)


/* Maximal output per TEST_MSG call. Longer messages are cut.
 * You may define another limit prior including "acutest.h"
 */
#ifndef TEST_MSG_MAXSIZE
    #define TEST_MSG_MAXSIZE    1024
#endif


/* Macro for dumping a block of memory.
 *
 * Its intended use is very similar to what TEST_MSG is for, but instead of
 * generating any printf-like message, this is for dumping raw block of a
 * memory in a hexadecimal form:
 *
 *   TEST_CHECK(size_produced == size_expected &&
 *              memcmp(addr_produced, addr_expected, size_produced) == 0);
 *   TEST_DUMP("Expected:", addr_expected, size_expected);
 *   TEST_DUMP("Produced:", addr_produced, size_produced);
 */
#define TEST_DUMP(title, addr, size)    acutest_dump_(title, addr, size)

/* Maximal output per TEST_DUMP call (in bytes to dump). Longer blocks are cut.
 * You may define another limit prior including "acutest.h"
 */
#ifndef TEST_DUMP_MAXSIZE
    #define TEST_DUMP_MAXSIZE   1024
#endif


/* Common test initialiation/clean-up
 *
 * In some test suites, it may be needed to perform some sort of the same
 * initialization and/or clean-up in all the tests.
 *
 * Such test suites may use macros TEST_INIT and/or TEST_FINI prior including
 * this header. The expansion of the macro is then used as a body of helper
 * function called just before executing every single (TEST_INIT) or just after
 * it ends (TEST_FINI).
 *
 * Examples of various ways how to use the macro TEST_INIT:
 *
 *   #define TEST_INIT      my_init_func();
 *   #define TEST_INIT      my_init_func()      // Works even without the semicolon
 *   #define TEST_INIT      setlocale(LC_ALL, NULL);
 *   #define TEST_INIT      { setlocale(LC_ALL, NULL); my_init_func(); }
 *
 * TEST_FINI is to be used in the same way.
 */


/**********************
 *** Implementation ***
 **********************/

/* The unit test files should not rely on anything below. */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#if defined(unix) || defined(__unix__) || defined(__unix) || defined(__APPLE__)
    #define ACUTEST_UNIX_       1
    #include <errno.h>
    #include <libgen.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <signal.h>
    #include <time.h>

    #if defined CLOCK_PROCESS_CPUTIME_ID  &&  defined CLOCK_MONOTONIC
        #define ACUTEST_HAS_POSIX_TIMER_    1
    #endif
#endif

#if defined(_gnu_linux_) || defined(__linux__)
    #define ACUTEST_LINUX_      1
    #include <fcntl.h>
    #include <sys/stat.h>
#endif

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
    #define ACUTEST_WIN_        1
    #include <windows.h>
    #include <io.h>
#endif

#if defined(__APPLE__)
    #define ACUTEST_MACOS_
    #include <assert.h>
    #include <stdbool.h>
    #include <sys/types.h>
    #include <unistd.h>
    #include <sys/sysctl.h>
#endif

#ifdef __cplusplus
    #include <exception>
#endif

#ifdef __has_include
    #if __has_include(<valgrind.h>)
        #include <valgrind.h>
    #endif
#endif

/* Enable the use of the non-standard keyword __attribute__ to silence warnings under some compilers */
#if defined(__GNUC__) || defined(__clang__)
    #define ACUTEST_ATTRIBUTE_(attr)    __attribute__((attr))
#else
    #define ACUTEST_ATTRIBUTE_(attr)
#endif

/* Note our global private identifiers end with '_' to mitigate risk of clash
 * with the unit tests implementation. */

#ifdef __cplusplus
    extern "C" {
#endif

#ifdef _MSC_VER
    /* In the multi-platform code like ours, we cannot use the non-standard
     * "safe" functions from Microsoft C lib like e.g. sprintf_s() instead of
     * standard sprintf(). Hence, lets disable the warning C4996. */
    #pragma warning(push)
    #pragma warning(disable: 4996)
#endif


struct acutest_test_ {
    const char* name;
    void (*func)(void);
};

struct acutest_test_data_ {
    unsigned char flags;
    double duration;
};

enum {
    ACUTEST_FLAG_RUN_ = 1 << 0,
    ACUTEST_FLAG_SUCCESS_ = 1 << 1,
    ACUTEST_FLAG_FAILURE_ = 1 << 2,
};

extern const struct acutest_test_ acutest_list_[];

int acutest_check_(int cond, const char* file, int line, const char* fmt, ...);
void acutest_case_(const char* fmt, ...);
void acutest_message_(const char* fmt, ...);
void acutest_dump_(const char* title, const void* addr, size_t size);
void acutest_abort_(void) ACUTEST_ATTRIBUTE_(noreturn);


#ifndef TEST_NO_MAIN

static char* acutest_argv0_ = NULL;
static size_t acutest_list_size_ = 0;
static struct acutest_test_data_* acutest_test_data_ = NULL;
static size_t acutest_count_ = 0;
static int acutest_no_exec_ = -1;
static int acutest_no_summary_ = 0;
static int acutest_tap_ = 0;
static int acutest_skip_mode_ = 0;
static int acutest_worker_ = 0;
static int acutest_worker_index_ = 0;
static int acutest_cond_failed_ = 0;
static int acutest_was_aborted_ = 0;
static FILE *acutest_xml_output_ = NULL;

static int acutest_stat_failed_units_ = 0;
static int acutest_stat_run_units_ = 0;

static const struct acutest_test_* acutest_current_test_ = NULL;
static int acutest_current_index_ = 0;
static char acutest_case_name_[TEST_CASE_MAXSIZE] = "";
static int acutest_test_already_logged_ = 0;
static int acutest_case_already_logged_ = 0;
static int acutest_verbose_level_ = 2;
static int acutest_test_failures_ = 0;
static int acutest_colorize_ = 0;
static int acutest_timer_ = 0;

static int acutest_abort_has_jmp_buf_ = 0;
static jmp_buf acutest_abort_jmp_buf_;


static void
acutest_cleanup_(void)
{
    free((void*) acutest_test_data_);
}

static void ACUTEST_ATTRIBUTE_(noreturn)
acutest_exit_(int exit_code)
{
    acutest_cleanup_();
    exit(exit_code);
}

#if defined ACUTEST_WIN_
    typedef LARGE_INTEGER acutest_timer_type_;
    static LARGE_INTEGER acutest_timer_freq_;
    static acutest_timer_type_ acutest_timer_start_;
    static acutest_timer_type_ acutest_timer_end_;

    static void
    acutest_timer_init_(void)
    {
        QueryPerformanceFrequency(&acutest_timer_freq_);
    }

    static void
    acutest_timer_get_time_(LARGE_INTEGER* ts)
    {
        QueryPerformanceCounter(ts);
    }

    static double
    acutest_timer_diff_(LARGE_INTEGER start, LARGE_INTEGER end)
    {
        double duration = (double)(end.QuadPart - start.QuadPart);
        duration /= (double)acutest_timer_freq_.QuadPart;
        return duration;
    }

    static void
    acutest_timer_print_diff_(void)
    {
        printf("%.6lf secs", acutest_timer_diff_(acutest_timer_start_, acutest_timer_end_));
    }
#elif defined ACUTEST_HAS_POSIX_TIMER_
    static clockid_t acutest_timer_id_;
    typedef struct timespec acutest_timer_type_;
    static acutest_timer_type_ acutest_timer_start_;
    static acutest_timer_type_ acutest_timer_end_;

    static void
    acutest_timer_init_(void)
    {
        if(acutest_timer_ == 1)
            acutest_timer_id_ = CLOCK_MONOTONIC;
        else if(acutest_timer_ == 2)
            acutest_timer_id_ = CLOCK_PROCESS_CPUTIME_ID;
    }

    static void
    acutest_timer_get_time_(struct timespec* ts)
    {
        clock_gettime(acutest_timer_id_, ts);
    }

    static double
    acutest_timer_diff_(struct timespec start, struct timespec end)
    {
        double endns;
        double startns;

        endns = end.tv_sec;
        endns *= 1e9;
        endns += end.tv_nsec;

        startns = start.tv_sec;
        startns *= 1e9;
        startns += start.tv_nsec;

        return ((endns - startns)/ 1e9);
    }

    static void
    acutest_timer_print_diff_(void)
    {
        printf("%.6lf secs",
            acutest_timer_diff_(acutest_timer_start_, acutest_timer_end_));
    }
#else
    typedef int acutest_timer_type_;
    static acutest_timer_type_ acutest_timer_start_;
    static acutest_timer_type_ acutest_timer_end_;

    void
    acutest_timer_init_(void)
    {}

    static void
    acutest_timer_get_time_(int* ts)
    {
        (void) ts;
    }

    static double
    acutest_timer_diff_(int start, int end)
    {
        (void) start;
        (void) end;
        return 0.0;
    }

    static void
    acutest_timer_print_diff_(void)
    {}
#endif

#define ACUTEST_COLOR_DEFAULT_              0
#define ACUTEST_COLOR_GREEN_                1
#define ACUTEST_COLOR_RED_                  2
#define ACUTEST_COLOR_DEFAULT_INTENSIVE_    3
#define ACUTEST_COLOR_GREEN_INTENSIVE_      4
#define ACUTEST_COLOR_RED_INTENSIVE_        5

static int ACUTEST_ATTRIBUTE_(format (printf, 2, 3))
acutest_colored_printf_(int color, const char* fmt, ...)
{
    va_list args;
    char buffer[256];
    int n;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    buffer[sizeof(buffer)-1] = '\0';

    if(!acutest_colorize_) {
        return printf("%s", buffer);
    }

#if defined ACUTEST_UNIX_
    {
        const char* col_str;
        switch(color) {
            case ACUTEST_COLOR_GREEN_:              col_str = "\033[0;32m"; break;
            case ACUTEST_COLOR_RED_:                col_str = "\033[0;31m"; break;
            case ACUTEST_COLOR_GREEN_INTENSIVE_:    col_str = "\033[1;32m"; break;
            case ACUTEST_COLOR_RED_INTENSIVE_:      col_str = "\033[1;31m"; break;
            case ACUTEST_COLOR_DEFAULT_INTENSIVE_:  col_str = "\033[1m"; break;
            default:                                col_str = "\033[0m"; break;
        }
        printf("%s", col_str);
        n = printf("%s", buffer);
        printf("\033[0m");
        return n;
    }
#elif defined ACUTEST_WIN_
    {
        HANDLE h;
        CONSOLE_SCREEN_BUFFER_INFO info;
        WORD attr;

        h = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleScreenBufferInfo(h, &info);

        switch(color) {
            case ACUTEST_COLOR_GREEN_:              attr = FOREGROUND_GREEN; break;
            case ACUTEST_COLOR_RED_:                attr = FOREGROUND_RED; break;
            case ACUTEST_COLOR_GREEN_INTENSIVE_:    attr = FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
            case ACUTEST_COLOR_RED_INTENSIVE_:      attr = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
            case ACUTEST_COLOR_DEFAULT_INTENSIVE_:  attr = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY; break;
            default:                                attr = 0; break;
        }
        if(attr != 0)
            SetConsoleTextAttribute(h, attr);
        n = printf("%s", buffer);
        SetConsoleTextAttribute(h, info.wAttributes);
        return n;
    }
#else
    n = printf("%s", buffer);
    return n;
#endif
}

static void
acutest_begin_test_line_(const struct acutest_test_* test)
{
    if(!acutest_tap_) {
        if(acutest_verbose_level_ >= 3) {
            acutest_colored_printf_(ACUTEST_COLOR_DEFAULT_INTENSIVE_, "Test %s:\n", test->name);
            acutest_test_already_logged_++;
        } else if(acutest_verbose_level_ >= 1) {
            int n;
            char spaces[48];

            n = acutest_colored_printf_(ACUTEST_COLOR_DEFAULT_INTENSIVE_, "Test %s... ", test->name);
            memset(spaces, ' ', sizeof(spaces));
            if(n < (int) sizeof(spaces))
                printf("%.*s", (int) sizeof(spaces) - n, spaces);
        } else {
            acutest_test_already_logged_ = 1;
        }
    }
}

static void
acutest_finish_test_line_(int result)
{
    if(acutest_tap_) {
        const char* str = (result == 0) ? "ok" : "not ok";

        printf("%s %d - %s\n", str, acutest_current_index_ + 1, acutest_current_test_->name);

        if(result == 0  &&  acutest_timer_) {
            printf("# Duration: ");
            acutest_timer_print_diff_();
            printf("\n");
        }
    } else {
        int color = (result == 0) ? ACUTEST_COLOR_GREEN_INTENSIVE_ : ACUTEST_COLOR_RED_INTENSIVE_;
        const char* str = (result == 0) ? "OK" : "FAILED";
        printf("[ ");
        acutest_colored_printf_(color, "%s", str);
        printf(" ]");

        if(result == 0  &&  acutest_timer_) {
            printf("  ");
            acutest_timer_print_diff_();
        }

        printf("\n");
    }
}

static void
acutest_line_indent_(int level)
{
    static const char spaces[] = "                ";
    int n = level * 2;

    if(acutest_tap_  &&  n > 0) {
        n--;
        printf("#");
    }

    while(n > 16) {
        printf("%s", spaces);
        n -= 16;
    }
    printf("%.*s", n, spaces);
}

int ACUTEST_ATTRIBUTE_(format (printf, 4, 5))
acutest_check_(int cond, const char* file, int line, const char* fmt, ...)
{
    const char *result_str;
    int result_color;
    int verbose_level;

    if(cond) {
        result_str = "ok";
        result_color = ACUTEST_COLOR_GREEN_;
        verbose_level = 3;
    } else {
        if(!acutest_test_already_logged_  &&  acutest_current_test_ != NULL)
            acutest_finish_test_line_(-1);

        result_str = "failed";
        result_color = ACUTEST_COLOR_RED_;
        verbose_level = 2;
        acutest_test_failures_++;
        acutest_test_already_logged_++;
    }

    if(acutest_verbose_level_ >= verbose_level) {
        va_list args;

        if(!acutest_case_already_logged_  &&  acutest_case_name_[0]) {
            acutest_line_indent_(1);
            acutest_colored_printf_(ACUTEST_COLOR_DEFAULT_INTENSIVE_, "Case %s:\n", acutest_case_name_);
            acutest_test_already_logged_++;
            acutest_case_already_logged_++;
        }

        acutest_line_indent_(acutest_case_name_[0] ? 2 : 1);
        if(file != NULL) {
#ifdef ACUTEST_WIN_
            const char* lastsep1 = strrchr(file, '\\');
            const char* lastsep2 = strrchr(file, '/');
            if(lastsep1 == NULL)
                lastsep1 = file-1;
            if(lastsep2 == NULL)
                lastsep2 = file-1;
            file = (lastsep1 > lastsep2 ? lastsep1 : lastsep2) + 1;
#else
            const char* lastsep = strrchr(file, '/');
            if(lastsep != NULL)
                file = lastsep+1;
#endif
            printf("%s:%d: Check ", file, line);
        }

        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);

        printf("... ");
        acutest_colored_printf_(result_color, "%s", result_str);
        printf("\n");
        acutest_test_already_logged_++;
    }

    acutest_cond_failed_ = (cond == 0);
    return !acutest_cond_failed_;
}

void ACUTEST_ATTRIBUTE_(format (printf, 1, 2))
acutest_case_(const char* fmt, ...)
{
    va_list args;

    if(acutest_verbose_level_ < 2)
        return;

    if(acutest_case_name_[0]) {
        acutest_case_already_logged_ = 0;
        acutest_case_name_[0] = '\0';
    }

    if(fmt == NULL)
        return;

    va_start(args, fmt);
    vsnprintf(acutest_case_name_, sizeof(acutest_case_name_) - 1, fmt, args);
    va_end(args);
    acutest_case_name_[sizeof(acutest_case_name_) - 1] = '\0';

    if(acutest_verbose_level_ >= 3) {
        acutest_line_indent_(1);
        acutest_colored_printf_(ACUTEST_COLOR_DEFAULT_INTENSIVE_, "Case %s:\n", acutest_case_name_);
        acutest_test_already_logged_++;
        acutest_case_already_logged_++;
    }
}

void ACUTEST_ATTRIBUTE_(format (printf, 1, 2))
acutest_message_(const char* fmt, ...)
{
    char buffer[TEST_MSG_MAXSIZE];
    char* line_beg;
    char* line_end;
    va_list args;

    if(acutest_verbose_level_ < 2)
        return;

    /* We allow extra message only when something is already wrong in the
     * current test. */
    if(acutest_current_test_ == NULL  ||  !acutest_cond_failed_)
        return;

    va_start(args, fmt);
    vsnprintf(buffer, TEST_MSG_MAXSIZE, fmt, args);
    va_end(args);
    buffer[TEST_MSG_MAXSIZE-1] = '\0';

    line_beg = buffer;
    while(1) {
        line_end = strchr(line_beg, '\n');
        if(line_end == NULL)
            break;
        acutest_line_indent_(acutest_case_name_[0] ? 3 : 2);
        printf("%.*s\n", (int)(line_end - line_beg), line_beg);
        line_beg = line_end + 1;
    }
    if(line_beg[0] != '\0') {
        acutest_line_indent_(acutest_case_name_[0] ? 3 : 2);
        printf("%s\n", line_beg);
    }
}

void
acutest_dump_(const char* title, const void* addr, size_t size)
{
    static const size_t BYTES_PER_LINE = 16;
    size_t line_beg;
    size_t truncate = 0;

    if(acutest_verbose_level_ < 2)
        return;

    /* We allow extra message only when something is already wrong in the
     * current test. */
    if(acutest_current_test_ == NULL  ||  !acutest_cond_failed_)
        return;

    if(size > TEST_DUMP_MAXSIZE) {
        truncate = size - TEST_DUMP_MAXSIZE;
        size = TEST_DUMP_MAXSIZE;
    }

    acutest_line_indent_(acutest_case_name_[0] ? 3 : 2);
    printf((title[strlen(title)-1] == ':') ? "%s\n" : "%s:\n", title);

    for(line_beg = 0; line_beg < size; line_beg += BYTES_PER_LINE) {
        size_t line_end = line_beg + BYTES_PER_LINE;
        size_t off;

        acutest_line_indent_(acutest_case_name_[0] ? 4 : 3);
        printf("%08lx: ", (unsigned long)line_beg);
        for(off = line_beg; off < line_end; off++) {
            if(off < size)
                printf(" %02x", ((const unsigned char*)addr)[off]);
            else
                printf("   ");
        }

        printf("  ");
        for(off = line_beg; off < line_end; off++) {
            unsigned char byte = ((const unsigned char*)addr)[off];
            if(off < size)
                printf("%c", (iscntrl(byte) ? '.' : byte));
            else
                break;
        }

        printf("\n");
    }

    if(truncate > 0) {
        acutest_line_indent_(acutest_case_name_[0] ? 4 : 3);
        printf("           ... (and more %u bytes)\n", (unsigned) truncate);
    }
}

/* This is called just before each test */
static void
acutest_init_(const char *test_name)
{
#ifdef TEST_INIT
    TEST_INIT
    ; /* Allow for a single unterminated function call */
#endif

    /* Suppress any warnings about unused variable. */
    (void) test_name;
}

/* This is called after each test */
static void
acutest_fini_(const char *test_name)
{
#ifdef TEST_FINI
    TEST_FINI
    ; /* Allow for a single unterminated function call */
#endif

    /* Suppress any warnings about unused variable. */
    (void) test_name;
}

void
acutest_abort_(void)
{
    if(acutest_abort_has_jmp_buf_) {
        longjmp(acutest_abort_jmp_buf_, 1);
    } else {
        if(acutest_current_test_ != NULL)
            acutest_fini_(acutest_current_test_->name);
        abort();
    }
}

static void
acutest_list_names_(void)
{
    const struct acutest_test_* test;

    printf("Unit tests:\n");
    for(test = &acutest_list_[0]; test->func != NULL; test++)
        printf("  %s\n", test->name);
}

static void
acutest_remember_(int i)
{
    if(acutest_test_data_[i].flags & ACUTEST_FLAG_RUN_)
        return;

    acutest_test_data_[i].flags |= ACUTEST_FLAG_RUN_;
    acutest_count_++;
}

static void
acutest_set_success_(int i, int success)
{
    acutest_test_data_[i].flags |= success ? ACUTEST_FLAG_SUCCESS_ : ACUTEST_FLAG_FAILURE_;
}

static void
acutest_set_duration_(int i, double duration)
{
    acutest_test_data_[i].duration = duration;
}

static int
acutest_name_contains_word_(const char* name, const char* pattern)
{
    static const char word_delim[] = " \t-_/.,:;";
    const char* substr;
    size_t pattern_len;

    pattern_len = strlen(pattern);

    substr = strstr(name, pattern);
    while(substr != NULL) {
        int starts_on_word_boundary = (substr == name || strchr(word_delim, substr[-1]) != NULL);
        int ends_on_word_boundary = (substr[pattern_len] == '\0' || strchr(word_delim, substr[pattern_len]) != NULL);

        if(starts_on_word_boundary && ends_on_word_boundary)
            return 1;

        substr = strstr(substr+1, pattern);
    }

    return 0;
}

static int
acutest_lookup_(const char* pattern)
{
    int i;
    int n = 0;

    /* Try exact match. */
    for(i = 0; i < (int) acutest_list_size_; i++) {
        if(strcmp(acutest_list_[i].name, pattern) == 0) {
            acutest_remember_(i);
            n++;
            break;
        }
    }
    if(n > 0)
        return n;

    /* Try word match. */
    for(i = 0; i < (int) acutest_list_size_; i++) {
        if(acutest_name_contains_word_(acutest_list_[i].name, pattern)) {
            acutest_remember_(i);
            n++;
        }
    }
    if(n > 0)
        return n;

    /* Try relaxed match. */
    for(i = 0; i < (int) acutest_list_size_; i++) {
        if(strstr(acutest_list_[i].name, pattern) != NULL) {
            acutest_remember_(i);
            n++;
        }
    }

    return n;
}


/* Called if anything goes bad in Acutest, or if the unit test ends in other
 * way then by normal returning from its function (e.g. exception or some
 * abnormal child process termination). */
static void ACUTEST_ATTRIBUTE_(format (printf, 1, 2))
acutest_error_(const char* fmt, ...)
{
    if(acutest_verbose_level_ == 0)
        return;

    if(acutest_verbose_level_ >= 2) {
        va_list args;

        acutest_line_indent_(1);
        if(acutest_verbose_level_ >= 3)
            acutest_colored_printf_(ACUTEST_COLOR_RED_INTENSIVE_, "ERROR: ");
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        printf("\n");
    }

    if(acutest_verbose_level_ >= 3) {
        printf("\n");
    }
}

/* Call directly the given test unit function. */
static int
acutest_do_run_(const struct acutest_test_* test, int index)
{
    int status = -1;

    acutest_was_aborted_ = 0;
    acutest_current_test_ = test;
    acutest_current_index_ = index;
    acutest_test_failures_ = 0;
    acutest_test_already_logged_ = 0;
    acutest_cond_failed_ = 0;

#ifdef __cplusplus
/* TinyUSDZ: disable exception */
#if 0
    try {
#else
    {
#endif
#endif
        acutest_init_(test->name);
        acutest_begin_test_line_(test);

        /* This is good to do in case the test unit crashes. */
        fflush(stdout);
        fflush(stderr);

        if(!acutest_worker_) {
            acutest_abort_has_jmp_buf_ = 1;
            if(setjmp(acutest_abort_jmp_buf_) != 0) {
                acutest_was_aborted_ = 1;
                goto aborted;
            }
        }

        acutest_timer_get_time_(&acutest_timer_start_);
        test->func();
aborted:
        acutest_abort_has_jmp_buf_ = 0;
        acutest_timer_get_time_(&acutest_timer_end_);

        if(acutest_verbose_level_ >= 3) {
            acutest_line_indent_(1);
            if(acutest_test_failures_ == 0) {
                acutest_colored_printf_(ACUTEST_COLOR_GREEN_INTENSIVE_, "SUCCESS: ");
                printf("All conditions have passed.\n");

                if(acutest_timer_) {
                    acutest_line_indent_(1);
                    printf("Duration: ");
                    acutest_timer_print_diff_();
                    printf("\n");
                }
            } else {
                acutest_colored_printf_(ACUTEST_COLOR_RED_INTENSIVE_, "FAILED: ");
                if(!acutest_was_aborted_) {
                    printf("%d condition%s %s failed.\n",
                            acutest_test_failures_,
                            (acutest_test_failures_ == 1) ? "" : "s",
                            (acutest_test_failures_ == 1) ? "has" : "have");
                } else {
                    printf("Aborted.\n");
                }
            }
            printf("\n");
        } else if(acutest_verbose_level_ >= 1 && acutest_test_failures_ == 0) {
            acutest_finish_test_line_(0);
        }

        status = (acutest_test_failures_ == 0) ? 0 : -1;

#ifdef __cplusplus
/* TinyUSDZ: disable exception */
#if 0
    } catch(std::exception& e) {
        const char* what = e.what();
        acutest_check_(0, NULL, 0, "Threw std::exception");
        if(what != NULL)
            acutest_message_("std::exception::what(): %s", what);

        if(acutest_verbose_level_ >= 3) {
            acutest_line_indent_(1);
            acutest_colored_printf_(ACUTEST_COLOR_RED_INTENSIVE_, "FAILED: ");
            printf("C++ exception.\n\n");
        }
    } catch(...) {
        acutest_check_(0, NULL, 0, "Threw an exception");

        if(acutest_verbose_level_ >= 3) {
            acutest_line_indent_(1);
            acutest_colored_printf_(ACUTEST_COLOR_RED_INTENSIVE_, "FAILED: ");
            printf("C++ exception.\n\n");
        }
    }
#else
    }
#endif
#endif

    acutest_fini_(test->name);
    acutest_case_(NULL);
    acutest_current_test_ = NULL;

    return status;
}

/* Trigger the unit test. If possible (and not suppressed) it starts a child
 * process who calls acutest_do_run_(), otherwise it calls acutest_do_run_()
 * directly. */
static void
acutest_run_(const struct acutest_test_* test, int index, int master_index)
{
    int failed = 1;
    acutest_timer_type_ start, end;

    acutest_current_test_ = test;
    acutest_test_already_logged_ = 0;
    acutest_timer_get_time_(&start);

    if(!acutest_no_exec_) {

#if defined(ACUTEST_UNIX_)

        pid_t pid;
        int exit_code;

        /* Make sure the child starts with empty I/O buffers. */
        fflush(stdout);
        fflush(stderr);

        pid = fork();
        if(pid == (pid_t)-1) {
            acutest_error_("Cannot fork. %s [%d]", strerror(errno), errno);
            failed = 1;
        } else if(pid == 0) {
            /* Child: Do the test. */
            acutest_worker_ = 1;
            failed = (acutest_do_run_(test, index) != 0);
            acutest_exit_(failed ? 1 : 0);
        } else {
            /* Parent: Wait until child terminates and analyze its exit code. */
            waitpid(pid, &exit_code, 0);
            if(WIFEXITED(exit_code)) {
                switch(WEXITSTATUS(exit_code)) {
                    case 0:   failed = 0; break;   /* test has passed. */
                    case 1:   /* noop */ break;    /* "normal" failure. */
                    default:  acutest_error_("Unexpected exit code [%d]", WEXITSTATUS(exit_code));
                }
            } else if(WIFSIGNALED(exit_code)) {
                char tmp[32];
                const char* signame;
                switch(WTERMSIG(exit_code)) {
                    case SIGINT:  signame = "SIGINT"; break;
                    case SIGHUP:  signame = "SIGHUP"; break;
                    case SIGQUIT: signame = "SIGQUIT"; break;
                    case SIGABRT: signame = "SIGABRT"; break;
                    case SIGKILL: signame = "SIGKILL"; break;
                    case SIGSEGV: signame = "SIGSEGV"; break;
                    case SIGILL:  signame = "SIGILL"; break;
                    case SIGTERM: signame = "SIGTERM"; break;
                    default:      snprintf(tmp, 32, "signal %d", WTERMSIG(exit_code)); tmp[31] = '\n'; signame = tmp; break;
                }
                acutest_error_("Test interrupted by %s.", signame);
            } else {
                acutest_error_("Test ended in an unexpected way [%d].", exit_code);
            }
        }

#elif defined(ACUTEST_WIN_)

        char buffer[512] = {0};
        STARTUPINFOA startupInfo;
        PROCESS_INFORMATION processInfo;
        DWORD exitCode;

        /* Windows has no fork(). So we propagate all info into the child
         * through a command line arguments. */
        _snprintf(buffer, sizeof(buffer)-1,
                 "%s --worker=%d %s --no-exec --no-summary %s --verbose=%d --color=%s -- \"%s\"",
                 acutest_argv0_, index, acutest_timer_ ? "--time" : "",
                 acutest_tap_ ? "--tap" : "", acutest_verbose_level_,
                 acutest_colorize_ ? "always" : "never",
                 test->name);
        memset(&startupInfo, 0, sizeof(startupInfo));
        startupInfo.cb = sizeof(STARTUPINFO);
        if(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo)) {
            WaitForSingleObject(processInfo.hProcess, INFINITE);
            GetExitCodeProcess(processInfo.hProcess, &exitCode);
            CloseHandle(processInfo.hThread);
            CloseHandle(processInfo.hProcess);
            failed = (exitCode != 0);
            if(exitCode > 1) {
                switch(exitCode) {
                    case 3:             acutest_error_("Aborted."); break;
                    case 0xC0000005:    acutest_error_("Access violation."); break;
                    default:            acutest_error_("Test ended in an unexpected way [%lu].", exitCode); break;
                }
            }
        } else {
            acutest_error_("Cannot create unit test subprocess [%ld].", GetLastError());
            failed = 1;
        }

#else

        /* A platform where we don't know how to run child process. */
        failed = (acutest_do_run_(test, index) != 0);

#endif

    } else {
        /* Child processes suppressed through --no-exec. */
        failed = (acutest_do_run_(test, index) != 0);
    }
    acutest_timer_get_time_(&end);

    acutest_current_test_ = NULL;

    acutest_stat_run_units_++;
    if(failed)
        acutest_stat_failed_units_++;

    acutest_set_success_(master_index, !failed);
    acutest_set_duration_(master_index, acutest_timer_diff_(start, end));
}

#if defined(ACUTEST_WIN_)
/* Callback for SEH events. */
static LONG CALLBACK
acutest_seh_exception_filter_(EXCEPTION_POINTERS *ptrs)
{
    acutest_check_(0, NULL, 0, "Unhandled SEH exception");
    acutest_message_("Exception code:    0x%08lx", ptrs->ExceptionRecord->ExceptionCode);
    acutest_message_("Exception address: 0x%p", ptrs->ExceptionRecord->ExceptionAddress);

    fflush(stdout);
    fflush(stderr);

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif


#define ACUTEST_CMDLINE_OPTFLAG_OPTIONALARG_    0x0001
#define ACUTEST_CMDLINE_OPTFLAG_REQUIREDARG_    0x0002

#define ACUTEST_CMDLINE_OPTID_NONE_             0
#define ACUTEST_CMDLINE_OPTID_UNKNOWN_          (-0x7fffffff + 0)
#define ACUTEST_CMDLINE_OPTID_MISSINGARG_       (-0x7fffffff + 1)
#define ACUTEST_CMDLINE_OPTID_BOGUSARG_         (-0x7fffffff + 2)

typedef struct acutest_test_CMDLINE_OPTION_ {
    char shortname;
    const char* longname;
    int id;
    unsigned flags;
} ACUTEST_CMDLINE_OPTION_;

static int
acutest_cmdline_handle_short_opt_group_(const ACUTEST_CMDLINE_OPTION_* options,
                    const char* arggroup,
                    int (*callback)(int /*optval*/, const char* /*arg*/))
{
    const ACUTEST_CMDLINE_OPTION_* opt;
    int i;
    int ret = 0;

    for(i = 0; arggroup[i] != '\0'; i++) {
        for(opt = options; opt->id != 0; opt++) {
            if(arggroup[i] == opt->shortname)
                break;
        }

        if(opt->id != 0  &&  !(opt->flags & ACUTEST_CMDLINE_OPTFLAG_REQUIREDARG_)) {
            ret = callback(opt->id, NULL);
        } else {
            /* Unknown option. */
            char badoptname[3];
            badoptname[0] = '-';
            badoptname[1] = arggroup[i];
            badoptname[2] = '\0';
            ret = callback((opt->id != 0 ? ACUTEST_CMDLINE_OPTID_MISSINGARG_ : ACUTEST_CMDLINE_OPTID_UNKNOWN_),
                            badoptname);
        }

        if(ret != 0)
            break;
    }

    return ret;
}

#define ACUTEST_CMDLINE_AUXBUF_SIZE_  32

static int
acutest_cmdline_read_(const ACUTEST_CMDLINE_OPTION_* options, int argc, char** argv,
                      int (*callback)(int /*optval*/, const char* /*arg*/))
{

    const ACUTEST_CMDLINE_OPTION_* opt;
    char auxbuf[ACUTEST_CMDLINE_AUXBUF_SIZE_+1];
    int after_doubledash = 0;
    int i = 1;
    int ret = 0;

    auxbuf[ACUTEST_CMDLINE_AUXBUF_SIZE_] = '\0';

    while(i < argc) {
        if(after_doubledash  ||  strcmp(argv[i], "-") == 0) {
            /* Non-option argument. */
            ret = callback(ACUTEST_CMDLINE_OPTID_NONE_, argv[i]);
        } else if(strcmp(argv[i], "--") == 0) {
            /* End of options. All the remaining members are non-option arguments. */
            after_doubledash = 1;
        } else if(argv[i][0] != '-') {
            /* Non-option argument. */
            ret = callback(ACUTEST_CMDLINE_OPTID_NONE_, argv[i]);
        } else {
            for(opt = options; opt->id != 0; opt++) {
                if(opt->longname != NULL  &&  strncmp(argv[i], "--", 2) == 0) {
                    size_t len = strlen(opt->longname);
                    if(strncmp(argv[i]+2, opt->longname, len) == 0) {
                        /* Regular long option. */
                        if(argv[i][2+len] == '\0') {
                            /* with no argument provided. */
                            if(!(opt->flags & ACUTEST_CMDLINE_OPTFLAG_REQUIREDARG_))
                                ret = callback(opt->id, NULL);
                            else
                                ret = callback(ACUTEST_CMDLINE_OPTID_MISSINGARG_, argv[i]);
                            break;
                        } else if(argv[i][2+len] == '=') {
                            /* with an argument provided. */
                            if(opt->flags & (ACUTEST_CMDLINE_OPTFLAG_OPTIONALARG_ | ACUTEST_CMDLINE_OPTFLAG_REQUIREDARG_)) {
                                ret = callback(opt->id, argv[i]+2+len+1);
                            } else {
                                snprintf(auxbuf, sizeof(auxbuf), "--%s", opt->longname); auxbuf[sizeof(auxbuf)-1] = '\n';
                                ret = callback(ACUTEST_CMDLINE_OPTID_BOGUSARG_, auxbuf);
                            }
                            break;
                        } else {
                            continue;
                        }
                    }
                } else if(opt->shortname != '\0'  &&  argv[i][0] == '-') {
                    if(argv[i][1] == opt->shortname) {
                        /* Regular short option. */
                        if(opt->flags & ACUTEST_CMDLINE_OPTFLAG_REQUIREDARG_) {
                            if(argv[i][2] != '\0')
                                ret = callback(opt->id, argv[i]+2);
                            else if(i+1 < argc)
                                ret = callback(opt->id, argv[++i]);
                            else
                                ret = callback(ACUTEST_CMDLINE_OPTID_MISSINGARG_, argv[i]);
                            break;
                        } else {
                            ret = callback(opt->id, NULL);

                            /* There might be more (argument-less) short options
                             * grouped together. */
                            if(ret == 0  &&  argv[i][2] != '\0')
                                ret = acutest_cmdline_handle_short_opt_group_(options, argv[i]+2, callback);
                            break;
                        }
                    }
                }
            }

            if(opt->id == 0) {  /* still not handled? */
                if(argv[i][0] != '-') {
                    /* Non-option argument. */
                    ret = callback(ACUTEST_CMDLINE_OPTID_NONE_, argv[i]);
                } else {
                    /* Unknown option. */
                    char* badoptname = argv[i];

                    if(strncmp(badoptname, "--", 2) == 0) {
                        /* Strip any argument from the long option. */
                        char* assignment = strchr(badoptname, '=');
                        if(assignment != NULL) {
                            size_t len = assignment - badoptname;
                            if(len > ACUTEST_CMDLINE_AUXBUF_SIZE_)
                                len = ACUTEST_CMDLINE_AUXBUF_SIZE_;
                            strncpy(auxbuf, badoptname, len);
                            auxbuf[len] = '\0';
                            badoptname = auxbuf;
                        }
                    }

                    ret = callback(ACUTEST_CMDLINE_OPTID_UNKNOWN_, badoptname);
                }
            }
        }

        if(ret != 0)
            return ret;
        i++;
    }

    return ret;
}

static void
acutest_help_(void)
{
    printf("Usage: %s [options] [test...]\n", acutest_argv0_);
    printf("\n");
    printf("Run the specified unit tests; or if the option '--skip' is used, run all\n");
    printf("tests in the suite but those listed.  By default, if no tests are specified\n");
    printf("on the command line, all unit tests in the suite are run.\n");
    printf("\n");
    printf("Options:\n");
    printf("  -s, --skip            Execute all unit tests but the listed ones\n");
    printf("      --exec[=WHEN]     If supported, execute unit tests as child processes\n");
    printf("                          (WHEN is one of 'auto', 'always', 'never')\n");
    printf("  -E, --no-exec         Same as --exec=never\n");
#if defined ACUTEST_WIN_
    printf("  -t, --time            Measure test duration\n");
#elif defined ACUTEST_HAS_POSIX_TIMER_
    printf("  -t, --time            Measure test duration (real time)\n");
    printf("      --time=TIMER      Measure test duration, using given timer\n");
    printf("                          (TIMER is one of 'real', 'cpu')\n");
#endif
    printf("      --no-summary      Suppress printing of test results summary\n");
    printf("      --tap             Produce TAP-compliant output\n");
    printf("                          (See https://testanything.org/)\n");
    printf("  -x, --xml-output=FILE Enable XUnit output to the given file\n");
    printf("  -l, --list            List unit tests in the suite and exit\n");
    printf("  -v, --verbose         Make output more verbose\n");
    printf("      --verbose=LEVEL   Set verbose level to LEVEL:\n");
    printf("                          0 ... Be silent\n");
    printf("                          1 ... Output one line per test (and summary)\n");
    printf("                          2 ... As 1 and failed conditions (this is default)\n");
    printf("                          3 ... As 1 and all conditions (and extended summary)\n");
    printf("  -q, --quiet           Same as --verbose=0\n");
    printf("      --color[=WHEN]    Enable colorized output\n");
    printf("                          (WHEN is one of 'auto', 'always', 'never')\n");
    printf("      --no-color        Same as --color=never\n");
    printf("  -h, --help            Display this help and exit\n");

    if(acutest_list_size_ < 16) {
        printf("\n");
        acutest_list_names_();
    }
}

static const ACUTEST_CMDLINE_OPTION_ acutest_cmdline_options_[] = {
    { 's',  "skip",         's', 0 },
    {  0,   "exec",         'e', ACUTEST_CMDLINE_OPTFLAG_OPTIONALARG_ },
    { 'E',  "no-exec",      'E', 0 },
#if defined ACUTEST_WIN_
    { 't',  "time",         't', 0 },
    {  0,   "timer",        't', 0 },   /* kept for compatibility */
#elif defined ACUTEST_HAS_POSIX_TIMER_
    { 't',  "time",         't', ACUTEST_CMDLINE_OPTFLAG_OPTIONALARG_ },
    {  0,   "timer",        't', ACUTEST_CMDLINE_OPTFLAG_OPTIONALARG_ },  /* kept for compatibility */
#endif
    {  0,   "no-summary",   'S', 0 },
    {  0,   "tap",          'T', 0 },
    { 'l',  "list",         'l', 0 },
    { 'v',  "verbose",      'v', ACUTEST_CMDLINE_OPTFLAG_OPTIONALARG_ },
    { 'q',  "quiet",        'q', 0 },
    {  0,   "color",        'c', ACUTEST_CMDLINE_OPTFLAG_OPTIONALARG_ },
    {  0,   "no-color",     'C', 0 },
    { 'h',  "help",         'h', 0 },
    {  0,   "worker",       'w', ACUTEST_CMDLINE_OPTFLAG_REQUIREDARG_ },  /* internal */
    { 'x',  "xml-output",   'x', ACUTEST_CMDLINE_OPTFLAG_REQUIREDARG_ },
    {  0,   NULL,            0,  0 }
};

static int
acutest_cmdline_callback_(int id, const char* arg)
{
    switch(id) {
        case 's':
            acutest_skip_mode_ = 1;
            break;

        case 'e':
            if(arg == NULL || strcmp(arg, "always") == 0) {
                acutest_no_exec_ = 0;
            } else if(strcmp(arg, "never") == 0) {
                acutest_no_exec_ = 1;
            } else if(strcmp(arg, "auto") == 0) {
                /*noop*/
            } else {
                fprintf(stderr, "%s: Unrecognized argument '%s' for option --exec.\n", acutest_argv0_, arg);
                fprintf(stderr, "Try '%s --help' for more information.\n", acutest_argv0_);
                acutest_exit_(2);
            }
            break;

        case 'E':
            acutest_no_exec_ = 1;
            break;

        case 't':
#if defined ACUTEST_WIN_  ||  defined ACUTEST_HAS_POSIX_TIMER_
            if(arg == NULL || strcmp(arg, "real") == 0) {
                acutest_timer_ = 1;
    #ifndef ACUTEST_WIN_
            } else if(strcmp(arg, "cpu") == 0) {
                acutest_timer_ = 2;
    #endif
            } else {
                fprintf(stderr, "%s: Unrecognized argument '%s' for option --time.\n", acutest_argv0_, arg);
                fprintf(stderr, "Try '%s --help' for more information.\n", acutest_argv0_);
                acutest_exit_(2);
            }
#endif
            break;

        case 'S':
            acutest_no_summary_ = 1;
            break;

        case 'T':
            acutest_tap_ = 1;
            break;

        case 'l':
            acutest_list_names_();
            acutest_exit_(0);
            break;

        case 'v':
            acutest_verbose_level_ = (arg != NULL ? atoi(arg) : acutest_verbose_level_+1);
            break;

        case 'q':
            acutest_verbose_level_ = 0;
            break;

        case 'c':
            if(arg == NULL || strcmp(arg, "always") == 0) {
                acutest_colorize_ = 1;
            } else if(strcmp(arg, "never") == 0) {
                acutest_colorize_ = 0;
            } else if(strcmp(arg, "auto") == 0) {
                /*noop*/
            } else {
                fprintf(stderr, "%s: Unrecognized argument '%s' for option --color.\n", acutest_argv0_, arg);
                fprintf(stderr, "Try '%s --help' for more information.\n", acutest_argv0_);
                acutest_exit_(2);
            }
            break;

        case 'C':
            acutest_colorize_ = 0;
            break;

        case 'h':
            acutest_help_();
            acutest_exit_(0);
            break;

        case 'w':
            acutest_worker_ = 1;
            acutest_worker_index_ = atoi(arg);
            break;
        case 'x':
            acutest_xml_output_ = fopen(arg, "w");
            if (!acutest_xml_output_) {
                fprintf(stderr, "Unable to open '%s': %s\n", arg, strerror(errno));
                acutest_exit_(2);
            }
            break;

        case 0:
            if(acutest_lookup_(arg) == 0) {
                fprintf(stderr, "%s: Unrecognized unit test '%s'\n", acutest_argv0_, arg);
                fprintf(stderr, "Try '%s --list' for list of unit tests.\n", acutest_argv0_);
                acutest_exit_(2);
            }
            break;

        case ACUTEST_CMDLINE_OPTID_UNKNOWN_:
            fprintf(stderr, "Unrecognized command line option '%s'.\n", arg);
            fprintf(stderr, "Try '%s --help' for more information.\n", acutest_argv0_);
            acutest_exit_(2);
            break;

        case ACUTEST_CMDLINE_OPTID_MISSINGARG_:
            fprintf(stderr, "The command line option '%s' requires an argument.\n", arg);
            fprintf(stderr, "Try '%s --help' for more information.\n", acutest_argv0_);
            acutest_exit_(2);
            break;

        case ACUTEST_CMDLINE_OPTID_BOGUSARG_:
            fprintf(stderr, "The command line option '%s' does not expect an argument.\n", arg);
            fprintf(stderr, "Try '%s --help' for more information.\n", acutest_argv0_);
            acutest_exit_(2);
            break;
    }

    return 0;
}


#ifdef ACUTEST_LINUX_
static int
acutest_is_tracer_present_(void)
{
    /* Must be large enough so the line 'TracerPid: ${PID}' can fit in. */
    static const int OVERLAP = 32;

    char buf[512];
    int tracer_present = 0;
    int fd;
    size_t n_read = 0;

    fd = open("/proc/self/status", O_RDONLY);
    if(fd == -1)
        return 0;

    while(1) {
        static const char pattern[] = "TracerPid:";
        const char* field;

        while(n_read < sizeof(buf) - 1) {
            ssize_t n;

            n = read(fd, buf + n_read, sizeof(buf) - 1 - n_read);
            if(n <= 0)
                break;
            n_read += n;
        }
        buf[n_read] = '\0';

        field = strstr(buf, pattern);
        if(field != NULL  &&  field < buf + sizeof(buf) - OVERLAP) {
            pid_t tracer_pid = (pid_t) atoi(field + sizeof(pattern) - 1);
            tracer_present = (tracer_pid != 0);
            break;
        }

        if(n_read == sizeof(buf) - 1) {
            /* Move the tail with the potentially incomplete line we're looking
             * for to the beginning of the buffer. */
            memmove(buf, buf + sizeof(buf) - 1 - OVERLAP, OVERLAP);
            n_read = OVERLAP;
        } else {
            break;
        }
    }

    close(fd);
    return tracer_present;
}
#endif

#ifdef ACUTEST_MACOS_
static bool
acutest_AmIBeingDebugged(void)
{
    int junk;
    int mib[4];
    struct kinfo_proc info;
    size_t size;

    // Initialize the flags so that, if sysctl fails for some bizarre
    // reason, we get a predictable result.
    info.kp_proc.p_flag = 0;

    // Initialize mib, which tells sysctl the info we want, in this case
    // we're looking for information about a specific process ID.
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();

    // Call sysctl.
    size = sizeof(info);
    junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
    assert(junk == 0);

    // We're being debugged if the P_TRACED flag is set.
    return ( (info.kp_proc.p_flag & P_TRACED) != 0 );
}
#endif

int
main(int argc, char** argv)
{
    int i;

    acutest_argv0_ = argv[0];

#if defined ACUTEST_UNIX_
    acutest_colorize_ = isatty(STDOUT_FILENO);
#elif defined ACUTEST_WIN_
 #if defined _BORLANDC_
    acutest_colorize_ = isatty(_fileno(stdout));
 #else
    acutest_colorize_ = _isatty(_fileno(stdout));
 #endif
#else
    acutest_colorize_ = 0;
#endif

    /* Count all test units */
    acutest_list_size_ = 0;
    for(i = 0; acutest_list_[i].func != NULL; i++)
        acutest_list_size_++;

    acutest_test_data_ = (struct acutest_test_data_*)calloc(acutest_list_size_, sizeof(struct acutest_test_data_));
    if(acutest_test_data_ == NULL) {
        fprintf(stderr, "Out of memory.\n");
        acutest_exit_(2);
    }

    /* Parse options */
    acutest_cmdline_read_(acutest_cmdline_options_, argc, argv, acutest_cmdline_callback_);

    /* Initialize the proper timer. */
    acutest_timer_init_();

#if defined(ACUTEST_WIN_)
    SetUnhandledExceptionFilter(acutest_seh_exception_filter_);
#ifdef _MSC_VER
    _set_abort_behavior(0, _WRITE_ABORT_MSG);
#endif
#endif

    /* By default, we want to run all tests. */
    if(acutest_count_ == 0) {
        for(i = 0; acutest_list_[i].func != NULL; i++)
            acutest_remember_(i);
    }

    /* Guess whether we want to run unit tests as child processes. */
    if(acutest_no_exec_ < 0) {
        acutest_no_exec_ = 0;

        if(acutest_count_ <= 1) {
            acutest_no_exec_ = 1;
        } else {
#ifdef ACUTEST_WIN_
            if(IsDebuggerPresent())
                acutest_no_exec_ = 1;
#endif
#ifdef ACUTEST_LINUX_
            if(acutest_is_tracer_present_())
                acutest_no_exec_ = 1;
#endif
#ifdef ACUTEST_MACOS_
            if(acutest_AmIBeingDebugged())
                acutest_no_exec_ = 1;
#endif
#ifdef RUNNING_ON_VALGRIND
            /* RUNNING_ON_VALGRIND is provided by optionally included <valgrind.h> */
            if(RUNNING_ON_VALGRIND)
                acutest_no_exec_ = 1;
#endif
        }
    }

    if(acutest_tap_) {
        /* TAP requires we know test result ("ok", "not ok") before we output
         * anything about the test, and this gets problematic for larger verbose
         * levels. */
        if(acutest_verbose_level_ > 2)
            acutest_verbose_level_ = 2;

        /* TAP harness should provide some summary. */
        acutest_no_summary_ = 1;

        if(!acutest_worker_)
            printf("1..%d\n", (int) acutest_count_);
    }

    int index = acutest_worker_index_;
    for(i = 0; acutest_list_[i].func != NULL; i++) {
        int run = (acutest_test_data_[i].flags & ACUTEST_FLAG_RUN_);
        if (acutest_skip_mode_) /* Run all tests except those listed. */
            run = !run;
        if(run)
            acutest_run_(&acutest_list_[i], index++, i);
    }

    /* Write a summary */
    if(!acutest_no_summary_ && acutest_verbose_level_ >= 1) {
        if(acutest_verbose_level_ >= 3) {
            acutest_colored_printf_(ACUTEST_COLOR_DEFAULT_INTENSIVE_, "Summary:\n");

            printf("  Count of all unit tests:     %4d\n", (int) acutest_list_size_);
            printf("  Count of run unit tests:     %4d\n", acutest_stat_run_units_);
            printf("  Count of failed unit tests:  %4d\n", acutest_stat_failed_units_);
            printf("  Count of skipped unit tests: %4d\n", (int) acutest_list_size_ - acutest_stat_run_units_);
        }

        if(acutest_stat_failed_units_ == 0) {
            acutest_colored_printf_(ACUTEST_COLOR_GREEN_INTENSIVE_, "SUCCESS:");
            printf(" All unit tests have passed.\n");
        } else {
            acutest_colored_printf_(ACUTEST_COLOR_RED_INTENSIVE_, "FAILED:");
            printf(" %d of %d unit tests %s failed.\n",
                    acutest_stat_failed_units_, acutest_stat_run_units_,
                    (acutest_stat_failed_units_ == 1) ? "has" : "have");
        }

        if(acutest_verbose_level_ >= 3)
            printf("\n");
    }

    if (acutest_xml_output_) {
#if defined ACUTEST_UNIX_
        char *suite_name = basename(argv[0]);
#elif defined ACUTEST_WIN_
        char suite_name[_MAX_FNAME];
        _splitpath(argv[0], NULL, NULL, suite_name, NULL);
#else
        const char *suite_name = argv[0];
#endif
        fprintf(acutest_xml_output_, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        fprintf(acutest_xml_output_, "<testsuite name=\"%s\" tests=\"%d\" errors=\"%d\" failures=\"%d\" skip=\"%d\">\n",
            suite_name, (int)acutest_list_size_, acutest_stat_failed_units_, acutest_stat_failed_units_,
            (int)acutest_list_size_ - acutest_stat_run_units_);
        for(i = 0; acutest_list_[i].func != NULL; i++) {
            struct acutest_test_data_ *details = &acutest_test_data_[i];
            fprintf(acutest_xml_output_, "  <testcase name=\"%s\" time=\"%.2f\">\n", acutest_list_[i].name, details->duration);
            if (details->flags & ACUTEST_FLAG_FAILURE_)
                fprintf(acutest_xml_output_, "    <failure />\n");
            if (!(details->flags & ACUTEST_FLAG_FAILURE_) && !(details->flags & ACUTEST_FLAG_SUCCESS_))
                fprintf(acutest_xml_output_, "    <skipped />\n");
            fprintf(acutest_xml_output_, "  </testcase>\n");
        }
        fprintf(acutest_xml_output_, "</testsuite>\n");
        fclose(acutest_xml_output_);
    }

    acutest_cleanup_();

    return (acutest_stat_failed_units_ == 0) ? 0 : 1;
}


#endif  /* #ifndef TEST_NO_MAIN */

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

#ifdef __cplusplus
    }  /* extern "C" */
#endif

#endif  /* #ifndef ACUTEST_H */
