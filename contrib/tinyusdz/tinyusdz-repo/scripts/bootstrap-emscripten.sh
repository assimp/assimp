curdir=`pwd`

builddir=${curdir}/build-emcc

rm -rf ${builddir}
mkdir ${builddir}


# Assume emcc is already availbe(e.g. through source ./emsdk_env.sh)
cd ${builddir} && CXX=em++ CC=emcc cmake \
  -DCMAKE_VERBOSE_MAKEFILE=1 \
  -DSANITIZE_ADDRESS=1 \
  ..

