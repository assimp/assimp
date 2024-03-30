#ifndef BVH_PLATFORM_HPP
#define BVH_PLATFORM_HPP

#include <cstddef>
#include <cassert>

#ifdef _OPENMP
#include <omp.h>
#endif

#if defined(__GNUC__) || defined(__clang__)
#define bvh_restrict      __restrict
#define bvh_always_inline __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define bvh_restrict      __restrict
#define bvh_always_inline __forceinline
#else
#define bvh_restrict
#define bvh_always_inline inline
#endif

#if defined(__GNUC__) || defined(__clang__)
#define bvh_likely(x)   __builtin_expect(x, true)
#define bvh_unlikely(x) __builtin_expect(x, false)
#else
#define bvh_likely(x)   x
#define bvh_unlikely(x) x
#endif

namespace bvh {

#ifdef _OPENMP
inline size_t get_thread_count() { return omp_get_num_threads(); }
inline size_t get_thread_id()    { return omp_get_thread_num(); }
inline void assert_not_in_parallel() { assert(omp_get_level() == 0); }
inline void assert_in_parallel() { assert(omp_get_level() > 0); }
#else
inline constexpr size_t get_thread_count() { return 1; }
inline constexpr size_t get_thread_id()    { return 0; }
inline void assert_not_in_parallel() {}
inline void assert_in_parallel() {}
#endif

} // namespace bvh

#endif
