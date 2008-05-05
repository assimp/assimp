

/** @file Helper macros for the library 
 */
#ifndef AI_DEF_H_INC
#define AI_DEF_H_INC


/** Namespace helper macros for c++ compilers
 */
#ifdef __cplusplus
  #define AI_NAMESPACE_START namespace Assimp {
  #define AI_NAMESPACE_END };
#else
  #define AI_NAMESPACE_START
  #define AI_NAMESPACE_END
#endif

#ifdef _DEBUG
#  define ASSIMP_DEBUG
#endif

#endif //!!AI_DEF_H_INC
