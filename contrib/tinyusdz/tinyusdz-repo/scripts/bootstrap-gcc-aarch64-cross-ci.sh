curdir=`pwd`

builddir=${curdir}/build-cross

rm -rf ${builddir}
mkdir ${builddir}

cd ${builddir} && cmake \
  -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-linux-gnu.toolchain \
  -DCMAKE_VERBOSE_MAKEFILE=1 \
  -DTINYUSDZ_WITH_EXR=1 \
  -DTINYUSDZ_WITH_TIFF=1 \
  -DTINYUSDZ_BUILD_TESTS=Off \
  -DTINYUSDZ_BUILD_EXAMPLES=Off \
  ..

