#pragma once

#if defined(__ANDROID__)
#include <sstream>
#include <android/log.h>
#endif // #if defined(__ANDROID__)

#if !defined(TINYUSDZ_PRODUCTION_BUILD) && !defined(TINYUSDZ_FUZZER_BUILD)
#if defined(TINYUSDZ_DEBUG_PRINT)
#define TINYUSDZ_LOCAL_DEBUG_PRINT
#endif
#endif

#if defined(TINYUSDZ_PRODUCTION_BUILD)
// Do not include full filepath for privacy.

#define PUSH_ERROR_AND_RETURN(s)                         \
  do {                                                   \
    std::ostringstream ss_e;                             \
    ss_e << "[error]"                                    \
         << ":" << __func__ << "():" << __LINE__ << " "; \
    ss_e << s << "\n";                                   \
    PushError(ss_e.str());                               \
    return false;                                        \
  } while (0)

#define PUSH_ERROR_AND_RETURN_TAG(tag, s)                                    \
  do {                                                                       \
    std::ostringstream ss_e;                                                 \
    ss_e << "[error]" << tag << ":" << __func__ << "():" << __LINE__ << " "; \
    ss_e << s << "\n";                                                       \
    PushError(ss_e.str());                                                   \
    return false;                                                            \
  } while (0)

#define PUSH_ERROR(s)                                    \
  do {                                                   \
    std::ostringstream ss_e;                             \
    ss_e << "[error]"                                    \
         << ":" << __func__ << "():" << __LINE__ << " "; \
    ss_e << s << "\n";                                   \
    PushError(ss_e.str());                               \
  } while (0)

#define PUSH_WARN(s)                                     \
  do {                                                   \
    std::ostringstream ss_w;                             \
    ss_w << "[warn]"                                     \
         << ":" << __func__ << "():" << __LINE__ << " "; \
    ss_w << s << "\n";                                   \
    PushWarn(ss_w.str());                                \
  } while (0)

#else  // TINYUSDZ_PRODUCTION_BUILD

#define PUSH_ERROR_AND_RETURN(s)                                          \
  do {                                                                    \
    std::ostringstream ss_e;                                              \
    ss_e << "[error]" << __FILE__ << ":" << __func__ << "():" << __LINE__ \
         << " ";                                                          \
    ss_e << s << "\n";                                                    \
    PushError(ss_e.str());                                                \
    return false;                                                         \
  } while (0)

#define PUSH_ERROR_AND_RETURN_TAG(tag, s)                   \
  do {                                                      \
    std::ostringstream ss_e;                                \
    ss_e << "[error]" << __FILE__ << tag << ":" << __func__ \
         << "():" << __LINE__ << " ";                       \
    ss_e << s << "\n";                                      \
    PushError(ss_e.str());                                  \
    return false;                                           \
  } while (0)

#define PUSH_ERROR(s)                                                     \
  do {                                                                    \
    std::ostringstream ss_e;                                              \
    ss_e << "[error]" << __FILE__ << ":" << __func__ << "():" << __LINE__ \
         << " ";                                                          \
    ss_e << s << "\n";                                                    \
    PushError(ss_e.str());                                                \
  } while (0)

#define PUSH_WARN(s)                                                     \
  do {                                                                   \
    std::ostringstream ss_w;                                             \
    ss_w << "[warn]" << __FILE__ << ":" << __func__ << "():" << __LINE__ \
         << " ";                                                         \
    ss_w << s << "\n";                                                   \
    PushWarn(ss_w.str());                                                \
  } while (0)

#endif  // TINYUSDZ_PRODUCTION_BUILD

#if defined(TINYUSDZ_LOCAL_DEBUG_PRINT)
#if defined(__ANDROID__)
#define TINYUSDZLOGT(tag, ...)  ((void)__android_log_print(ANDROID_LOG_DEBUG,   tag, __VA_ARGS__))
#define TINYUSDZLOG0(tag, ...)  ((void)__android_log_print(ANDROID_LOG_DEFAULT, tag, __VA_ARGS__))
#define TINYUSDZLOGD(tag, ...)  ((void)__android_log_print(ANDROID_LOG_DEBUG,   tag, __VA_ARGS__))
#define TINYUSDZLOGI(tag, ...)  ((void)__android_log_print(ANDROID_LOG_INFO,    tag, __VA_ARGS__))
#define TINYUSDZLOGW(tag, ...)  ((void)__android_log_print(ANDROID_LOG_WARN,    tag, __VA_ARGS__))
#define TINYUSDZLOGE(tag, ...)  ((void)__android_log_print(ANDROID_LOG_ERROR,   tag, __VA_ARGS__))
#define DCOUT(x)                                               \
  do {                                                         \
    std::stringstream ss;                                      \
    ss << __FILE__ << ":" << __func__ << ":"                   \
              << std::to_string(__LINE__) << " " << x << "\n"; \
    TINYUSDZLOGE("tinyusdz", "%s", ss.str().c_str());          \
  } while (false)
#else
#define DCOUT(x)                                               \
  do {                                                         \
    std::cout << __FILE__ << ":" << __func__ << ":"            \
              << std::to_string(__LINE__) << " " << x << "\n"; \
  } while (false)
#endif // #if defined(__ANDROID__)
#else
#if defined(__ANDROID__)
#define TINYUSDZLOGT(tag, ...)
#define TINYUSDZLOG0(tag, ...)
#define TINYUSDZLOGD(tag, ...)
#define TINYUSDZLOGI(tag, ...)
#define TINYUSDZLOGW(tag, ...)
#define TINYUSDZLOGE(tag, ...)
#endif // #if defined(__ANDROID__)
#define DCOUT(x)
#endif // #if defined(TINYUSDZ_LOCAL_DEBUG_PRINT)

// Simple auto-free class
// Use this class when saving stack size is required(e.g. recursive function call).
// T must have default constructor
template<typename T>
class AutoFree {
 private:
  T *_v{nullptr}; 

 public:

  AutoFree() : _v(new T()) { 
  }

  ~AutoFree() {
    delete _v;
  }

  T &value() { return *_v; }
};
