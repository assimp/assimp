curdir=`pwd`

builddir=${curdir}/build

rm -rf ${builddir}
mkdir ${builddir}


cd ${builddir} && cmake \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSANITIZE_ADDRESS=On \
  -DCMAKE_VERBOSE_MAKEFILE=1 \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
  ..

