rm -rf build-clang-cl-wsl
mkdir build-clang-cl-wsl

# Path containing space does not work well, so use symlink to MSVC_BASE and WINSDK_BASE.
# Assume LLVM_NATIVE_TOOLCHAIN points to linux version of clang+llvm
# 
cd build-clang-cl-wsl
cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE="cmake/clang-cl-msvc-wsl.cmake" \
  -DHOST_ARCH=x64 \
  -DLLVM_NATIVE_TOOLCHAIN="/mnt/d/local/clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04/" \
  -DMSVC_BASE:FILEPATH="/mnt/d/VC/Tools/MSVC/14.26.28801/" \
  -DWINSDK_BASE="/mnt/d/winsdk/10/" \
  -DWINSDK_VER="10.0.18362.0" \
  ..

