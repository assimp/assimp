rmdir /s /q build
mkdir build

cd build
cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE="cmake/clang-cl-msvc-windows.cmake" ^
  -DHOST_ARCH=x64 ^
  -DLLVM_NATIVE_TOOLCHAIN="C:/Program Files/LLVM/" ^
  -DMSVC_BASE:FILEPATH="C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\BuildTools\\VC\\Tools\\MSVC\\14.26.28801\\" ^
  -DWINSDK_BASE="C:\\Program Files (x86)\\Windows Kits\\10\\" ^
  -DWINSDK_VER="10.0.18362.0" ^
  ..

