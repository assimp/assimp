# mingw gcc cross compile
curdir=`pwd`

# Set path to mingw in env var(if required).
export MINGW_GCC_DIR=/usr

builddir=${curdir}/build-mingw

rm -rf ${builddir}
mkdir ${builddir}

cd ${builddir} && cmake \
  -DCMAKE_TOOLCHAIN_FILE=${curdir}/cmake/mingw64-cross.cmake \
  -DCMAKE_VERBOSE_MAKEFILE=1 \
  ..

cd ${curdir}
