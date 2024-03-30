curdir=`pwd`

builddir=${curdir}/build

rm -rf ${builddir}
mkdir ${builddir}

# with lld linker
#  -DCMAKE_TOOLCHAIN_FILE=cmake/lld-linux.toolchain.cmake 

cd ${builddir} && cmake \
  -DCMAKE_VERBOSE_MAKEFILE=1 \
  ..

