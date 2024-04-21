// SPDX-License-Identifier: Apache 2.0
// Copyright 2022 - 2023, Syoyo Fujita.
// Copyright 2023 - Present, Light Transport Entertainment Inc.
// 
#include <algorithm>
#include <fstream>

#ifdef _WIN32

#ifdef _MSC_VER
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>  // include API for expanding a file path
#include <io.h>

#ifndef TINYUSDZ_MMAP_SUPPORTED
#define TINYUSDZ_MMAP_SUPPORTED (1)
#endif


#ifdef _MSC_VER
#undef NOMINMAX
#endif

#undef WIN32_LEAN_AND_MEAN

#if defined(__GLIBCXX__)  // mingw

#include <fcntl.h>  // _O_RDONLY

#include <ext/stdio_filebuf.h>  // fstream (all sorts of IO stuff) + stdio_filebuf (=streambuf)

#endif  // __GLIBCXX__

#else  // !_WIN32

#if defined(TINYUSDZ_BUILD_IOS) || defined(TARGET_OS_IPHONE) || \
    defined(TARGET_IPHONE_SIMULATOR) || defined(__ANDROID__) || \
    defined(__EMSCRIPTEN__) || defined(__wasi__)

// non posix

// TODO: Add mmmap or similar feature support to these system.

#else

// Assume Posix
#include <wordexp.h>

#include <sys/stat.h>
#include <sys/mman.h>

#ifndef TINYUSDZ_MMAP_SUPPORTED
#define TINYUSDZ_MMAP_SUPPORTED (1)
#endif


#endif

#endif  // _WIN32

#ifndef TINYUSDZ_MMAP_SUPPORTED
#define TINYUSDZ_MMAP_SUPPORTED (0)
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#if !defined(__wasi__)
#include "external/filesystem/include/ghc/filesystem.hpp"
#include "external/glob/single_include/glob/glob.hpp"
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "io-util.hh"
#include "str-util.hh"

namespace tinyusdz {
namespace io {

#ifdef TINYUSDZ_ANDROID_LOAD_FROM_ASSETS
AAssetManager *asset_manager = nullptr;
#endif


bool IsMMapSupported() {
#if TINYUSDZ_MMAP_SUPPORTED
  return true;
#else
  return false;
#endif
}

bool MMapFile(const std::string &filepath, MMapFileHandle *handle, bool writable) {
  (void)filepath;
  (void)handle;
  (void)writable;

#if TINYUSDZ_MMAP_SUPPORTED
#if defined(_WIN32)
#if 0 // TODO
  int fd = open(filepath.c_str(), writable ? O_RDWR : O_RDONLY);
  HANDLE hFile = _get_ofhandle(fd);
  HANDLE hMapping = CreateFileMapping(hFile, nullptr, PAGE_READWRITE, 0, 0, nullptr);
      if (hMapping == nullptr) {
        return false;
      }
      void *data = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
      if (!data) {
        return false;
      }
      CloseHandle(hMapping);
#else
  return false;
#endif
#else // !WIN32
  // assume posix
  FILE *fp = fopen(filepath.c_str(), writable ? "rw" : "r");
  int ret = std::fseek(fp, 0, SEEK_END);
  if (ret != 0) {
    fclose(fp);
    return false;
  } 

  size_t size = size_t(std::ftell(fp));
  std::fseek(fp, 0, SEEK_SET);

  if (size == 0) {
    return false;
  }
  
  int fd = fileno(fp);
  
  int flags = MAP_PRIVATE; // delayed access 
  void *addr = mmap(nullptr, size, writable ? PROT_READ|PROT_WRITE : PROT_READ, flags, fd, 0);
  if (addr == MAP_FAILED) {
    return false;
  }

  handle->addr = reinterpret_cast<uint8_t *>(addr);
  handle->size = size;
  handle->writable = writable;
  handle->filename = filepath;
  close(fd);

  return true;
#endif // !WIN32
#else // !TINYUSDZ_MMAP_SUPPORTED
  return false;
#endif
}

bool UnmapFile(const MMapFileHandle &handle) {
  (void)handle;
#if TINYUSDZ_MMAP_SUPPORTED
#if defined(_WIN32)
  // TODO
  return false;
#else // !WIN32
  if (handle.addr && handle.size) {
    int ret = munmap(reinterpret_cast<void *>(handle.addr), handle.size);  
    // ignore return code for now
    (void)ret;
    return true;
  }
  return false;
#endif
#else // !TINYUSDZ_MMAP_SUPPORTED
  return false;
#endif
}


std::string ExpandFilePath(const std::string &_filepath, void *) {
  std::string filepath = _filepath;
  if (filepath.size() > 2048) {
    // file path too large.
    // TODO: Report warn.
    filepath.resize(2048);
  }

#ifdef _WIN32
  // Assume input `filepath` is encoded in UTF-8
  std::wstring wfilepath = UTF8ToWchar(filepath);
  DWORD wlen = ExpandEnvironmentStringsW(wfilepath.c_str(), nullptr, 0);
  wchar_t *wstr = new wchar_t[wlen];
  ExpandEnvironmentStringsW(wfilepath.c_str(), wstr, wlen);

  std::wstring ws(wstr);
  delete[] wstr;
  return WcharToUTF8(ws);

#else

#if defined(TINYUSDZ_BUILD_IOS) || defined(TARGET_OS_IPHONE) || \
    defined(TARGET_IPHONE_SIMULATOR) || defined(__ANDROID__) || \
    defined(__EMSCRIPTEN__) || defined(__OpenBSD__) || defined(__wasi__)
  // no expansion
  std::string s = filepath;
#else
  std::string s;
  wordexp_t p;

  if (filepath.empty()) {
    return "";
  }

  // Quote the string to keep any spaces in filepath intact.
  std::string quoted_path = "\"" + filepath + "\"";
  // char** w;
  // TODO: wordexp() is a awful API. Implement our own file path expansion
  // routine. Set NOCMD for security.
  int ret = wordexp(quoted_path.c_str(), &p, WRDE_NOCMD);
  if (ret) {
    // err
    s = filepath;
    return s;
  }

  // Use first element only.
  if (p.we_wordv) {
    s = std::string(p.we_wordv[0]);
    wordfree(&p);
  } else {
    s = filepath;
  }

#endif

  return s;
#endif
}

#ifdef _WIN32
std::wstring UTF8ToWchar(const std::string &str) {
  int wstr_size =
      MultiByteToWideChar(CP_UTF8, 0, str.data(), int(str.size()), nullptr, 0);
  std::wstring wstr(size_t(wstr_size), 0);
  MultiByteToWideChar(CP_UTF8, 0, str.data(), int(str.size()), &wstr[0],
                      int(wstr.size()));
  return wstr;
}

std::string WcharToUTF8(const std::wstring &wstr) {
  int str_size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), int(wstr.size()),
                                     nullptr, 0, nullptr, nullptr);
  std::string str(size_t(str_size), 0);
  WideCharToMultiByte(CP_UTF8, 0, wstr.data(), int(wstr.size()), &str[0],
                      int(str.size()), nullptr, nullptr);
  return str;
}
#endif

bool ReadWholeFile(std::vector<uint8_t> *out, std::string *err,
                   const std::string &filepath, size_t filesize_max,
                   void *userdata) {
  (void)userdata;

#ifdef TINYUSDZ_ANDROID_LOAD_FROM_ASSETS
  if (tinyusdz::io::asset_manager) {
    AAsset *asset = AAssetManager_open(asset_manager, filepath.c_str(),
                                       AASSET_MODE_STREAMING);
    if (!asset) {
      if (err) {
        (*err) += "File open error(from AssestManager) : " + filepath + "\n";
      }
      return false;
    }
    size_t size = AAsset_getLength(asset);
    if (size == 0) {
      if (err) {
        (*err) += "Invalid file size : " + filepath +
                  " (does the path point to a directory?)";
      }
      return false;
    }
    out->resize(size);
    AAsset_read(asset, reinterpret_cast<char *>(&out->at(0)), size);
    AAsset_close(asset);
    return true;
  } else {
    if (err) {
      (*err) += "No asset manager specified : " + filepath + "\n";
    }
    return false;
  }

#else
#ifdef _WIN32
#if defined(__GLIBCXX__)  // mingw
  int file_descriptor =
      _wopen(UTF8ToWchar(filepath).c_str(), _O_RDONLY | _O_BINARY);
  __gnu_cxx::stdio_filebuf<char> wfile_buf(file_descriptor, std::ios_base::in);
  std::istream f(&wfile_buf);
#elif defined(_MSC_VER) || defined(_LIBCPP_VERSION)
  // For libcxx, assume _LIBCPP_HAS_OPEN_WITH_WCHAR is defined to accept
  // `wchar_t *`
  std::ifstream f(UTF8ToWchar(filepath).c_str(), std::ifstream::binary);
#else
  // Unknown compiler/runtime
  std::ifstream f(filepath.c_str(), std::ifstream::binary);
#endif
#else
  std::ifstream f(filepath.c_str(), std::ifstream::binary);
#endif
  if (!f) {
    if (err) {
      (*err) += "File open error : " + filepath + "\n";
    }
    return false;
  }

  // For directory(and pipe?), peek() will fail(Posix gnustl/libc++ only)
  int buf = f.peek();
  (void)buf;
  if (!f) {
    if (err) {
      (*err) += "File read error. Maybe empty file or invalid file : " + filepath + "\n";
    }
    return false;
  }

  f.seekg(0, f.end);
  size_t sz = static_cast<size_t>(f.tellg());
  f.seekg(0, f.beg);

  if (int64_t(sz) < 0) {
    if (err) {
      (*err) += "Invalid file size : " + filepath +
                " (does the path point to a directory?)";
    }
    return false;
  } else if (sz == 0) {
    if (err) {
      (*err) += "File is empty : " + filepath + "\n";
    }
    return false;
  } else if (uint64_t(sz) >= uint64_t(std::numeric_limits<int64_t>::max())) {
    // Posixish environment.
    if (err) {
      (*err) += "Invalid File(Pipe or special device?) : " + filepath + "\n";
    }
    return false;
  }

  if ((filesize_max > 0) && (sz > filesize_max)) {
    if (err) {
      (*err) += "File size is too large : " + filepath +
                " sz = " + std::to_string(sz) +
                ", allowed max filesize = " + std::to_string(filesize_max) +
                "\n";
    }
    return false;
  }

  out->resize(sz);
  f.read(reinterpret_cast<char *>(&out->at(0)),
         static_cast<std::streamsize>(sz));

  if (!f) {
    // read failure.
    if (err) {
      (*err) += "Failed to read file: " + filepath + "\n";
    }
    return false;
  }

  return true;
#endif
}

bool ReadFileHeader(std::vector<uint8_t> *out, std::string *err,
                    const std::string &filepath, uint32_t max_read_bytes,
                    void *userdata) {
  (void)userdata;

  // hard limit to 1MB.
  max_read_bytes =
      (std::max)(1u, (std::min)(uint32_t(1024 * 1024), max_read_bytes));

#ifdef TINYUSDZ_ANDROID_LOAD_FROM_ASSETS
  if (tinyusdz::io::asset_manager) {
    AAsset *asset = AAssetManager_open(asset_manager, filepath.c_str(),
                                       AASSET_MODE_STREAMING);
    if (!asset) {
      if (err) {
        (*err) += "File open error(from AssestManager) : " + filepath + "\n";
      }
      return false;
    }
    size_t size = AAsset_getLength(asset);
    if (size == 0) {
      if (err) {
        (*err) += "Invalid file size : " + filepath +
                  " (does the path point to a directory?)";
      }
      return false;
    }

    size = (std::min)(size_t(max_read_bytes), size);
    out->resize(size);
    AAsset_read(asset, reinterpret_cast<char *>(&out->at(0)), size);
    AAsset_close(asset);
    return true;
  } else {
    if (err) {
      (*err) += "No asset manager specified : " + filepath + "\n";
    }
    return false;
  }

#else
#ifdef _WIN32
#if defined(__GLIBCXX__)  // mingw
  int file_descriptor =
      _wopen(UTF8ToWchar(filepath).c_str(), _O_RDONLY | _O_BINARY);
  __gnu_cxx::stdio_filebuf<char> wfile_buf(file_descriptor, std::ios_base::in);
  std::istream f(&wfile_buf);
#elif defined(_MSC_VER) || defined(_LIBCPP_VERSION)
  // For libcxx, assume _LIBCPP_HAS_OPEN_WITH_WCHAR is defined to accept
  // `wchar_t *`
  std::ifstream f(UTF8ToWchar(filepath).c_str(), std::ifstream::binary);
#else
  // Unknown compiler/runtime
  std::ifstream f(filepath.c_str(), std::ifstream::binary);
#endif
#else
  std::ifstream f(filepath.c_str(), std::ifstream::binary);
#endif
  if (!f) {
    if (err) {
      (*err) += "File does not exit or open error : " + filepath + "\n";
    }
    return false;
  }

  // For directory(and pipe?), peek() will fail(Posix gnustl/libc++ only)
  int buf = f.peek();
  (void)buf;
  if (!f) {
    if (err) {
      (*err) += "File read error. Maybe empty file or invalid file : " + filepath + "\n";
    }
    return false;
  }


  f.seekg(0, f.end);
  size_t sz = static_cast<size_t>(f.tellg());
  f.seekg(0, f.beg);

  if (int64_t(sz) < 0) {
    if (err) {
      (*err) += "Invalid file size : " + filepath +
                " (does the path point to a directory?)";
    }
    return false;
  } else if (sz == 0) {
    if (err) {
      (*err) += "File is empty : " + filepath + "\n";
    }
    return false;
  } else if (uint64_t(sz) >= uint64_t(std::numeric_limits<int64_t>::max())) {
    // Posixish environment.
    if (err) {
      (*err) += "Invalid File(Pipe or special device?) : " + filepath + "\n";
    }
    return false;
  }

  sz = (std::min)(size_t(max_read_bytes), sz);

  out->resize(sz);
  f.read(reinterpret_cast<char *>(&out->at(0)),
         static_cast<std::streamsize>(sz));

  if (!f) {
    // read failure.
    if (err) {
      (*err) += "Failed to read file: " + filepath + "\n";
    }
    return false;
  }

  return true;
#endif
}

bool WriteWholeFile(const std::string &filepath, const unsigned char *contents,
                    size_t content_bytes, std::string *err) {
#ifdef _WIN32
#if defined(__GLIBCXX__)  // mingw
  int file_descriptor = _wopen(UTF8ToWchar(filepath).c_str(),
                               _O_CREAT | _O_WRONLY | _O_TRUNC | _O_BINARY);
  __gnu_cxx::stdio_filebuf<char> wfile_buf(
      file_descriptor, std::ios_base::out | std::ios_base::binary);
  std::ostream f(&wfile_buf);
#elif defined(_MSC_VER) || defined(_LIBCPP_VERSION)
  std::ofstream f(UTF8ToWchar(filepath).c_str(), std::ofstream::binary);
#else  // other C++ compiler for win32?
  std::ofstream f(filepath.c_str(), std::ofstream::binary);
#endif
#else
  std::ofstream f(filepath.c_str(), std::ofstream::binary);
#endif
  if (!f) {
    if (err) {
      (*err) += "File open error for writing : " + filepath + "\n";
    }
    return false;
  }

  f.write(reinterpret_cast<const char *>(contents),
          static_cast<std::streamsize>(content_bytes));
  if (!f) {
    if (err) {
      (*err) += "File write error: " + filepath + "\n";
    }
    return false;
  }

  return true;
}

#ifdef _WIN32
bool WriteWholeFile(const std::wstring &filepath, const unsigned char *contents,
                    size_t content_bytes, std::string *err) {
#if defined(__GLIBCXX__)  // mingw
  int file_descriptor =
      _wopen(filepath.c_str(), _O_CREAT | _O_WRONLY | _O_TRUNC | _O_BINARY);
  __gnu_cxx::stdio_filebuf<char> wfile_buf(
      file_descriptor, std::ios_base::out | std::ios_base::binary);
  std::ostream f(&wfile_buf);
#elif defined(_MSC_VER) || defined(_LIBCPP_VERSION)
  // MSVC extension allow wstrng as an argument.
  std::ofstream f(filepath.c_str(), std::ofstream::binary);
#else  // other C++ compiler for win32?
#error "Unsupporte platform"
#endif

  if (!f) {
    if (err) {
      // This would print garbage character...
      // FIXME: First create string in wchar, then convert to wstring?
      (*err) += "File open error for writing : " + WcharToUTF8(filepath) + "\n";
    }
    return false;
  }

  f.write(reinterpret_cast<const char *>(contents),
          static_cast<std::streamsize>(content_bytes));
  if (!f) {
    if (err) {
      (*err) += "File write error: " + WcharToUTF8(filepath) + "\n";
    }
    return false;
  }

  return true;
}
#endif

std::string GetBaseDir(const std::string &filepath) {
  if (filepath.find_last_of("/\\") != std::string::npos)
    return filepath.substr(0, filepath.find_last_of("/\\"));
  return "";
}

std::string GetFileExtension(const std::string &FileName) {
  if (FileName.find_last_of(".") != std::string::npos)
    return FileName.substr(FileName.find_last_of(".") + 1);
  return "";
}

std::string GetBaseFilename(const std::string &filepath) {
  auto idx = filepath.find_last_of("/\\");
  if (idx != std::string::npos) return filepath.substr(idx + 1);
  return filepath;
}

bool IsAbsPath(const std::string &filename) {
  if (filename.size() > 0) {
    if (filename[0] == '/') {
      return true;
    }
  }

  // UNC path?
  if (filename.size() > 2) {
    if ((filename[0] == '\\') && (filename[1] == '\\')) {
      return true;
    }
  }

  // TODO: Windows drive path(e.g. C:\, D:\, ...)

  return false;
}

std::string JoinPath(const std::string &dir, const std::string &filename) {
  if (dir.empty()) {
    return filename;
  } else {
    // check '/'
    char lastChar = *dir.rbegin();

    // TODO: Support more relative path case.

    std::string basedir;
    if (lastChar != '/') {
      basedir = dir + std::string("/");
    } else {
      basedir = dir;
    }

    if (basedir.size()) {
      if (startsWith(filename, "./")) {
        // strip "./"
        return basedir + removePrefix(filename, "./");
      }
      return basedir + filename;
    } else {
      return filename;
    }
  }
}

bool USDFileExists(const std::string &fpath) {
  size_t read_len = 9;  // USD file must be at least 9 bytes or more.

  std::string err;
  std::vector<uint8_t> data;

  if (!ReadFileHeader(&data, &err, fpath, uint32_t(read_len))) {
    return false;
  }

  return true;
}

bool IsUDIMPath(const std::string &path) {
  return SplitUDIMPath(path, nullptr, nullptr);
}

bool SplitUDIMPath(const std::string &path, std::string *pre,
                   std::string *post) {
  std::string tag = "<UDIM>";

  auto rs = std::search(path.begin(), path.end(), tag.begin(), tag.end());
  if (rs == path.end()) {
    return false;
  }

  auto re = std::find_end(path.begin(), path.end(), tag.begin(), tag.end());
  if (re == path.end()) {
    return false;
  }

  // No multiple tags. e.g. diffuse.<UDIM>.<UDIM>.png
  if (rs != re) {
    return false;
  }

  if (pre) {
    (*pre) = std::string(path.begin(), rs);
  }

  if (post) {
    (*post) = std::string(re, path.end());
  }

  return true;
}

bool FileExists(const std::string &filepath, void *userdata) {
  (void)userdata;

  bool ret{false};
#ifdef TINYUSDZ_ANDROID_LOAD_FROM_ASSETS
  if (asset_manager) {
    AAsset *asset = AAssetManager_open(asset_manager, filepath.c_str(),
                                       AASSET_MODE_STREAMING);
    if (!asset) {
      return false;
    }
    AAsset_close(asset);
    ret = true;
  } else {
    return false;
  }
#else
#ifdef _WIN32
#if defined(_MSC_VER) || defined(__GLIBCXX__) || defined(_LIBCPP_VERSION)
  FILE *fp = nullptr;
  errno_t err = _wfopen_s(&fp, UTF8ToWchar(filepath).c_str(), L"rb");
  if (err != 0) {
    return false;
  }
#else
  FILE *fp = nullptr;
  errno_t err = fopen_s(&fp, filepath.c_str(), "rb");
  if (err != 0) {
    return false;
  }
#endif

#else
  FILE *fp = fopen(filepath.c_str(), "rb");
#endif
  if (fp) {
    ret = true;
    fclose(fp);
  } else {
    ret = false;
  }
#endif

  return ret;
}

std::string FindFile(const std::string &filename,
                     const std::vector<std::string> &search_paths) {
  // TODO: Use ghc filesystem?

  if (filename.empty()) {
    return filename;
  }

  if (search_paths.empty()) {
    std::string absPath = io::ExpandFilePath(filename, /* userdata */ nullptr);
    if (io::FileExists(absPath, /* userdata */ nullptr)) {
      return absPath;
    }
  }

  for (size_t i = 0; i < search_paths.size(); i++) {
    std::string absPath = io::ExpandFilePath(
        io::JoinPath(search_paths[i], filename), /* userdata */ nullptr);
    if (io::FileExists(absPath, /* userdata */ nullptr)) {
      return absPath;
    }
  }

  return std::string();
}

}  // namespace io
}  // namespace tinyusdz
