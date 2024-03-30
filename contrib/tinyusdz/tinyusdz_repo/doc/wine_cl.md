# WINE + clang-cl build of pure Win32/Win64 TinyUSDZ library

TinyUSDZ can be compiled with clang-cl + MSVC SDK.
And can run it on top of WINE. Pure Windows C/C++ app/library build on Linux!

## MSVC SDK license

https://devblogs.microsoft.com/cppblog/updates-to-visual-studio-build-tools-license-for-c-and-cpp-open-source-projects/

MSVC SDK(Visual Studio Build Tools, Microsoft C++ Build Tools) EULA has been relaxed to use it(grab a copy) for OSS build without Visual Studio license.

Note that Windows SDK does not require VS license.

## Setup

Download llvm prebuilt for your HOST architecure(e.g. Ubuntu x86_64 if you are using x64 Linux()
Note that ubuntu prebuit is provided up to 14.0.0

https://github.com/llvm/llvm-project/releases/tag/llvmorg-14.0.0

Install Ninja build tool.

### WSL

On WSL, easy way is instll VSBT in Windows side.

https://visualstudio.microsoft.com/visual-cpp-build-tools/

You can also use `msvc-wine` in the following.

### Linux(Ubuntu)

https://github.com/mstorsjo/msvc-wine

We recommend to use msvc-wine effort. It automates downloading MSVC SDK and Windows SDK and unpacking it.

Note that recent msvc-wine makes foldername/filename lowercase.

## Cross compile TinyUSDZ with clang-cl 

Plase see `scripts/bootstrap-clang-cl-wsl.sh`

```
  cd build-clang-cl-wsl
  cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE="cmake/clang-cl-msvc-wsl.cmake" \
    -DHOST_ARCH=x64 \
    -DLLVM_NATIVE_TOOLCHAIN="/mnt/d/local/clang+llvm-14.0.0-x86_64-linux-gnu-ubuntu-18.04/" \
    -DMSVC_BASE:FILEPATH="/mnt/d/VC/Tools/MSVC/14.26.28801/" \
    -DWINSDK_BASE="/mnt/d/winsdk/10/" \
    -DWINSDK_VER="10.0.18362.0" \
    ..
```

Edit `MSVC_BASE` and `WINSDK_BASE`(You can use Windows path if you are cross-compiling on WSL), and `WINSDK_VER` to fit your MSVC SDK, Windows SDK.

Note that recent msvc-wine makes foldername/filename lowercase. You may need to edit some foldername/filename(e.g. `Windows.h -> windows.h` in `cmake/clang-cl-msvc-wsl.cmake`

## Run on Wine.

`vcruntime**.dll` and some MSVC/ucrt dlls are required in the PATH.
You can set it through `WINEPATH`

```
$ WINEPATH="%PATH%;C:\path\to\dlls" wine64 ./usda_parser.exe
```

EoL.
