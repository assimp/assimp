curdir=`pwd`

builddir=${curdir}/build-gcc5

rm -rf ${builddir}
mkdir ${builddir}

cd ${builddir} && cmake \
  -DCMAKE_C_COMPILER=gcc-5 \
  -DCMAKE_CXX_COMPILER=g++-5 \
  -DCMAKE_VERBOSE_MAKEFILE=1 \
  ..

