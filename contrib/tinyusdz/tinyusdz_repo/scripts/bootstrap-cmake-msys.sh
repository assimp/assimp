curdir=`pwd`

builddir=${curdir}/build-msys

rm -rf ${builddir}
mkdir ${builddir}

# Assume mingw-32-make, clang, etc is installed
# Easyest way is to unpack llvm-mingw32 and add path to it.

cd ${builddir} && CXX=clang++.exe CC=clang.exe cmake \
  -G "MinGW Makefiles" \
  -DCMAKE_VERBOSE_MAKEFILE=1 \
  ..
