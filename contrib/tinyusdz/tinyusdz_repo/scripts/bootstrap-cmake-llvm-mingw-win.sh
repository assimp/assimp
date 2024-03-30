# llvm-mingw + mintty bash(git for Windows)
# Assume Ninja is installed on your system
curdir=`pwd`

# Set path to llvm-mingw in env var.
set LLVM_MINGW_DIR=/d/local/llvm-mingw-20200325-ubuntu-18.04/

builddir=${curdir}/build-llvm-mingw

rm -rf ${builddir}
mkdir ${builddir}

cd ${builddir} && cmake \
  -DCMAKE_TOOLCHAIN_FILE=${curdir}/cmake/llvm-mingw-win64.cmake \
  -G "Ninja" \
  -DCMAKE_VERBOSE_MAKEFILE=1 \
  ..

