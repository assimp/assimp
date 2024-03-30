/*
  Native File Dialog Extended
  Repository: https://github.com/btzy/nativefiledialog-extended
  License: Zlib
  Author: Bernard Teo

  This header is a thin C++ wrapper for nfd.h.
  C++ projects can choose to use this header instead of nfd.h directly.

  Refer to documentation on nfd.h for instructions on how to use these functions.
*/

#ifndef _NFD_HPP
#define _NFD_HPP

#include <nfd.h>
#include <cstddef>  // for std::size_t
#include <memory>   // for std::unique_ptr
#ifdef NFD_THROWS_EXCEPTIONS
#include <stdexcept>
#endif

namespace NFD {

inline nfdresult_t Init() noexcept {
    return ::NFD_Init();
}

inline void Quit() noexcept {
    ::NFD_Quit();
}

inline void FreePath(nfdnchar_t* outPath) noexcept {
    ::NFD_FreePathN(outPath);
}

inline nfdresult_t OpenDialog(nfdnchar_t*& outPath,
                              const nfdnfilteritem_t* filterList = nullptr,
                              nfdfiltersize_t filterCount = 0,
                              const nfdnchar_t* defaultPath = nullptr) noexcept {
    return ::NFD_OpenDialogN(&outPath, filterList, filterCount, defaultPath);
}

inline nfdresult_t OpenDialogMultiple(const nfdpathset_t*& outPaths,
                                      const nfdnfilteritem_t* filterList = nullptr,
                                      nfdfiltersize_t filterCount = 0,
                                      const nfdnchar_t* defaultPath = nullptr) noexcept {
    return ::NFD_OpenDialogMultipleN(&outPaths, filterList, filterCount, defaultPath);
}

inline nfdresult_t SaveDialog(nfdnchar_t*& outPath,
                              const nfdnfilteritem_t* filterList = nullptr,
                              nfdfiltersize_t filterCount = 0,
                              const nfdnchar_t* defaultPath = nullptr,
                              const nfdnchar_t* defaultName = nullptr) noexcept {
    return ::NFD_SaveDialogN(&outPath, filterList, filterCount, defaultPath, defaultName);
}

inline nfdresult_t PickFolder(nfdnchar_t*& outPath,
                              const nfdnchar_t* defaultPath = nullptr) noexcept {
    return ::NFD_PickFolderN(&outPath, defaultPath);
}

inline const char* GetError() noexcept {
    return ::NFD_GetError();
}

inline void ClearError() noexcept {
    ::NFD_ClearError();
}

namespace PathSet {
inline nfdresult_t Count(const nfdpathset_t* pathSet, nfdpathsetsize_t& count) noexcept {
    return ::NFD_PathSet_GetCount(pathSet, &count);
}

inline nfdresult_t GetPath(const nfdpathset_t* pathSet,
                           nfdpathsetsize_t index,
                           nfdnchar_t*& outPath) noexcept {
    return ::NFD_PathSet_GetPathN(pathSet, index, &outPath);
}

inline void FreePath(nfdnchar_t* filePath) noexcept {
    ::NFD_PathSet_FreePathN(filePath);
}

inline void Free(const nfdpathset_t* pathSet) noexcept {
    ::NFD_PathSet_Free(pathSet);
}
}  // namespace PathSet

#ifdef NFD_DIFFERENT_NATIVE_FUNCTIONS
/* we need the C++ bindings for the UTF-8 functions as well, because there are different functions
 * for them */

inline void FreePath(nfdu8char_t* outPath) noexcept {
    ::NFD_FreePathU8(outPath);
}

inline nfdresult_t OpenDialog(nfdu8char_t*& outPath,
                              const nfdu8filteritem_t* filterList = nullptr,
                              nfdfiltersize_t count = 0,
                              const nfdu8char_t* defaultPath = nullptr) noexcept {
    return ::NFD_OpenDialogU8(&outPath, filterList, count, defaultPath);
}

inline nfdresult_t OpenDialogMultiple(const nfdpathset_t*& outPaths,
                                      const nfdu8filteritem_t* filterList = nullptr,
                                      nfdfiltersize_t count = 0,
                                      const nfdu8char_t* defaultPath = nullptr) noexcept {
    return ::NFD_OpenDialogMultipleU8(&outPaths, filterList, count, defaultPath);
}

inline nfdresult_t SaveDialog(nfdu8char_t*& outPath,
                              const nfdu8filteritem_t* filterList = nullptr,
                              nfdfiltersize_t count = 0,
                              const nfdu8char_t* defaultPath = nullptr,
                              const nfdu8char_t* defaultName = nullptr) noexcept {
    return ::NFD_SaveDialogU8(&outPath, filterList, count, defaultPath, defaultName);
}

inline nfdresult_t PickFolder(nfdu8char_t*& outPath,
                              const nfdu8char_t* defaultPath = nullptr) noexcept {
    return ::NFD_PickFolderU8(&outPath, defaultPath);
}

namespace PathSet {
inline nfdresult_t GetPath(const nfdpathset_t* pathSet,
                           nfdpathsetsize_t index,
                           nfdu8char_t*& outPath) noexcept {
    return ::NFD_PathSet_GetPathU8(pathSet, index, &outPath);
}
inline void FreePath(nfdu8char_t* filePath) noexcept {
    ::NFD_PathSet_FreePathU8(filePath);
}
}  // namespace PathSet
#endif

// smart objects

class Guard {
   public:
#ifndef NFD_THROWS_EXCEPTIONS
    inline Guard() noexcept {
        Init();  // always assume that initialization succeeds
    }
#else
    inline Guard() {
        if (!Init()) {
            throw std::runtime_error(GetError());
        }
    }
#endif
    inline ~Guard() noexcept { Quit(); }

    // Not allowed to copy or move this class
    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;
};

template <typename T>
struct PathDeleter {
    inline void operator()(T* ptr) const noexcept { FreePath(ptr); }
};

typedef std::unique_ptr<nfdchar_t, PathDeleter<nfdchar_t>> UniquePath;
typedef std::unique_ptr<nfdnchar_t, PathDeleter<nfdnchar_t>> UniquePathN;
typedef std::unique_ptr<nfdu8char_t, PathDeleter<nfdu8char_t>> UniquePathU8;

struct PathSetDeleter {
    inline void operator()(const nfdpathset_t* ptr) const noexcept { PathSet::Free(ptr); }
};

typedef std::unique_ptr<const nfdpathset_t, PathSetDeleter> UniquePathSet;

template <typename T>
struct PathSetPathDeleter {
    inline void operator()(T* ptr) const noexcept { PathSet::FreePath(ptr); }
};

typedef std::unique_ptr<nfdchar_t, PathSetPathDeleter<nfdchar_t>> UniquePathSetPath;
typedef std::unique_ptr<nfdnchar_t, PathSetPathDeleter<nfdnchar_t>> UniquePathSetPathN;
typedef std::unique_ptr<nfdu8char_t, PathSetPathDeleter<nfdu8char_t>> UniquePathSetPathU8;

inline nfdresult_t OpenDialog(UniquePathN& outPath,
                              const nfdnfilteritem_t* filterList = nullptr,
                              nfdfiltersize_t filterCount = 0,
                              const nfdnchar_t* defaultPath = nullptr) noexcept {
    nfdnchar_t* out;
    nfdresult_t res = OpenDialog(out, filterList, filterCount, defaultPath);
    if (res == NFD_OKAY) {
        outPath.reset(out);
    }
    return res;
}

inline nfdresult_t OpenDialogMultiple(UniquePathSet& outPaths,
                                      const nfdnfilteritem_t* filterList = nullptr,
                                      nfdfiltersize_t filterCount = 0,
                                      const nfdnchar_t* defaultPath = nullptr) noexcept {
    const nfdpathset_t* out;
    nfdresult_t res = OpenDialogMultiple(out, filterList, filterCount, defaultPath);
    if (res == NFD_OKAY) {
        outPaths.reset(out);
    }
    return res;
}

inline nfdresult_t SaveDialog(UniquePathN& outPath,
                              const nfdnfilteritem_t* filterList = nullptr,
                              nfdfiltersize_t filterCount = 0,
                              const nfdnchar_t* defaultPath = nullptr,
                              const nfdnchar_t* defaultName = nullptr) noexcept {
    nfdnchar_t* out;
    nfdresult_t res = SaveDialog(out, filterList, filterCount, defaultPath, defaultName);
    if (res == NFD_OKAY) {
        outPath.reset(out);
    }
    return res;
}

inline nfdresult_t PickFolder(UniquePathN& outPath,
                              const nfdnchar_t* defaultPath = nullptr) noexcept {
    nfdnchar_t* out;
    nfdresult_t res = PickFolder(out, defaultPath);
    if (res == NFD_OKAY) {
        outPath.reset(out);
    }
    return res;
}

#ifdef NFD_DIFFERENT_NATIVE_FUNCTIONS
inline nfdresult_t OpenDialog(UniquePathU8& outPath,
                              const nfdu8filteritem_t* filterList = nullptr,
                              nfdfiltersize_t filterCount = 0,
                              const nfdu8char_t* defaultPath = nullptr) noexcept {
    nfdu8char_t* out;
    nfdresult_t res = OpenDialog(out, filterList, filterCount, defaultPath);
    if (res == NFD_OKAY) {
        outPath.reset(out);
    }
    return res;
}

inline nfdresult_t OpenDialogMultiple(UniquePathSet& outPaths,
                                      const nfdu8filteritem_t* filterList = nullptr,
                                      nfdfiltersize_t filterCount = 0,
                                      const nfdu8char_t* defaultPath = nullptr) noexcept {
    const nfdpathset_t* out;
    nfdresult_t res = OpenDialogMultiple(out, filterList, filterCount, defaultPath);
    if (res == NFD_OKAY) {
        outPaths.reset(out);
    }
    return res;
}

inline nfdresult_t SaveDialog(UniquePathU8& outPath,
                              const nfdu8filteritem_t* filterList = nullptr,
                              nfdfiltersize_t filterCount = 0,
                              const nfdu8char_t* defaultPath = nullptr,
                              const nfdu8char_t* defaultName = nullptr) noexcept {
    nfdu8char_t* out;
    nfdresult_t res = SaveDialog(out, filterList, filterCount, defaultPath, defaultName);
    if (res == NFD_OKAY) {
        outPath.reset(out);
    }
    return res;
}

inline nfdresult_t PickFolder(UniquePathU8& outPath,
                              const nfdu8char_t* defaultPath = nullptr) noexcept {
    nfdu8char_t* out;
    nfdresult_t res = PickFolder(out, defaultPath);
    if (res == NFD_OKAY) {
        outPath.reset(out);
    }
    return res;
}
#endif

namespace PathSet {
inline nfdresult_t Count(const UniquePathSet& uniquePathSet, nfdpathsetsize_t& count) noexcept {
    return Count(uniquePathSet.get(), count);
}
inline nfdresult_t GetPath(const UniquePathSet& uniquePathSet,
                           nfdpathsetsize_t index,
                           UniquePathSetPathN& outPath) noexcept {
    nfdnchar_t* out;
    nfdresult_t res = GetPath(uniquePathSet.get(), index, out);
    if (res == NFD_OKAY) {
        outPath.reset(out);
    }
    return res;
}
#ifdef NFD_DIFFERENT_NATIVE_FUNCTIONS
inline nfdresult_t GetPath(const UniquePathSet& uniquePathSet,
                           nfdpathsetsize_t index,
                           UniquePathSetPathU8& outPath) noexcept {
    nfdu8char_t* out;
    nfdresult_t res = GetPath(uniquePathSet.get(), index, out);
    if (res == NFD_OKAY) {
        outPath.reset(out);
    }
    return res;
}
#endif
}  // namespace PathSet

}  // namespace NFD

#endif
