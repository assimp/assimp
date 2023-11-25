#ifndef ASSIMP_REVISION_H_INC
#define ASSIMP_REVISION_H_INC

#define GitVersion 0x77a8f019
#define GitBranch "heads/master"

#define VER_MAJOR 5
#define VER_MINOR 3
#define VER_PATCH 0
#define VER_BUILD 0

#define STR_HELP(x) #x
#define STR(x) STR_HELP(x)

#define VER_FILEVERSION             VER_MAJOR,VER_MINOR,VER_PATCH,VER_BUILD
#if (GitVersion == 0)
#define VER_FILEVERSION_STR         STR(VER_MAJOR) "." STR(VER_MINOR) "." STR(VER_PATCH) "." STR(VER_BUILD)
#else
#define VER_FILEVERSION_STR         STR(VER_MAJOR) "." STR(VER_MINOR) "." STR(VER_PATCH) "." STR(VER_BUILD) " (Commit 77a8f019)"
#endif
#define VER_COPYRIGHT_STR           "\xA9 2006-2023"

#ifdef  NDEBUG
#define VER_ORIGINAL_FILENAME_STR   "assimp-vc143-mt.dll"
#else
#define VER_ORIGINAL_FILENAME_STR   "assimp-vc143-mtd.dll"
#endif //  NDEBUG

#endif // ASSIMP_REVISION_H_INC
