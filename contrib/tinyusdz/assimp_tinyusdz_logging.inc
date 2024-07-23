/**
 * Usage
 *   Add line below all other #include statements:
 *     #include "../../../assimp_tinyusdz_logging.inc"
 *   to files:
 *    - contrib/tinyusdz/tinyusdz_repo/src/tydra/render-data.cc
 *    - contrib/tinyusdz/tinyusdz_repo/src/tydra/scene-access.cc
 */
#pragma once

#if defined(__ANDROID__)
#include <sstream>
#include <android/log.h>

#define TINYUSDZLOGT(tag, ...)  ((void)__android_log_print(ANDROID_LOG_DEBUG,   tag, __VA_ARGS__))
#define TINYUSDZLOG0(tag, ...)  ((void)__android_log_print(ANDROID_LOG_DEFAULT, tag, __VA_ARGS__))
#define TINYUSDZLOGD(tag, ...)  ((void)__android_log_print(ANDROID_LOG_DEBUG,   tag, __VA_ARGS__))
#define TINYUSDZLOGI(tag, ...)  ((void)__android_log_print(ANDROID_LOG_INFO,    tag, __VA_ARGS__))
#define TINYUSDZLOGW(tag, ...)  ((void)__android_log_print(ANDROID_LOG_WARN,    tag, __VA_ARGS__))
#define TINYUSDZLOGE(tag, ...)  ((void)__android_log_print(ANDROID_LOG_ERROR,   tag, __VA_ARGS__))
#else
#define TINYUSDZLOGT(tag, ...)
#define TINYUSDZLOG0(tag, ...)
#define TINYUSDZLOGD(tag, ...)
#define TINYUSDZLOGI(tag, ...)
#define TINYUSDZLOGW(tag, ...)
#define TINYUSDZLOGE(tag, ...)
#endif // #if defined(__ANDROID__)

#if defined(TINYUSDZ_LOCAL_DEBUG_PRINT)
#if defined(__ANDROID__)
#if defined(ASSIMP_USD_VERBOSE_LOGS)
// Works well but _extremely_ verbose
#define DCOUT(x)                                               \
  do {                                                         \
    std::stringstream ss;                                      \
    ss << __FILE__ << ":" << __func__ << ":"                   \
              << std::to_string(__LINE__) << " " << x << "\n"; \
    TINYUSDZLOGE("tinyusdz", "%s", ss.str().c_str());          \
  } while (false)
#else // defined(ASSIMP_USD_VERBOSE_LOGS)
// Silent version
#define DCOUT(x)                                               \
  do {                                                         \
    std::stringstream ss;                                      \
    ss << __FILE__ << ":" << __func__ << ":"                   \
              << std::to_string(__LINE__) << " " << x << "\n"; \
  } while (false)
#endif // defined(ASSIMP_USD_VERBOSE_LOGS)
#else // defined(__ANDROID__)
#define DCOUT(x)                                               \
  do {                                                         \
    std::cout << __FILE__ << ":" << __func__ << ":"            \
              << std::to_string(__LINE__) << " " << x << "\n"; \
  } while (false)
#endif // #if defined(__ANDROID__)
#endif // #if defined(TINYUSDZ_LOCAL_DEBUG_PRINT)